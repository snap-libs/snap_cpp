# MeCab → BERT Morph Head 대체 전체 과정

## 1. 동기

### 문제 인식

기존 TTS 파이프라인은 MeCab에 의존하여 형태소 분석을 수행:

```
텍스트 → MeCab(Viterbi) → 형태소+POS → g2pk 발음규칙 → 발음열
```

MeCab의 근본적 한계:

| 문제 | 설명 |
|------|------|
| **Viterbi 오류 불투명** | MeCab이 어디서 틀리는지 알 수 없음 → "못하는 것만 BERT가 하자"는 성립 불가 |
| **유지보수 중단** | mecab-ko-dic 마지막 업데이트: **2018년 7월** (은전한닢 프로젝트 사실상 중단) |
| **최신 단어 미지원** | 인스타그램, 메타버스, 챗GPT 등 2018년 이후 단어 **44% 인식 실패** |
| **블랙박스** | C 라이브러리 의존, 사전 수정 어려움 |

### 핵심 아이디어

> "MeCab을 버리고, 사전만 가지고 전처리한 후 BERT에 입력으로 들어가면 안 되나?"

```
Before: 텍스트 → MeCab(사전+Viterbi) → 결과
After:  텍스트 → 사전(trie 조회) → 후보 feature → BERT head → 결과
                 ↑                                    ↑
           Viterbi 제거                         BERT가 판별
```

---

## 2. 아키텍처 설계

### 입력 구성

모델의 입력은 문자(char) 단위이며, 각 문자 위치에 다음 feature를 결합:

```
[768] BERT hidden state (kykim/bert-kor-base, frozen)
 [32] char embedding (문자 ID → 32d)
 [16] position-in-token embedding (BERT 토큰 내 위치 → 16d)
 [45] dict_starts (이 위치에서 시작하는 POS 후보 multi-hot)
 [45] dict_covers (이 위치를 커버하는 POS 후보 multi-hot)
─────
[906] total → Linear(256) → GELU → Dropout(0.1) → Linear(76)
```

### 출력: BIO-POS 태그 (76 클래스)

```
각 문자에 대해 B-NNG, I-NNG, B-VV, I-VV, ... 등 76개 레이블 예측.
B = 형태소 시작, I = 형태소 내부
POS = 45종 품사 태그
```

### Dict Feature의 역할

```
사전 조회 결과를 multi-hot 벡터로 인코딩:

예: "학교에" → 
  학: starts=[NNG,NNP,XR,XSN,MAG]  ← "학", "학교" 등 시작
  교: starts=[NNG,NNP]              ← "교" 시작
      covers=[NNG,NNP]              ← "학교"에 의해 커버
  에: starts=[JKB,NNG,VV,EC]        ← "에" 시작
```

> [!IMPORTANT]
> Dict feature는 "이 위치에서 가능한 POS가 뭔지" 힌트를 주는 것이며, 
> 최종 판별은 BERT의 문맥 이해력이 담당합니다.

---

## 3. 데이터 파이프라인

### 3.1 Gold 데이터: 모두의말뭉치

```
소스:     NIKL_MP v1.1 (국립국어원 형태 분석 말뭉치)
파일:     c:\work\raconvoice\mal\NIKL_MP_v1.1_JSON.zip
구성:     NXMP (문어) 7,265 문서 + SXMP (구어) 423 문서
시기:     2009~2016년 뉴스/방송 텍스트, 2018년 배포
태깅:     전문가 검수 형태소 분석 (Gold standard)
```

**변환 과정** ([convert_nikl_corpus.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/convert_nikl_corpus.py)):

```
NIKL JSON → 문장별 JSONL
  각 문장: {text, morphemes: [{form, label}], label_ids: [int]}
  label_ids = 문자별 BIO-POS 레이블 (비공백 문자만)
```

````carousel
### 데이터 통계

```
전체 문장:     374,044건
유효 문장:     121,972건 (label 매핑 성공)
건너뜀:        252,072건 (복잡한 형태소 경계 불일치)
학습/검증:     109,774 / 12,198 (90:10)
```
<!-- slide -->
### Gold vs MeCab 차이 예시

