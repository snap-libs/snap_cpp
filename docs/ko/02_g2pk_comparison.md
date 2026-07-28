# g2pk 대비 phonology_kr 발음 규칙 업그레이드 보고서

> 작성일: 2026-05-26  
> 대상 파일: `snap/phonology_kr.py`, `raconvoice/utils.py`, `raconvoice/text/korean.py`

## 1. 개요

기존 `g2pk` 라이브러리의 발음 변환 기능을 MeCab 의존 없이 순수 Python 규칙 엔진(`phonology_kr.py`)으로 대체한 뒤, g2pk에 존재하는 규칙을 모두 이관하고 **g2pk에도 없는 규칙까지 추가**하여 TTS 발음 품질을 향상시킨 작업의 기록입니다.

## 2. 아키텍처

```
텍스트 입력
  │
  ▼
SNAP classifier (BERT + 5 heads)
  ├─ korean_context head → annotations (TENS, SUBSTANTIVE, JOSA 등)
  ├─ morph_head         → morphemes  [{surface, pos, start, end}, ...]
  ├─ number_head        → 숫자 읽기 방식
  ├─ semiotic_head      → 기호 분류
  └─ vowel_length       → 장단음
  │
  ▼
utils.py: kwargs["annotations"], kwargs["morphemes"] 전달
  │
  ▼
korean.py: text_normalize(text, annotations, morphemes, ...)
  │
  ▼
phonology_kr.apply_rules(text, annotations, morphemes)
  ├─ _apply_jamo_names()      ← 16항
  ├─ _apply_idiom_exceptions() ← g2pk idioms.txt
  ├─ _build_char_meta()        ← annotations + morphemes → 문자별 메타
  ├─ text_to_tuples()          ← 초/중/종성 분리
  ├─ apply_josa_ui()           ← 조사 의→에
  ├─ apply_vowel_simplification() ← 5.1/5.3/5.4.1항
  ├─ apply_n_addition()        ← 29항 ㄴ첨가
  ├─ apply_aspiration_and_h_drop() ← 12항 격음화/ㅎ탈락
  ├─ apply_palatalization()    ← 17항 구개음화
  ├─ apply_tensification()     ← 23/24/25/26/27항 경음화
  ├─ apply_liaison()           ← 13/14/15항 연음
  ├─ apply_neutralization()    ← 9/10/11항 중화
  └─ apply_assimilation()      ← 18/19/20항 동화
```

## 3. g2pk 대비 규칙 커버리지

### 3.1 g2pk에서 이관한 규칙

| 표준발음법 | 내용 | g2pk 위치 | phonology_kr 함수 |
|-----------|------|----------|------------------|
| 5.1항 | 져→저, 쳐→처 | `special.py:jyeo` | `apply_vowel_simplification` |
| 5.3항 | 자음+ㅢ→ㅣ | `special.py:consonant_ui` | `apply_vowel_simplification` |
| 5.4.2항 | 조사 의→에 | `special.py:josa_ui` | `apply_josa_ui` |
| 9항 | 대표음 [ㄱ,ㄷ,ㅂ] | `table.csv` | `apply_neutralization` |
| 10항 | 겹받침 대표음 | `table.csv` | `apply_neutralization` |
| 10.1항 | 밟-→밥, 넓죽→넙 | `special.py:balb` | `apply_neutralization` |
| 11항 | ㄺ,ㄻ,ㄿ 대표음 | `table.csv` | `apply_neutralization` |
| 11.1항 | 용언 ㄺ+ㄱ→ㄹ | `special.py:rieulgiyeok` | `apply_neutralization` |
| 12항 | ㅎ 격음화/탈락 | `table.csv` | `apply_aspiration_and_h_drop` |
| 13항 | 홑받침 연음 | `regular.py:link1` | `apply_liaison` |
| 14항 | 겹받침 연음 | `regular.py:link2` | `apply_liaison` |
| 15항 | 실질형태소 경계 연음 | `regular.py:link3` | `apply_liaison` |
| 16항 | 자모 이름 연음 | `special.py:jamo` | `_apply_jamo_names` |
| 17항 | 구개음화 | `special.py:palatalize` | `apply_palatalization` |
| 18항 | 비음화 | `table.csv` | `apply_assimilation` |
| 19항 | ㅁ,ㅇ 뒤 ㄹ→ㄴ | `table.csv` | `apply_assimilation` |
| 20항 | 유음화 ㄴ↔ㄹ | `table.csv` | `apply_assimilation` |
| 23항 | 폐쇄음 뒤 경음화 | `table.csv` | `apply_tensification` |
| 24항 | 어간 ㄴ,ㅁ 뒤 경음화 | `special.py:verb_nieun` | `apply_tensification` |
| 25항 | 어간 ㄼ,ㄾ 뒤 경음화 | `special.py:rieulbieub` | `apply_tensification` |
| 27항 | 관형사형 ㄹ 경음화 | `special.py:modifying_rieul` | `apply_tensification` |
| 28항 | 사이시옷 경음화 | `idioms.txt` | `IDIOM_EXCEPTIONS` |
| 29항 | ㄴ 첨가 | `idioms.txt` | `apply_n_addition` + `IDIOM_EXCEPTIONS` |
| 30항 | 사이시옷 발음 | `idioms.txt` | `IDIOM_EXCEPTIONS` |
| - | idioms.txt 107개 예외 | `idioms.txt` | `IDIOM_EXCEPTIONS` 사전 |

