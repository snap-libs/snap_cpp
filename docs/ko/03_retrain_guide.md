# 한국어 TTS Frontend — 재학습 재현 가이드

> BERT 모델 교체 시 이 문서를 따라 모든 neural head를 재학습한다.
> 마지막 검증: 2026-05-26, BERT: `kykim/bert-kor-base`

---

## 1. 학습 순서

> [!IMPORTANT]
> morph를 **가장 먼저** 학습해야 한다. 다른 head들의 음운규칙이 morph 결과에 의존하기 때문.
> 나머지 3개(semiotic, heteronym, beon)는 순서 무관하고 **병렬 학습 가능**.

```
Step 1: morph head        (의존성: 없음, 다른 head의 기반)
Step 2: semiotic head     (의존성: 없음)
Step 3: heteronym head    (의존성: 없음)
Step 4: beon head         (의존성: 없음)
Step 5: 검증              (test_final.py)
```

---

## 2. Head별 학습 상세

### Step 1: Morph Head (형태소 분석)

| 항목 | 값 |
|------|-----|
| 학습 스크립트 | [train_morph_trie.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/train_morph_trie.py) |
| 학습 데이터 | [morph_gold_train.jsonl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/data/morph/morph_gold_train.jsonl) (121,972건) |
| 출력 | `morph_head_trie.onnx`, `morph_label_map.json` |
| 구조 | BERT hidden (768) + trie feature → 76cls (BIO-POS) |
| 기대 정확도 | BIO ≥ 93%, POS ≥ 94% |
| 보조 파일 | `morph_trie.pkl` (trie 사전, BERT와 무관하므로 재생성 불필요) |

```bash
python snap/scripts/train_morph_trie.py
```

---

### Step 2: Semiotic Head (기호 패턴 분류)

| 항목 | 값 |
|------|-----|
| 학습 스크립트 | [train_semiotic_head.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/train_semiotic_head.py) |
| 학습 데이터 | [semiotic_train.jsonl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/data/semiotic/semiotic_train.jsonl) (2,151건) |
| 출력 | `semiotic_head.onnx`, `semiotic_label_map.json` |
| 구조 | Frozen BERT → MLP (768→64→ReLU→6) |
| 학습 방식 | standalone (BERT frozen, MLP만 학습) |
| 하이퍼파라미터 | LR=2e-4, epochs=30, batch=32, val_split=0.15 |
| 기대 정확도 | Val ≥ 97%, Eval 100% (108건) |

```bash
python snap/scripts/train_semiotic_head.py
```

---

### Step 3: Heteronym Head (동철이음이의어 경음화)

