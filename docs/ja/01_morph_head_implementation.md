# 일본어 SNAP morph_head 구현 계획

> 작성: 2026-05-28  
> 대상: `snap/weights/ja/morph_head_trie.onnx` (신규)  
> 참조: 한국어 morph_head 구현 (`snap/weights/ko/morph_head_trie.onnx`)

---

## 1. 배경

### MeCab 대체 필요성

일본어 TTS 전처리에서 MeCab은 두 가지 핵심 역할을 수행한다:

1. **단어 분리 (Word Segmentation)**: 일본어는 공백이 없어 `株価が上昇した` → `株価/が/上昇/し/た`로 분리해야 사전 룩업이 가능
2. **읽기 할당 (Reading Assignment)**: `株価` → `かぶか`, `上昇` → `じょうしょう`

SNAP의 목표는 **MeCab 없이** ONNX 단일 배포로 동일 기능을 제공하는 것.

### 한국어와의 차이

| | 한국어 morph_head | 일본어 morph_head |
|---|---|---|
| 공백 유무 | 어절 단위 공백 있음 | **공백 없음** (더 어려움) |
| 주요 출력 | 형태소 경계 + POS | 단어 경계 + POS |
| 읽기 출력 | 불필요 (한글=발음) | **필요** (한자→가나) |
| 학습 데이터 | morph_gold_train.jsonl (121,972건) | 동일 방식으로 생성 예정 |
| 사전 | KoNLPy 기반 trie | **UniDic/IPAdic 기반 trie** |

---

## 2. 사용 가능한 도구

현재 환경:

| 도구 | 상태 | 용도 |
|------|------|------|
| `fugashi` | ✅ 설치됨 | UniDic 형태소 분석기 |
| `unidic` | ✅ 설치됨 | UniDic 사전 (29필드) |
| `MeCab` (Python) | ❌ 없음 | — (fugashi로 대체) |

### UniDic 주요 필드

```python
word.surface      # 표층형 (株価)
word.feature.pos1 # 품사1 (名詞)
word.feature.pos2 # 품사2 (普通名詞)
word.feature.kana # 읽기 카타카나 (カブカ)
word.feature.lemma# 기본형 (株価)
```

---

## 3. 레이블 설계

### UniDic pos1 → SNAP 태그 매핑

한국어 76cls (BIO × 38 POS)에 대응하는 일본어 태그셋.

| UniDic pos1 | UniDic pos2 | SNAP 태그 | 설명 |
|-------------|-------------|---------|------|
| 名詞 | 普通名詞 | `NNG` | 일반명사 |
| 名詞 | 固有名詞 | `NNP` | 고유명사 |
| 名詞 | 数詞 | `NR` | 수사 |
| 名詞 | 代名詞 | `NP` | 대명사 |
| 動詞 | * | `VV` | 동사 |
| 形容詞 | * | `VA` | 형용사 |
| 形状詞 | * | `VAX` | な형용사 |
| 副詞 | * | `MAG` | 부사 |
| 助詞 | 格助詞 | `JKG` | 格조사 (が/を/に...) |
| 助詞 | 係助詞 | `JX` | 계조사 (は/も...) |
| 助詞 | 接続助詞 | `JC` | 접속조사 (て/で...) |
| 助詞 | 副助詞 | `JA` | 부사조사 |
| 助動詞 | * | `XSA` | 조동사 (た/です/ます...) |
| 接頭辞 | * | `XPN` | 접두사 |
| 接尾辞 | * | `XSN` | 접미사 |
| 感動詞 | * | `IC` | 감동사 |
| 記号 | * | `SY` | 기호 |
| 補助記号 | 句点 | `SF` | 마침표 |
| 補助記号 | 読点 | `SP` | 쉼표 |
| 空白 | * | `SW` | 공백 |

**BIO 조합**: `B-NNG`, `I-NNG`, `B-VV`, `I-VV`, ...  
**예상 총 클래스**: ~50cls (한국어 76보다 적음)

---

## 4. 구현 단계

### Step 1: 학습 데이터 생성 스크립트

**파일**: `snap/scripts/generate_ja_morph_data.py`

```python
import fugashi
import json
from pathlib import Path

tagger = fugashi.Tagger()

def analyze(text: str) -> list:
    """문장 → [{surface, pos, reading, start, end}, ...]"""
    result = []
    pos = 0
    for word in tagger(text):
        surface = word.surface
        start = text.find(surface, pos)
        result.append({
            "surface": surface,
            "pos": map_pos(word.feature.pos1, word.feature.pos2),
            "reading": word.feature.kana or surface,
            "start": start,
            "end": start + len(surface),
        })
        pos = start + len(surface)
    return result
```

**데이터 소스**: 일본어 뉴스 코퍼스 (한국어와 동일하게 Wikipedia/뉴스 기사 활용)  
**목표 건수**: 50,000~100,000문장 (한국어 121,972건 대비 유사 수준)  
**출력 형식**: `morph_gold_train_ja.jsonl`

```jsonl
{"text": "今日は良い天気です", "morphemes": [{"surface": "今日", "pos": "NNG", "reading": "キョウ", "start": 0, "end": 2}, ...]}
```

---

### Step 2: trie 사전 구축