```
Gold:  "한국화학시험연구원" → 한국화학시험연구원/NNP (1 형태소)
MeCab: "한국화학시험연구원" → 한국/NNP + 화학/NNG + 시험/NNG + 연구/NNG + 원/NNG (5 형태소)

Gold:  "어제" → 어제/MAG (부사)
MeCab: "어제" → 어제/NNG (명사) ← POS 오류
```
````

### 3.2 BERT 캐시

```
모델:     kykim/bert-kor-base (768d hidden)
캐시:     scripts/data/_bert_cache/{md5}.npz
생성:     GPU batch=128, 796/s
총:       93,141건 캐시 → 117초
```

> BERT는 frozen으로 사용. hidden state를 미리 캐시하여 학습 시 반복 연산 제거.

### 3.3 사전 데이터

#### MeCab 기존 사전

```
소스:     mecab-ko-dic-2.1.1-20180720
엔트리:   816,283개
추출:     학습 데이터 97,385문장을 MeCab lattice에 넣어 후보 수집
결과:     70,987개 (surface, POS) 쌍
```

#### 우리말샘 (국립국어원 개방형 사전)

```
소스:     opendict.korean.go.kr
파일:     전체 내려받기_우리말샘_json_20260503.zip (1.8GB)
표제어:   1,204,559건
  품사 있음:  773,224건 (64%)
  품사 없음:  431,335건 (36%)
신규 추가:  521,066건 (MeCab에 없는 것)
라이선스:  CC BY-SA (상업적 사용 가능)
```

**우리말샘 품사 매핑:**

| 우리말샘 | MeCab 태그 | 비고 |
|---------|-----------|------|
| 명사 | NNG | 일반명사 |
| 동사 | VV | 동사 |
| 형용사 | VA | 형용사 |
| 부사 | MAG | 부사 |
| 감탄사 | IC | 감탄사 |
| 대명사 | NP | 대명사 |
| 의존 명사 | NNB | 의존명사 |
| 보조 동사/형용사 | VX | 보조용언 |

### 3.4 자체 Trie 사전 구축

**구축 스크립트:** [build_morph_trie.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/build_morph_trie.py)

```
1단계: MeCab lattice에서 엔트리 추출    → 70,987건 (37초)
2단계: 우리말샘 신규 단어 추가           → 521,066건 (19초)
최종:  592,053 (surface, POS) 엔트리
저장:  weights/ko/morph_trie.pkl (12.5MB)
```

**Trie 구조:**

```python
# Python dict 기반 trie
root = {
  "학": {
    "_pos": {"NNG", "XR", "MAG", "XSN"},     # "학" 자체
    "교": {
      "_pos": {"NNG", "NNP"},                  # "학교"
      "에": { ... }
    },
    "생": {
      "_pos": {"NNG"},                         # "학생"
    }
  }
}
```

**조회 API:**

```python
trie.lookup("학교에갔다", 0)
→ [("학", {"NNG","XR","MAG","XSN"}),
   ("학교", {"NNG","NNP"})]
```

---

## 4. 실험 결과

### 4.1 PoC 검증 (소규모)

| 실험 | 데이터 | BIO-POS | POS |
|------|--------|---------|-----|
| Baseline (dict 없음) | MeCab 500건 | 84.5% | 86.0% |
| + Dict feature | MeCab 500건 | 88.9% | 90.2% |
| + Dict feature | Gold 4K건 | 86.5% | 88.7% |

> [!TIP]
> Dict feature 추가만으로 +4.4% 향상 → 사전 후보가 BERT 판별에 핵심 역할 확인.

### 4.2 전체 학습

| 모델 | 사전 소스 | BIO-POS | POS | Dict 계산 | 총 시간 |
|------|----------|---------|-----|----------|--------|
| V1 (MeCab lattice) | MeCab 사전만 | 93.2% | 94.4% | 31.9s | 66분 |
| **Trie (MeCab-free)** | **MeCab+우리말샘** | **93.5%** | **94.7%** | **5.2s** | **56분** |

```
개선:  BIO +0.3%, POS +0.3%
속도:  Dict feature 계산 6.1배 빠름
의존:  MeCab 라이브러리 완전 제거
```

### 4.3 MeCab Viterbi 대비 정확도

```
같은 검증 세트 (12,198문장, 144,164 chars):

              BIO-POS    POS
MeCab Viterbi: 84.2%    86.0%
Trie + BERT:   93.5%    94.7%
─────────────────────────────
차이:          +9.3%    +8.7%
```