| 항목 | 값 |
|------|-----|
| 학습 스크립트 | [train_semantic_heads.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/train_semantic_heads.py) |
| 학습 데이터 | [heteronym_train.jsonl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/data/unified_semantic/heteronym_train.jsonl) (1,943건) |
| 출력 | `heteronym_head.onnx`, `heteronym_label_map.json` |
| 구조 | Frozen BERT → MLP (768→64→ReLU→2, TENS/NONE) |
| 학습 방식 | standalone (BERT frozen, MLP만 학습) |
| 하이퍼파라미터 | LR=2e-4, epochs=30, batch=32, val_split=0.15, early_stop=5 |
| 기대 정확도 | Val ≥ 98%, Holdout ≥ 94% (494건) |
| 타겟 사전 | [heteronym_targets.json](file:///c:/work/RaconVoice/RaconVoice_V6/snap/weights/ko/heteronym_targets.json) (20개 단어) |

```bash
python snap/scripts/train_semantic_heads.py --head heteronym
```

---

### Step 4: Beon Head ("번" 읽기 분류)

| 항목 | 값 |
|------|-----|
| 학습 스크립트 | [train_semantic_heads.py](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/train_semantic_heads.py) |
| 학습 데이터 | [beon_extracted.jsonl](file:///c:/work/RaconVoice/RaconVoice_V6/snap/scripts/data/unified_semantic/beon_extracted.jsonl) (770건) |
| 출력 | `beon_head.onnx`, `beon_label_map.json` |
| 구조 | Frozen BERT → MLP (768→64→ReLU→2, native/sino) |
| 학습 방식 | standalone (BERT frozen, MLP만 학습) |
| 하이퍼파라미터 | LR=2e-4, epochs=30, batch=32, val_split=0.15, early_stop=5 |
| 기대 정확도 | Val ≥ 99% |

```bash
python snap/scripts/train_semantic_heads.py --head beon
```

---

## 3. 재학습 불필요 (룰 기반, BERT와 무관)

| 모듈 | 파일 | 이유 |
|------|------|------|
| **number** | classifier.py 내 사전 | 단위 사전 lookup, ONNX 없음 |
| **vowel_length** | `vowel_length_long_dict.json` | bigram 사전 lookup |
| **phone/ip** | classifier.py 내 정규식 | 정규식 매칭 |
| **josa_ui (의→에)** | phonology_kr.py | morph POS=JKG 기반 룰 |
| **phonology 8규칙** | phonology_kr.py | 전부 룰 기반 |

---

## 4. 학습 데이터 인벤토리

### 실제 사용 (현재 학습에 사용됨)

| 파일 | 건수 | 용도 |
|------|-----:|------|
| `morph/morph_gold_train.jsonl` | 121,972 | morph head 학습 |
| `semiotic/semiotic_train.jsonl` | 2,151 | semiotic head 학습 |
| `unified_semantic/heteronym_train.jsonl` | 1,943 | heteronym head 학습 |
| `unified_semantic/beon_extracted.jsonl` | 770 | beon head 학습 |

### 검증 데이터 (Eval)

| 파일 | 건수 | 용도 |
|------|-----:|------|
| `eval/number_eval.jsonl` | 239 | 숫자 읽기 정확도 |
| `eval/semiotic_eval.jsonl` | 108 | 기호 분류 정확도 |
| `eval/tensification_eval.jsonl` | 224 | 경음화 정확도 |
| `eval/josa_ui_eval.jsonl` | 339 | 조사의 변환 정확도 |
| `eval/vowel_eval.jsonl` | 112 | 장단음 정확도 |
| `eval/liaison_eval.jsonl` | 300 | 연음 정확도 |
| `corpus/corpus_sentences.jsonl` | 3,116 | 파이프라인 안정성 (crash 없음) |
| `unified_semantic/validation_set.jsonl` | 501 | heteronym holdout 검증 |

### 레거시 (미사용, 참고용)

| 파일 | 건수 | 비고 |
|------|-----:|------|
| `korean_context/korean_context_train.jsonl` | 6,034 | ~~korean_context~~ 제거됨 |
| `number/number_train.jsonl` | 367 | number가 룰 기반으로 변경됨 |
| `number/number_v7_train.jsonl` | 2,903 | 위와 동일 |
| `tensification/tensification_train.jsonl` | 2,100 | heteronym으로 대체 |
| `morph/morph_v7_train.jsonl` | 7,371 | morph_gold로 대체 |

---

## 5. 출력 파일 (weights/ko/)

### 재학습 대상 (BERT 의존)

| 파일 | 크기 | Head |
|------|-----:|------|
| `model.onnx` | ~440MB | BERT 본체 (새 BERT로 교체) |
| `morph_head_trie.onnx` | 2.5MB | morph |
| `semiotic_head.onnx` | ~780KB | semiotic |
| `heteronym_head.onnx` | 194KB | heteronym |
| `beon_head.onnx` | 194KB | beon |

### 재학습 불필요

| 파일 | Head |
|------|------|
| `morph_trie.pkl` | morph (trie 사전) |
| `morph_label_map.json` | morph (라벨 맵) |
| `semiotic_label_map.json` | semiotic (라벨 맵) |
| `heteronym_targets.json` | heteronym (타겟 단어 20개) |
| `heteronym_label_map.json` | heteronym (라벨 맵) |
| `beon_label_map.json` | beon (라벨 맵) |
| `vowel_length_long_dict.json` | vowel_length (사전) |
| `snap_config.json` | 전체 설정 |

### 삭제 가능

| 파일 | 비고 |
|------|------|
| `number_head.onnx` | 룰 기반으로 대체, 미사용 |
| `number_label_map.json` | 위와 동일 |
| `korean_context_head.onnx` | 제거됨 |
| `korean_context_head_v2.onnx` | 제거됨 |
| `korean_context_label_map.json` | 제거됨 |
| `tensification_targets.json` | heteronym_targets.json으로 대체 |

---

## 6. 검증 절차

### 자동 검증 스크립트

```bash
# 최종 종합 테스트 (수동 36건 + eval 347건 + corpus 3,116건)
python snap/scripts/test_final.py

# 기존 정량 테스트 (tensification/number/semiotic/josa_ui/corpus)
python snap/scripts/test_pipeline_full.py
```

### 기대 결과

| 항목 | 기대 |
|------|:----:|
| Part 1 수동 검증 | **36/36 (100%)** |
| number eval | **239/239 (100%)** |
| semiotic eval | **108/108 (100%)** |
| corpus smoke | **3116/3116 (100%)** |

---

## 7. 체크리스트

```
재학습 시 체크리스트:

□ 1. 새 BERT → model.onnx 변환
□ 2. morph head 학습 → morph_head_trie.onnx
□ 3. semiotic head 학습 → semiotic_head.onnx
□ 4. heteronym head 학습 → heteronym_head.onnx
□ 5. beon head 학습 → beon_head.onnx
□ 6. test_final.py 실행 → ALL PASS 확인
□ 7. test_pipeline_full.py 실행 → 정확도 baseline 이상 확인
□ 8. 레거시 파일 삭제 (number_head.onnx, korean_context_head*.onnx)
```
