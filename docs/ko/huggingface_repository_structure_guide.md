# SNAP Hugging Face 저장소 최종 디렉터리 및 버전 관리 표준 구성안

본 문서는 SNAP 프로젝트의 대용량 ONNX 모델, 사전, 룰셋 자산을 Hugging Face Hub에서 효율적이고 안전하게 관리하기 위한 **최종 아키텍처 구성안**입니다.

---

## 1. 핵심 설계 원칙

1. **독립적 사전 버저닝 (Independent Lexicon Versioning)**: 사전 단어 추가(몇 KB~MB)와 ONNX 모델 재학습(수백 MB)의 버전을 분리하여 모델 재학습 없이도 사전만 1초 만에 업데이트 및 롤백 가능.
2. **다차원 모델 파생 관리 (Multi-Variant Management)**: KcBERT, RoBERTa 등 다양한 BERT 백본 및 INT8/FP16 양자화 파생 모델을 디렉터리 충돌 없이 병렬 관리.
3. **중앙 매니페스트 앵커링 (Central Manifest Anchoring)**: `manifest.json`을 통해 활성 버전(`active_version`)을 중앙에서 통제하여 런타임 안정성 및 1초 롤백 보장.
4. **Git LFS 해시 중복 제거 (Deduplication)**: 파일 내용 기반 SHA256 관리를 통해 동일 파일 중복 업로드 시 저장소 실제 용량 증가 0 byte 구현.

---

## 2. 최종 디렉터리 구조 (Hugging Face Repository Layout)

```
huggingface.co/snap-libs/snap-models/
├── manifest.json                             # 👈 1. 저장소 최상위 중앙 버전 컨트롤 매니페스트
├── README.md                                 # Hugging Face Model Card 및 사용법
│
├── ko/                                       # 🇰🇷 한국어 자산
│   ├── dictionaries/                         # 📚 2. 공유 사전 (독립 버전 관리)
│   │   ├── v1.0.0/                           # 릴리즈 초기 사전
│   │   │   ├── dict_eng_merged.json
│   │   │   ├── heteronym.json
│   │   │   └── rule_custom.json
│   │   ├── v1.1.0/                           # 🆕 단어 1,000개 추가된 사전
│   │   │   ├── dict_eng_merged.json
│   │   │   ├── heteronym.json
│   │   │   └── rule_custom.json
│   │   └── version_manifest.json
│   │
│   └── model_variants/                       # 🤖 3. 모델 백본 & 양자화 파생 폴더
│       ├── kcbert-base-int8/                 # [Variant A] 기본 추천 모델 (KcBERT INT8)
│       │   ├── v1.0.0/
│       │   │   ├── KR_model_bert_int8.onnx
│       │   │   ├── KR_number_classifier.onnx
│       │   │   ├── KR_semiotic_head.onnx
│       │   │   ├── tokenizer.json
│       │   │   └── vocab.txt
│       │   └── v1.1.0/
│       │       └── KR_number_classifier.onnx # (개선된 수사 분류기 헤더)
│       │
│       └── roberta-small-fp16/               # [Variant B] 경량 모델 (RoBERTa FP16)
│           └── v1.0.0/
│               ├── RoBERTa_model_fp16.onnx
│               └── tokenizer.json
│
├── ja/                                       # 🇯🇵 일본어 자산
│   ├── dictionaries/
│   │   └── v1.0.0/
│   └── model_variants/
│       └── ja-kanji-bert-int8/
│           └── v1.0.0/
│
└── en/                                       # 🇺🇸 영어 자산
    ├── dictionaries/
    │   └── v1.0.0/
    └── model_variants/
        └── en-bert-base-int8/
            └── v1.0.0/
```

---

## 3. 매니페스트 파일 명세

### 3.1 최상위 중앙 매니페스트 (`manifest.json`)

```json
{
  "schema_version": "1.0.0",
  "min_engine_version": "1.0.0",
  "updated_at": "2026-07-28",
  "languages": {
    "ko": {
      "active_dict_version": "v1.1.0",
      "active_model_variant": "kcbert-base-int8",
      "active_model_version": "v1.0.0"
    },
    "ja": {
      "active_dict_version": "v1.0.0",
      "active_model_variant": "ja-kanji-bert-int8",
      "active_model_version": "v1.0.0"
    },
    "en": {
      "active_dict_version": "v1.0.0",
      "active_model_variant": "en-bert-base-int8",
      "active_model_version": "v1.0.0"
    }
  }
}
```

---

## 4. C++ / Python SDK 스마트 버전 로딩 메커니즘

### C++ API 예시
```cpp
// 1. 기본 사용 (manifest.json의 active_dict_version & active_model_variant 자동 감지)
void* handle = snap_create("./models", "ko");

// 2. 특정 모델 Variant 지정 사용
void* handle = snap_create_with_variant("./models", "ko", "roberta-small-fp16");
```

---

## 5. 작업 시나리오별 관리 가이드

| 상황 | 실행 방법 | 소요 시간 | 비고 |
| :--- | :--- | :---: | :--- |
| **사전 단어 1,000개 추가** | `dictionaries/v1.2.0/` 폴더 생성 ➔ `manifest.json` 의 `active_dict_version`을 `v1.2.0`으로 변경 | **1분** | ONNX 모델 재학습 필요 없음 |
| **사전 오류로 롤백** | `manifest.json` 의 `active_dict_version`을 `v1.1.0`으로 변경 | **1초** | 즉시 복구 완료 |
| **새로운 BERT 백본 추가** | `model_variants/roberta-small-fp16/v1.0.0/` 폴더 생성 ➔ 업로드 | **5분** | 기존 KcBERT 사용자에 영향 없음 |
