# SNAP Head Report — Korean TTS Frontend

> 최종 업데이트: 2026-06-14  
> 대상: [classifier.py](file:///c:/work/snap/snap_py/snap/classifier.py) + [phonology_kr.py](file:///c:/work/snap/snap_py/snap/phonology_kr.py)

---

## 1. 시스템 구조

BERT 1회 호출 → hidden states를 4개 neural head가 공유 + 2개 룰 기반 모듈.

```
입력 텍스트
    │
    ▼
┌─────────────────────────────┐
│  BERT (kykim/bert-kor-base) │  ← 1회만 호출
│  hidden states [1,seq,768]  │
└──────────┬──────────────────┘
           │
     ┌──────┼──────────┬──────────┐
     ▼      ▼          ▼          ▼
  morph  semiotic  heteronym   counter
  head   head      head        head
     │      │          │          │
     ▼      ▼          ▼          ▼
  POS/경계  기호분류   경음화판별   단위읽기
     │      │          │          │
     ▼      └──────────┴──────────┘
phonology                │
   _kr.py ←──────────────┘
     │        + number (룰 기반 단위 사전)
     │        + vowel_length (사전)
     │        + phone/ip (정규식)
     ▼
  발음 변환 결과 → TTS
```

### deprecated

| 제거 | 대체 |
|------|------|
| `korean_context_head.onnx` (5cls) | `heteronym_head.onnx` + `model_counter.onnx` |
| annotation `JOSA` | morph POS `JKG` 규칙 |
| annotation `SUBSTANTIVE`/`FORMAL` | morph boundary + POS 규칙 |

---

## 2. Morph Head

### 목적

**MeCab 없이** 형태소 경계와 POS를 예측한다.
phonology_kr.py의 연음/경음화/조사 규칙이 형태소 경계와 POS에 의존하므로 핵심 head.

### 모델

| 항목 | 값 |
|------|-----|
| ONNX | [morph_head_trie.onnx](file:///c:/work/snap/snap_py/weights/ko/morph_head_trie.onnx) (2.5MB) |
| 구조 | BERT hidden + trie 사전 feature → 76cls (BIO-POS) |
| 학습 데이터 | **121,972건** ([morph_gold_train.jsonl](file:///c:/work/snap/snap_py/scripts/data/morph/morph_gold_train.jsonl)) |
| 클래스 | 76개 (BIO × POS 조합) |
| **Val BIO** | **93.5%** |
| **Val POS** | **94.7%** |

### 활용

- `JKG` POS → 조사 '의'→'에' 변환 (`apply_josa_ui`)
- morph boundary + 실질형태소 POS → 연음 경계 판단 (`apply_liaison`)
- 어미 `ETM` → 관형형 뒤 경음화 판단

---

## 3. Semiotic Head

### 목적

숫자 패턴의 **의미를 분류**하여 올바른 읽기를 결정한다. 2단계 구조:
- **Phase 1 (룰)**: 패턴 자체로 확정 가능한 것 (전화번호, IP주소)
- **Phase 2 (neural)**: 문맥이 의미를 결정하는 것 (시간/날짜/점수 등)

### 모델

| 항목 | 값 |
|------|-----|
| ONNX | [semiotic_head.onnx](file:///c:/work/snap/snap_py/weights/ko/semiotic_head.onnx) |
| 구조 | Frozen BERT → MLP (768→64→ReLU→6) |
| 학습 데이터 | **2,151건** |
| 클래스 | 6개: time, date, score, ratio, verse, fraction |
| **Val Acc** | **100%** |
| **Eval Acc** | **100%** (108건) |

---

## 4. Number (룰 기반)

### 목적

숫자를 **고유어(native)** vs **한자어(sino)**로 읽을지 판별한다.

### 구현

**Neural head가 아닌 룰 기반 단위 사전**으로 구현. 단위가 읽기 방식을 결정하므로 BERT 불필요.

| 항목 | 값 |
|------|-----|
| 방식 | 단위 suffix 사전 lookup |
| ONNX | **불필요** (제거됨) |
| **Eval Acc** | **100%** (239/239) |

### 주요 룰

| 규칙 | 예시 |
|------|------|
| native 단위 (개,명,살,마리,잔,병,대,시 등 27종) | "3살" → native |
| sino 단위 (층,호,년,월,일,분,초,원,세 등 23종) | "3층" → sino |
| **"제" 접두사** → sino 강제 | "제15대" → sino |
| **100 이상 숫자** → sino 강제 | "523명" → sino |

---

## 5. Heteronym Head

### 목적

동철이음이의어 20개 단어에 대해 **경음화(TENS) 여부**를 판별한다.
3계층 구조: **always_tens 룰 → 조사 룰 → neural**.

### 처리 흐름

```
단어 매칭
  │
  ├─ always_tens? ──→ 무조건 TENS (고가/전과/물질/불법/볼거리)
  │
  ├─ 잠자리 + "에"? ─→ 무조건 TENS
  │
  └─ neural head ───→ MLP 분류 (나머지 15개 단어)
       │
       └─ fallback ──→ default 값 (head 없을 때)
```

### 타겟 단어 (20개)

[heteronym_targets.json](file:///c:/work/snap/snap_py/weights/ko/heteronym_targets.json) 기준:

| 단어 | Default | always_tens | 의미 (TENS / NONE) |
|------|:-------:|:-----------:|-------------------|
| 고가 | TENS | ✅ | 高價 / 高架 |
| 전과 | TENS | ✅ | 轉科 / 前科 |
| 물질 | TENS | ✅ | 物質 / 해녀잠수 |
| 불법 | TENS | ✅ | 不法 / 佛法 |
| 감기 | NONE | | 감는 행위 / 질병 |
| 대가 | TENS | | 代價 / 大家 |
| 문과 | TENS | | 인문계열 / 과거분과 |
| 병적 | NONE | | 兵籍 / 病的 |
| 볼거리 | TENS | ✅ | 구경거리 / 질병 |
| 성적 | NONE | | 成績 / 性的 |
| 송장 | NONE | | 시체 / 送狀 |
| 시가 | TENS | | 市價 / 詩歌 |
| 안다 | NONE | | 알다 / 껴안다 |
| 열병 | TENS | | 熱病 / 閱兵 |
| 영장 | TENS | | 令狀 / 靈長 |
| 외과 | TENS | | 外科 / 外踝 |
| 원장 | NONE | | 院長 / 原狀 |
| 잠자리 | NONE | | 곤충 / 잠자는곳 |
| 제법 | NONE | | 부사 / 製法 |
| 지적 | NONE | | 指摘 / 知的 |

> [!NOTE]
> **always_tens 근거**: 고가/전과/물질/불법/볼거리는 의미가 달라도 발음(경음화)은 동일. 네이티브 스피커 판단으로 확정. 볼거리(질병=mumps)는 현대 일상에서 거의 사용되지 않으며, 사용되더라도 "볼꺼리"로 발음.

### 모델

| 항목 | 값 |
|------|-----|
| ONNX | [heteronym_head.onnx](file:///c:/work/snap/snap_py/weights/ko/heteronym_head.onnx) (194KB) |
| 구조 | Frozen BERT + MLP (768→64→ReLU→2) |
| 학습 데이터 | **1,658건** (always_tens 제외) |
| **Val Acc** | **98.0%** |

### Holdout 검증 (501건)

학습에 사용되지 않은 독립 holdout ([validation_set.jsonl](file:///c:/work/snap/snap_py/scripts/data/unified_semantic/validation_set.jsonl)).

**전체: 456/501 (91.0%)** — 룰 + neural 합산

| 단어 | 처리 | 전체 | 에러 | 정확도 |
|------|------|:----:|:----:|:------:|
| 고가 | 룰 | 70 | 0 | **100%** |
| 전과 | 룰 | 56 | 0 | **100%** |
| 물질 | 룰 | 51 | 0 | **100%** |
| 불법 | 룰 | 23 | 0 | **100%** |
| 볼거리 | 룰 | 43 | 0 | **100%** |
| 대가 | Neural | 31 | 1 | **96.8%** |
| 감기 | Neural | 81 | 2 | **97.5%** |
| 잠자리 | Neural | 53 | 5 | **90.6%** |
| **안다** | Neural | 86 | **30** | **65.1%** |

> [!WARNING]
> **에러 38건 중 안다 30건 (79%)**. "~을/를 안다 + 동사" 구문에서 껴안다/알다를 단일 토큰 hidden state만으로 구분하기 어려움.

### 개선 이력

| 라운드 | 정확도 | 주요 변경 |
|:------:|:------:|----------|
| 최초 | 81.2% | 1,943건 학습 |
| +보강 | 83.8% | 전과/고가/잠자리 데이터 추가 |
| +잠자리에 룰 | 85.8% | "잠자리에" → TENS 룰 |
| +전과/고가 보강 | 86.6% | +35건 augment |
| +always_tens 룰 | 90.8% | 고가/전과/물질/불법 룰 확정 |
| +볼거리 always_tens | **91.0%** | 볼거리 룰 추가 (현재) |

---

## 6. Counter Head

### 목적

숫자 + 의존명사(`'번'`, `'대'`, `'장'`, `'동'`) 조합에서 **고유어(native) vs 한자어(sino)** 읽기를 판별한다.

### 처리 흐름

```
숫자 + 의존명사 ("번", "대", "장", "동") 패턴
  │
  ├─ "번째"? ───→ 100 미만: native, 100 이상: sino (룰)
  │
  └─ neural ───→ MLP 분류 (model_counter.onnx)
```

### 모델

| 항목 | 값 |
|------|-----|
| ONNX | [model_counter.onnx](file:///c:/work/snap/snap_py/weights/ko/model_counter.onnx) (194KB) |
| 구조 | Frozen BERT + MLP (768→64→ReLU→2) |
| 학습 데이터 | **2,575건** (번: 783건, 대: 603건, 장: 595건, 동: 594건) |
| **Val Acc** | **98.5%** |
| **E2E Eval** | **100%** (test_final.py의 counter 테스트 4/4) |

---

## 7. Vowel Length (사전 기반)

| 항목 | 값 |
|------|-----|
| 방식 | 사전 lookup (`vowel_length_long_dict.json`) |
| **정확도** | **98.74%** |
| Eval | 112건 |

---

## 8. 전체 성능 요약

### Neural Head (BERT hidden state 사용)

| Head | ONNX | 구조 | 학습 데이터 | Val Acc | 독립 Eval |
|------|------|------|----------:|:-------:|:---------:|
| **morph** | `morph_head_trie.onnx` | 76cls | 121,972 | BIO 93.5% / POS 94.7% | — |
| **semiotic** | `semiotic_head.onnx` | 768→64→6 | 2,151 | **100%** | **100%** (108건) |
| **heteronym** | `heteronym_head.onnx` | 768→64→2 | 1,658 | **98.0%** | **91.0%** (501건, 룰 포함) |
| **counter** | `model_counter.onnx` | 768→64→2 | 2,575 | **98.5%** | **100%** (test_final.py) |

### 룰 기반 (BERT 불필요)

| 모듈 | 방식 | Eval |
|------|------|:----:|
| **number** | 단위 사전 + "제" 접두사 + 100이상 규칙 | **100%** (239건) |
| **vowel_length** | bigram 사전 lookup | **98.74%** (112건) |
| **phone/ip** | 정규식 매칭 → 고정 label | — |
| **always_tens** | heteronym_targets.json 플래그 | **100%** (243건) |
| **잠자리에** | 조사 "에" 패턴 | **100%** |
| **번째** | "번째" suffix → native 고정 | **100%** |

### 통합 테스트

| 테스트 | 건수 | 정확도 |
|--------|-----:|:------:|
| 수동 검증 (기능별) | 36 | **100%** |
| number eval | 239 | **100%** |
| semiotic eval | 108 | **100%** |
| corpus smoke | 3,116 | **100%** |
| **총합** | **3,499** | **100%** |

---

## 9. 관련 파일

### 모델
| 파일 | 크기 | 설명 |
|------|-----:|------|
| [morph_head_trie.onnx](file:///c:/work/snap/snap_py/weights/ko/morph_head_trie.onnx) | 2.5MB | 형태소 분석 |
| [semiotic_head.onnx](file:///c:/work/snap/snap_py/weights/ko/semiotic_head.onnx) | — | 기호 패턴 분류 |
| [heteronym_head.onnx](file:///c:/work/snap/snap_py/weights/ko/heteronym_head.onnx) | 194KB | 경음화 판별 |
| [model_counter.onnx](file:///c:/work/snap/snap_py/weights/ko/model_counter.onnx) | 194KB | 번/대/장/동 단위 읽기 |

### 학습 데이터
| 파일 | 건수 |
|------|-----:|
| [morph_gold_train.jsonl](file:///c:/work/snap/snap_py/scripts/data/morph/morph_gold_train.jsonl) | 121,972 |
| [semiotic_train.jsonl](file:///c:/work/snap/snap_py/scripts/data/semiotic/semiotic_train.jsonl) | 2,151 |
| [heteronym_train.jsonl](file:///c:/work/snap/snap_py/scripts/data/unified_semantic/heteronym_train.jsonl) | 1,658 |
| [unit_extracted.jsonl](file:///c:/work/snap/snap_py/scripts/data/unified_semantic/unit_extracted.jsonl) | 2,575 |
| [number_v7_train.jsonl](file:///c:/work/snap/snap_py/scripts/data/number/number_v7_train.jsonl) | 2,903 |

### 검증 데이터
| 파일 | 건수 | 용도 |
|------|-----:|------|
| [validation_set.jsonl](file:///c:/work/snap/snap_py/scripts/data/unified_semantic/validation_set.jsonl) | 501 | heteronym 독립 holdout |
| [heteronym_eval.jsonl](file:///c:/work/snap/snap_py/scripts/data/unified_semantic/heteronym_eval.jsonl) | 501 | heteronym eval 결과 |

### 주요 스크립트
| 파일 | 용도 |
|------|------|
| [train_semantic_heads.py](file:///c:/work/snap/snap_py/scripts/train_semantic_heads.py) | heteronym/counter 학습 |
| [eval_independent.py](file:///c:/work/snap/snap_py/scripts/eval_independent.py) | 독립 holdout 검증 |
| [test_final.py](file:///c:/work/snap/snap_py/scripts/test_final.py) | 3,499건 통합 테스트 |
| [train_morph_trie.py](file:///c:/work/snap/snap_py/scripts/train_morph_trie.py) | morph head 학습 |
| [rebench_snap.py](file:///c:/work/snap/snap_py/scripts/rebench_snap.py) | phonology_kr 변경 후 전체 파이프라인 재벤치마크 |

---

## 10. phonology_kr 버그픽스 성능 현황 (2026-06-01)

> 상세 기록: [05_experiment_log.md](file:///c:/work/snap/docs/ko/05_experiment_log.md)

### 수정 내역 (4건, phonology_kr.py)

| # | 수정 | 표준발음법 | 효과 |
|:---:|------|:--------:|------|
| 1 | ㅎ 격음화 대상 받침 추가 (ㅅ,ㅆ,ㅊ,ㅌ) | 제23조 | 못해→모태, 비슷하게→비스타게 |
| 2 | 어절 경계 연음 차단 | 제13조 | 쓰는 옷이잖아→쓰는 오시자나 |
| 3 | `의` 어절 첫음절 보호 | 제5항 | 의미→이미 오류 수정 |
| 4 | 어절 경계 ㄴ첨가 차단 | 제29조 | 심 명 니상→이상 |

### 벤치마크 결과 (SXMP 구어체 10,000건)

| 지표 | 수정 전 | **수정 후** | g2pk |
|------|:-------:|:-----------:|:----:|
| avg CER | 11.67% | **9.17%** | 11.85% |
| SNAP Win vs g2pk | 9.5% | **34.4%** | — |
| g2pk Win | 8.3% | **7.5%** | — |
| 개선:퇴보 | — | **46:1** | — |