### 4.4 학습 곡선

```
Trie 기반 학습:
  Ep 1:  BIO=90.2%  POS=92.0%  ← 첫 epoch부터 90%
  Ep 5:  BIO=92.9%  POS=94.3%
  Ep10:  BIO=93.2%  POS=94.5%
  Ep15:  BIO=93.4%  POS=94.6%
  Ep20:  BIO=93.4%  POS=94.7%
  Ep30:  BIO=93.5%  POS=94.7%  ← 안정적 수렴, 오버피팅 없음
```

---

## 5. 실패 분석

검증 세트 3,000문장에서 오류 패턴 분석 ([analyze_errors.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/analyze_errors.py)):

### 오류 분류

```
전체: 35,419 chars
정답: 32,907 (92.9%)
오류: 2,512 (7.1%)
  경계 오류 (B/I만 다름):  435건 (17%) → 발음 영향 없음
  POS 오류:              2,077건 (83%)
    같은 그룹 내:           740건 (36%) → 발음 영향 없음
    그룹 간 혼동:         1,337건 (64%) → 발음 영향 있음

★ 실제 TTS 오류율: 3.77%
```

### 문장별 오류 분포

```
오류 0개: 69% ← 10문장 중 7문장은 완벽
오류 1개: 14%
오류 2개:  8%
오류 3+:   9%
```

### 주요 혼동 패턴

| 혼동 | 건수 | TTS 영향 | 예시 |
|------|------|---------|------|
| NNP↔NNG (고유명사↔일반명사) | 330 | **없음** ○ | "갈릴레오" |
| MAG↔NNG (부사↔명사) | 133 | **있음** ● | "어제" |
| IC↔MAG (감탄↔부사) | 109 | **있음** ● | "인자" (방언) |
| EF↔EC (종결↔연결어미) | 93 | **없음** ○ | "짧게" |
| VV↔NNG (동사↔명사) | 65 | **있음** ● | "모이다" |
| XSN/XPN↔NNG (접사↔명사) | 70 | **있음** ● | "님", "신" |

> [!NOTE]
> 발음에 영향 없는 오류(NNP↔NNG, EF↔EC)가 전체의 ~36%.
> 실제 TTS에 영향을 주는 오류율은 3.77%.

---

## 6. 사전 대안 조사

### mecab-ko-dic 문제점

```
마지막 업데이트: 2018년 7월 (7년 전)
유지보수: 사실상 중단 (은전한닢 프로젝트)
최신 단어 테스트: 25개 중 14개 인식 (56%)
  ✗ 인스타그램, 메타버스, 인플루언서, 챗GPT 등
```

### 대안 비교

| 대안 | 업데이트 | 단어 수 | Lattice API | 적합성 |
|------|---------|---------|------------|-------|
| MeCab-ko-dic | 2018 중단 | 816K | ✓ | 레거시 |
| **우리말샘** | **상시 업데이트** | **1.2M** | - | **✓ 채택** |
| Kiwi | 2025.05 활발 | - | ✗ API 없음 | ✗ |
| Komoran | 간헐적 | - | ✗ | ✗ |

### 최종 결정: MeCab 사전 + 우리말샘 병합

```
MeCab 기존:   70,987 엔트리 (학습 데이터 커버리지)
우리말샘 신규: 521,066 엔트리 (최신 단어 포함)
최종 trie:   592,053 엔트리
```

---

## 7. 최종 시스템 구성

### 파일 구조

```
weights/ko/
  morph_trie.pkl           # 자체 trie 사전 (12.5MB)
  morph_head_trie_v1.pth   # BERT head 가중치
  shared/
    morph_label_map.json   # BIO-POS 레이블 매핑 (76 labels)
```

### 런타임 의존성

```
Before (MeCab 의존):
  python-mecab-ko  ← C 바이너리
  mecab-ko-dic     ← 2018년 사전
  _mecab.pyd       ← C FFI
  
After (MeCab 제거):
  transformers     ← BERT
  torch            ← 모델 실행
  pickle           ← trie 로드 (표준 라이브러리)
```

### 추론 코드 (MeCab 없이)