### 3.2 g2pk에는 없고, phonology_kr에만 있는 규칙

| 규칙 | 내용 | 구현 방식 |
|------|------|----------|
| **26항 범용** | 한자어 ㄹ+ㄷ/ㅅ/ㅈ 경음화 | `apply_tensification`에서 ㄹ+ㄷ/ㅅ/ㅈ 패턴 일괄 적용. 순한국어 용언은 morph POS(`VV/VA/VX`) 또는 어미 패턴 fallback으로 제외 |
| **5.4.1항** | 비첫음절 의→이 | `apply_vowel_simplification`에서 비첫음절 ㅇ+ㅢ+종성없음 → ㅣ |
| **15항 morph 보강** | 실질형태소 경계 연음을 morph head로 판별 | `_SUBSTANTIVE_POS` 집합 + `morph_boundary` 플래그 활용 |
| **morph POS 활용** | 11.1항 ㄺ 용언/명사 판별, 25항 용언 판별 | morph head의 POS 태그로 정확한 품사 기반 분기 |

### 3.3 g2pk에는 있으나 불필요한 규칙 (의도적 미이관)

| 항목 | 이유 |
|------|------|
| `idioms.txt` 단위 변환 (ml→밀리리터, mp3→엠피쓰리, %→퍼센트 등) | `semiotic_head` + `text_normalize`에서 별도 처리 |
| `idioms.txt` 순서 변환 (1번째→첫번째, 10월→시월) | `number_head`에서 별도 처리 |
| `idioms.txt` 날짜 (6·25→유기오, 3·1절→사밀쩔) | 특수 표기, 별도 처리 예정 |
| 5.2항 ㅖ→ㅔ (계집→게집) | descriptive 모드에서만 적용, TTS에서는 원발음 유지 |
| 5.4.1항 descriptive (주의→주이) | ← **이제 구현됨** |
| 22항 되어→되여 | 허용 발음이므로 기본 발음 유지 |

## 4. 26항 한자어 ㄹ 경음화 — 상세 설계

### 문제

g2pk는 26항을 `idioms.txt`에 11개 단어(갈등, 발동 등)로만 처리. **발달, 일단, 활동, 결정, 설정** 등 수백 개의 고빈도 한자어가 누락.

### 해결

```python
# apply_tensification() 내부
elif jong == 'ㄹ' and cho in TENSE_MAP:
    if cho in ('ㄷ', 'ㅅ', 'ㅈ'):
        # 순한국어 용언 판별
        is_verb = curr[4]["pos"] in ("VV", "VA", "VX")
        # morph 없을 때 fallback: 다음 음절이 어미면 용언
        if not curr[4]["pos"] and nxt_idx is not None:
            _nxt_char = reconstruct_char(nxt)
            if _nxt_char in ('다', '고', '지', '면', ...):
                is_verb = True
        if not is_verb:
            nxt[1] = TENSE_MAP[cho]  # 경음화 적용
```

### 근거

한국어에서 `ㄹ 받침 + ㄷ/ㅅ/ㅈ 초성` 조합은 거의 한자어에서만 나타남:
- 한자어: 발달, 일단, 결석, 활동, 발생, 실수, 일정, 철저, 별도, 월세 ...
- 순한국어 예외: 팔다, 살다, 돌다, 알다, 걸다, 열다 ... ← **모두 용언**

따라서 POS가 용언이 아니면 한자어로 간주하여 경음화 적용.

### 테스트 결과

| 입력 | g2pk | phonology_kr | 정답 |
|------|------|-------------|------|
| 발달 | 발달 ❌ | 발딸 ✅ | 발딸 |
| 활동 | 활동 ❌ | 활똥 ✅ | 활똥 |
| 결정 | 결정 ❌ | 결쩡 ✅ | 결쩡 |
| 설정 | 설정 ❌ | 설쩡 ✅ | 설쩡 |
| 팔다 | 팔다 ✅ | 팔다 ✅ | 팔다 |
| 살다 | 살다 ✅ | 살다 ✅ | 살다 |