**파일**: `snap/scripts/build_ja_morph_trie.py`

UniDic 어휘 전체 → trie 구조로 변환.

```python
# trie 노드 구조 (한국어와 동일)
{
    "株": {
        "価": {
            "_pos": ["NNG"],
            "_reading": "カブカ"  # ← 일본어 추가 필드
        }
    },
    ...
}
```

**저장**: `snap/weights/ja/morph_trie.pkl`

> [!NOTE]
> 읽기(_reading)는 trie에만 저장하고, morph_head의 출력 클래스에는 포함하지 않음.  
> 단어 경계 + POS만 neural로 예측하고, 읽기는 trie에서 룩업.  
> 다의어 읽기만 yomi_head(neural)가 처리.

---

### Step 3: morph head 학습

**파일**: `snap/scripts/train_ja_morph.py`

한국어 `train_morph_trie.py`와 동일한 아키텍처:

```
입력:
  bert_hidden:  [1, MAX_CHARS, 768]   ← BERT hidden states
  tok_indices:  [1, MAX_CHARS]         ← 각 문자의 토큰 인덱스
  pos_in_tok:   [1, MAX_CHARS]         ← 토큰 내 위치
  char_ids:     [1, MAX_CHARS]         ← 문자 ID
  dict_starts:  [1, MAX_CHARS, N_POS]  ← trie 시작 피처
  dict_covers:  [1, MAX_CHARS, N_POS]  ← trie 커버 피처

출력:
  logits: [1, MAX_CHARS, N_LABELS]     ← BIO-POS 예측
```

**BERT 모델**: `snap/weights/ja/` (현재 tohoku-nlp 기반, 문자 단위 토크나이저)

---

### Step 4: ONNX 내보내기

**출력**: `snap/weights/ja/morph_head_trie.onnx`  
**입력 이름**: `hidden` (한국어와 통일 — `_run_head` 호환)

> [!IMPORTANT]
> 현재 `yomi_head.onnx`의 입력 이름이 `hidden_states`로 달라 버그가 있었음.  
> `classifier.py`에서 동적으로 입력 이름을 조회하도록 수정 완료 (2026-05-28).

---

### Step 5: classifier.py 연결

`snap_config.json`에 morph 섹션 추가:

```json
{
  "morph": {
    "enabled": true,
    "head_onnx": "morph_head_trie.onnx",
    "label_map": "morph_label_map_ja.json",
    "trie": "morph_trie.pkl"
  }
}
```

`classifier._run_morph()` 로직은 한국어와 동일하게 재사용.

---

### Step 6: phonology_ja.py 연결

morph 결과를 활용하는 추가 기능:

```python
# 현재: 사전 longest-match만으로 읽기 결정
# 개선: morph 경계를 활용해 단어 단위로 trie 룩업
#       → 경계 애매한 경우 morph가 결정적 역할

# 예: 一日中 → 一(NR) + 日中(NNG) 로 분리 → 各각 읽기 독립 처리
#    vs 一日中 → NNG 단일 단어로 분류 → いちにちじゅう
```

**rendaku (연탁) 처리** (추후):
- 복합어 두 번째 요소의 어두 청음 → 탁음 변화
- morph head가 복합어 경계를 제공해야 적용 가능

---

## 5. 검증 계획

### 단위 검증

| 테스트 | 내용 |
|--------|------|
| 단어 분리 정확도 | UD Japanese GSD (~8K 문장) 기준 F1 측정 |
| POS 정확도 | 동일 |
| 읽기 정확도 | 수동 검증 케이스 100건 |

### 회귀 테스트

```bash
python snap/test_ja_integration.py   # 기존 yomi_head 테스트 포함
```

### 알려진 어려운 케이스

| 입력 | 기대 분리 | 어려운 이유 |
|------|----------|------------|
| `一日中` | 一/日中 | 一日(いちにち)로도 읽힘 |
| `東京都` | 東京/都 | 東京都(とうきょうと) 고유명사 |
| `子供たち` | 子供/たち | 子供 vs 子/供 |
| `見る` | 見る | 動詞 단일 토큰 |

---

## 6. 의존성 관계

```
Step 1 (데이터 생성)
    ↓
Step 2 (trie 구축)   ← fugashi + unidic (이미 설치됨)
    ↓
Step 3 (학습)         ← train_morph_trie.py 재활용
    ↓
Step 4 (ONNX)
    ↓
Step 5 (통합)
    ↓
Step 6 (phonology 연결)
```

---

## 7. 미결 사항

> [!IMPORTANT]
> **레이블 수 최종 확정 필요**  
> UniDic의 pos1/pos2 조합 중 실제 코퍼스에 등장하는 것만 선별해야 함.  
> generate_ja_morph_data.py 실행 후 빈도 분석으로 결정.

> [!NOTE]
> **읽기를 trie에만 저장하는 설계의 한계**  
> trie에 없는 OOV 단어는 읽기를 제공할 수 없음.  
> OOV 처리: (1) 히라가나/가타카나는 그대로, (2) 한자 OOV는 phonology_ja의 kanji_dict 폴백.

> [!NOTE]
> **pitch accent head는 이 구현 범위 밖**  
> morph_head 완성 후 별도 계획 수립.