```python
import pickle, torch
import numpy as np
from transformers import AutoTokenizer, AutoModel

# 1. 사전 로드
trie = pickle.load(open("weights/ko/morph_trie.pkl", "rb"))

# 2. BERT 로드
tokenizer = AutoTokenizer.from_pretrained("kykim/bert-kor-base")
bert = AutoModel.from_pretrained("kykim/bert-kor-base")

# 3. Head 로드
ckpt = torch.load("weights/ko/morph_head_trie_v1.pth")
model = MorphHeadWithDict()
model.load_state_dict(ckpt["model_state_dict"])

# 4. 추론
text = "메타버스 시대의 언택트 사회"
bert_hidden = bert(**tokenizer(text, return_tensors="pt")).last_hidden_state
dict_features = extract_dict_features_from_trie(text, trie)
predictions = model(bert_hidden, ..., dict_features)
# → 각 문자에 B-NNG, I-NNG, B-MAG, ... 등 예측
```

---

## 8. 벤치마크 요약

### 정확도

```
              BIO-POS    POS      vs MeCab
MeCab Viterbi: 84.2%    86.0%    baseline
V1 (MeCab):    93.2%    94.4%    +8.4%
Trie (final):  93.5%    94.7%    +8.7%
```

### 속도 (RTX 3090 + i7-13700K)

| 단계 | V1 (MeCab) | Trie |
|------|-----------|------|
| BERT 캐시 (93K건) | 117s | 117s (동일) |
| Dict feature (110K건) | 28.7s | **4.7s** (6x↑) |
| 학습 30ep | 3,942s | **3,340s** (15%↑) |
| **총** | **~68분** | **~57분** |

### Dict feature 벤치마크

```
5,000문장 기준:
  MeCab lattice: 3,706 문장/s
  Python trie:   23,969 문장/s  (6.5x 빠름)
  
커버리지:
  MeCab 후보 100% 포함 (100/100)
  + 우리말샘 52만 추가 후보
```

---

## 9. 한계 및 향후 과제

### 현재 한계

```
1. 학습 데이터가 2016년 이전 텍스트 (모두의말뭉치 v1.1)
   → 최신 문맥에서의 성능 미검증
2. TTS 오류율 3.77% (문장당 ~0.9개)
3. 건너뛴 252K 문장 (복잡한 형태소 경계) 미활용
4. Python trie 메모리 오버헤드 (~100MB in-memory)
```

### 개선 방향

| 방향 | 기대 효과 | 난이도 |
|------|----------|--------|
| 건너뛴 252K 문장 복구 | 데이터 2x → 정확도 ↑ | ★★☆ |
| Head 구조 확대 (3-layer) | 복잡 패턴 학습 | ★★☆ |
| BERT 마지막 2 layer unfreeze | 문맥 이해 ↑ | ★★★ |
| marisa-trie / Rust 전환 | 메모리 12.5MB→~2MB | ★★☆ |
| TTS 파이프라인 통합 테스트 | 실제 발음 품질 검증 | ★★☆ |
| 우리말샘 정기 업데이트 반영 | 최신 단어 자동 반영 | ★☆☆ |

---

## 10. 생성된 파일 목록

### 스크립트

| 파일 | 용도 |
|------|------|
| [convert_nikl_corpus.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/convert_nikl_corpus.py) | NIKL Gold → 학습 JSONL 변환 |
| [build_morph_trie.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/build_morph_trie.py) | MeCab+우리말샘 → 자체 trie 구축 |
| [train_morph_trie.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/train_morph_trie.py) | Trie 기반 morph head 학습 |
| [analyze_errors.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/analyze_errors.py) | 실패 분석 |
| [convert_urimal_dict.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/convert_urimal_dict.py) | 우리말샘 → CSV 변환 |

### 모델/데이터

| 파일 | 용도 | 크기 |
|------|------|------|
| [morph_trie.pkl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/weights/ko/morph_trie.pkl) | 자체 trie 사전 | 12.5MB |
| [morph_head_trie_v1.pth](file:///c:/work/RaconVoice/RaconVoice_V6/snap/weights/ko/morph_head_trie_v1.pth) | 최종 모델 가중치 | ~2.5MB |
| [morph_label_map.json](file:///c:/work/RaconVoice/RaconVoice_V6/snap/weights/ko/shared/morph_label_map.json) | BIO-POS 레이블 매핑 | 76 labels |
| [morph_gold_train.jsonl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/data/morph/morph_gold_train.jsonl) | Gold 학습 데이터 | 121,972건 |