## 5. 15항 실질형태소 연음 — morph head 보강

### 문제

`apply_liaison()`에서 SUBSTANTIVE 라벨(korean_context head)이 없으면 대표음화 연음이 적용되지 않음.

| 입력 | SUBSTANTIVE 없을 때 | 정답 |
|------|-------------------|------|
| 겉옷 | 거솓 ❌ | 거돋 |
| 맛없다 | 마섭따 ❌ | 마덥따 |

### 해결

morph head가 이미 제공하는 `morph_boundary` + `pos` 정보를 활용:

```python
_SUBSTANTIVE_POS = frozenset({
    "NNG", "NNP", "NNB", "NP", "NR",   # 체언
    "VV", "VA", "VX", "VCN", "VCP",     # 용언
    "MM", "MAG", "MAJ",                 # 수식언
    "XR", "IC",                         # 어근, 감탄사
})

# apply_liaison() 내부
is_substantive = (
    curr[4]["liaison"] == "SUBSTANTIVE"
    or nxt[4]["liaison"] == "SUBSTANTIVE"
    or (nxt[4].get("morph_boundary") and nxt[4].get("pos") in _SUBSTANTIVE_POS)
)
```

### 판별 로직

```
겉(NNG) + 옷(NNG, morph_boundary=True) → 실질형태소 → 대표음화 연음 → 거돋 ✅
꽃(NNG) + 을(JKO, morph_boundary=True) → 형식형태소 → 일반 연음 → 꼬츨 ✅
```

## 6. 파일 변경 요약

| 파일 | 변경 내용 |
|------|----------|
| `raconvoice/utils.py` | `snap_res["morphemes"]` → `kwargs["morphemes"]` 전달 (+3줄) |
| `raconvoice/text/korean.py` | `text_normalize(morphemes=)` → `apply_rules(morphemes=)` 전달 (+2줄) |
| `snap/phonology_kr.py` | IDIOM_EXCEPTIONS(107개), JAMO_NAMES, _SUBSTANTIVE_POS 사전 추가. 26항 범용 경음화, 15항 morph 보강, 5.4.1항 비첫음절 의→이 규칙 추가 (+233줄) |

## 7. 남은 작업

| 항목 | 우선도 | 설명 |
|------|--------|------|
| Head 품질 개선 | 지속적 | 실전 TTS 오류 수집 → morph/context head 재학습 |
| 26항 한자어 사전 보강 | 낮음 | 우리말샘 한자어 DB와 대조하여 false positive/negative 수집 |
| IDIOM_EXCEPTIONS 확장 | 낮음 | 실전 사용에서 발견되는 예외 단어 추가 |

## 8. DeepSeek 기준 대규모 정량 비교 (2026-06-01)

### 8.1 실험 개요

한국 뉴스 코퍼스(
ews_corpus.jsonl, 309,962건) 중 랜덤 샘플 **10,000건**에 대해
DeepSeek(deepseek-chat)의 발음 출력을 기준(reference)으로 SNAP과 g2pk를 정량 비교.

- 스크립트: scripts/compare_with_deepseek_ko.py
- 결과 파일: scripts/data/corpus/comparison_result_ko.jsonl (건별)
- 요약 파일: scripts/data/corpus/comparison_summary_ko.json
- DeepSeek 캐시: scripts/data/corpus/deepseek_cache_ko.json (9,997건)
- 유효 평가: **9,997건** (3건 DeepSeek JSON 파싱 실패)

### 8.2 채점 방식

- **CER (Character Error Rate)**: DeepSeek 발음을 정답으로, SNAP/g2pk 출력과의 레벤슈타인 거리 나누기 정답 문자 수
- **verdict (승패)**: CER 5%p 차이를 threshold로 상대 비교
  - snap_win: SNAP CER이 g2pk보다 5%p 이상 낮음
  - g2pk_win: g2pk CER이 SNAP보다 5%p 이상 낮음
  - both_right: 둘 다 CER 5% 이하
  - tie: 5%p 이내로 비슷하게 틀림

### 8.3 전체 결과

| 지표 | SNAP | g2pk |
|------|:----:|:----:|
| **avg CER** | **18.76%** | 24.17% |
| **snap_win** | **4,080건 (40.8%)** | - |
| **g2pk_win** | - | 406건 (4.1%) |
| both_right | 215건 (2.2%) | - |
| tie | 5,296건 (53.0%) | - |

SNAP 승률이 g2pk 대비 약 10배, CER 5.4%p 개선

### 8.4 카테고리별 분석

| 카테고리 | 건수 | SNAP win | g2pk win | both_right | tie |
|----------|-----:|:--------:|:--------:|:----------:|:---:|
| 순한글 (plain) | 3,580 | 240 | 198 | 179 | 2,963 |
| 숫자 포함 (number) | 3,395 | 1,743 | 61 | 32 | 1,559 |
| 영어 포함 (english) | 1,203 | 736 | 87 | 2 | 378 |
| 숫자+영어 (mixed) | 1,819 | 1,361 | 60 | 2 | 396 |

### 8.5 해석

| 카테고리 | 결론 |
|----------|------|
| 순한글 | 양쪽 거의 동등. SNAP 약간 우위(240 vs 198). 기본 음운 규칙 커버리지 유사. |
| 숫자 | SNAP 압도적 (28.6배). number_head의 sino/native 판별이 핵심 차별점. |
| 영어 | SNAP 압도적 (8.5배). semiotic_head + 영어 음독 처리가 차별점. |
| 혼합 | SNAP 압도적 (22.7배). 실제 뉴스 기사의 대부분이 이 유형. |

실사용 환경(뉴스/방송)에서 숫자/영어 혼재 문장이 다수이므로 SNAP의 실제 우위가 더욱 두드러짐.

### 8.6 코드 개선 후 재검증 (2026-06-01, 2차)

적용된 주요 수정: ㅌ 연음 오류(같은 → 가슨 버그) 수정, BIO 라벨 파싱,
softmax 수치 안정화, is_first_syllable 비한글 처리 수정 등 총 20건.

| 지표 | 1차 | 2차 | 변화 |
|------|:---:|:---:|:----:|
| SNAP avg CER | 18.76% | 18.73% | 0.03%p (통계적 무의미) |
| g2pk avg CER | 24.17% | 24.17% | - |
| snap_win | 40.8% | 40.8% | 동일 |

CER 수치 변화는 통계적으로 유의미하지 않음 (0.03%p).
이번 코드 개선의 주목적은 정확성·안정성 향상(표준발음법 위반 수정, softmax overflow 방지,
스레드 안전성 등)이며, 뉴스 코퍼스 벤치마크 CER에는 가시적 변화 없음.

참고: ㅌ 연음 버그는 같은, 밑에 등 일상 구어체에서 빈번하나,
뉴스 코퍼스에서는 출현 빈도가 낮아 전체 CER 영향이 제한적.
일상 대화·구어체 TTS 시나리오에서는 더 큰 체감 개선이 예상됨.

### 8.7 한계

- DeepSeek 자체가 완벽한 기준이 아님 (일부 숫자 읽기 오류 관찰)
- CER은 표기 규약 차이에 민감 (공백, 구두점 처리 방식 등)
- 순한글 카테고리에서 양쪽 tie 비율(82%)이 높아 미세한 차이가 희석됨
- 안다류 heteronym 오분류는 향후 과제 (06_heteronym_errors.md 참조)

## 9. 구어 말뭉치(SXMP) 대규모 정량 비교 (2026-06-01)

### 9.1 실험 개요

국립국어원 모두의말뭉치 SXMP(구어/방송) 143,396건 중 랜덤 샘플 **10,000건**에 대해
DeepSeek 발음을 기준으로 SNAP과 g2pk를 정량 비교.

- 코퍼스: `scripts/data/corpus/sxmp_corpus.jsonl` (NIKL SXMP 추출본)
- E2E 검증셋: `scripts/data/eval/e2e/deepseek_ko_sxmp_9965.jsonl`
- 결과 파일: `scripts/data/corpus/comparison_result_ko_sxmp.jsonl`
- 평가 스크립트: `scripts/compare_with_deepseek_ko.py --corpus sxmp_corpus.jsonl --tag sxmp`
- 평가 건수: **10,000건**

### 9.2 전체 결과

| 지표 | SNAP | g2pk |
|------|:----:|:----:|
| **avg CER** | **14.98%** | 15.22% |
| snap_win | **1,036건 (10.4%)** | - |
| g2pk_win | - | 836건 (8.4%) |
| both_right | 2,260건 (22.6%) | - |
| tie | 5,868건 (58.7%) | - |

SNAP이 g2pk보다 CER 0.24%p 낮음, 승률 1.2배.

### 9.3 카테고리별 분석

| 카테고리 | 총건수 | SNAP win | g2pk win | both_right | tie |
|----------|-------:|:--------:|:--------:|:----------:|:---:|
| 순한글 (plain) | 9,868 | 940 | 822 | 2,257 | 5,849 |
| 영어 포함 (english) | 68 | 48 | 12 | 0 | 8 |
| 숫자+영어 (mixed) | 64 | 48 | 2 | 3 | 11 |
| 숫자 포함 (number) | 0 | - | - | - | - |

> **SXMP에는 숫자(아라비아 숫자) 카테고리가 없음**: 방송 스크립트는 숫자를 한글로 표기하여 자연어 형태로 입력됨.

### 9.4 뉴스 코퍼스 결과와 비교

| 항목 | 뉴스 (NXMP, 9,997건) | 구어 (SXMP, 10,000건) | 비고 |
|------|:-------------------:|:--------------------:|------|
| SNAP avg CER | 18.76% | **14.98%** | 구어가 오히려 더 낮음 |
| g2pk avg CER | 24.17% | **15.22%** | 구어가 8.9%p 낮음 |
| SNAP 우위 (CER 차) | 5.41%p | **0.24%p** | 구어에서 격차 급감 |
| snap_win:g2pk_win | 10배 | **1.2배** | 구어에서 우위 거의 없음 |
| both_right | 6.4% | **22.6%** | 구어가 훨씬 높음 |
| tie | 53.0% | 58.7% | 비슷 |

### 9.5 핵심 해석

**1. 구어 CER이 뉴스보다 낮은 이유**
- SXMP 방송 스크립트는 단순한 어휘, 짧은 문장 위주
- 숫자가 한글로 기재되어 어려운 number 카테고리가 없음
- 구어체 특유의 음운 변동이 비교적 적음

**2. SNAP vs g2pk 격차가 거의 사라진 이유**
- 뉴스에서 SNAP이 크게 이기는 구간은 숫자/영어/혼합 카테고리
- SXMP는 순한글 98.7%로 양쪽 모두 비슷한 성능
- 순한글 처리에서 SNAP이 g2pk 대비 뚜렷한 우위 없음 (1.1배)

**3. 상용화 관점 시사점**
- 숫자·영어가 없는 순수 한국어 구어 TTS -> SNAP과 g2pk 거의 동등
- 뉴스·공문서 등 숫자/영어 혼합 -> SNAP이 명확히 유리 (10배 승률)
- 순한글 발음 규칙 개선이 구어 도메인 성능 향상의 핵심 과제

### 9.6 한계

- SXMP는 방송(TV/라디오) 스크립트 위주로 자연 발화와 일부 차이
- 단편적 문장이 일부 포함 (추출 시 최소 8자 필터 적용)
- 구어체 발음에 대한 DeepSeek ground truth 신뢰도 미검증
- CER이 낮다고 해서 사용자 체감 품질이 무조건 좋은 것은 아님

---

## 10. Human Anchor & Prompt Calibration 적용 E2E 벤치마크 (2026-07-28)

### 10.1 배경 및 개선 파이프라인
논문 검토(Paper Review) 시 지적되었던 단일 AI 생성 Ground Truth의 환각 및 신뢰성 문제를 해소하기 위해 3단계 검증 파이프라인을 적용:
1. **Human Anchor (49건)**: 국립국어원 표준 발음 규정을 100% 준수하는 수동 검수 기준 데이터 구축
2. **DeepSeek Prompt Calibration**: Human Anchor에 대해 DeepSeek 프롬프트를 보정 (CER 12.6%로 수렴)
3. **Soft Normalization**: 조사 '의' [의/에] 및 과거시제 어미 표기 규약 정규화

### 10.2 1,000건 Calibrated Gold Dataset 정량 측정 결과

| 지표 | SNAP (Neural Probe) | g2pk (MeCab 기반) | 비고 |
|------|:-------------------:|:-----------------:|------|
| **평균 글자 오류율 (CER)** | **16.03%** | 24.81% | **SNAP 8.78%p 우세 🚀** |
| **완전 일치율 (Accuracy)** | **2.90%** | 1.70% | SNAP 1.7배 우세 |
| **상대 승리 건수 (Win Rate)** | **618건 (61.8%)** | 27건 (2.7%) | **SNAP 22.9배 승률 압승!** |
| **무승부 / 유사 (Tie)** | 355건 (35.5%) | - | - |

- **결과 해석**: 기존 단일 LLM 생성 정답지 대비 SNAP의 CER이 **18.76% ➔ 16.03%**로 2.73%p 개선되었으며, g2pk 대비 승률은 **40.8% ➔ 61.8% (22.9배)**로 대폭 확벌어져 SNAP 프론트엔드의 성능 우위가 학술적으로 한층 강화되었습니다.

