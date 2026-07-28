---
name: snap-hf-management
description: SNAP 모델 및 발음 사전의 Hugging Face 저장소 버저닝, 독립적 사전 업데이트, 원자적 롤백 및 파이썬 유틸리티 실행 지침 가이드
---

# SNAP Hugging Face 저장소 & 버전 관리 스킬 가이드

본 스킬 가이드는 SNAP 프로젝트의 대용량 ONNX 모델, 발음/역정규화 사전, 룰셋 자산을 Hugging Face Hub에서 효율적이고 안전하게 관리하기 위한 표준 운영 가이드(SOP)입니다.

---

## 1. 핵심 아키텍처 규약

1. **사전 독립 버전 관리 (Independent Lexicon Versioning)**: 사전 단어 수정과 ONNX 모델 재학습의 버전을 분리하여, 모델 재학습 없이 사전만 1초 만에 업데이트 및 롤백 가능.
2. **다차원 모델 파생 관리 (Multi-Variant Management)**: KcBERT, RoBERTa 등 백본 아키텍처 및 INT8/FP16 양자화 모델을 `model_variants/` 하위로 병렬 관리.
3. **중앙 매니페스트 앵커링 (Central Manifest Anchoring)**: `models/manifest.json`을 통해 활성 버전(`active_version`)을 중앙에서 통제.
4. **Git LFS 해시 중복 제거 (Hash Deduplication)**: 동일 파일 중복 업로드 시 물리 용량 증가 0 byte 보장.

---

## 2. Hugging Face 저장소 디렉터리 구조

```
huggingface.co/snap-libs/snap-models/
├── manifest.json                             # 최상위 중앙 버전 컨트롤 매니페스트
├── README.md                                 # Model Card 및 사용 안내
│
├── ko/                                       # 🇰🇷 한국어 자산
│   ├── dictionaries/                         # 📚 공유 사전 (독립 버전 관리)
│   │   ├── v1.0.0/
│   │   ├── v1.1.0/                           # 🆕 사전 단어 추가 버전
│   │   │   ├── dict_eng_merged.json
│   │   │   ├── heteronym.json
│   │   │   └── rule_custom.json
│   │   └── version_manifest.json
│   │
│   └── model_variants/                       # 🤖 모델 백본 & 양자화 파생
│       ├── kcbert-base-int8/                 # [Variant A] 기본 추천 모델 (KcBERT INT8)
│       │   └── v1.0.0/
│       │       ├── KR_model_bert_int8.onnx
│       │       └── vocab.txt
│       └── roberta-small-fp16/               # [Variant B] 경량 모델 (RoBERTa FP16)
│           └── v1.0.0/
```

---

## 3. 중앙 매니페스트 (`manifest.json`) 명세

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
    }
  }
}
```

---

## 4. 버전 업데이트 및 롤백 스크립트 실행법

### 4.1 자동화 스크립트 (`scripts/update_version.py`)
```bash
# 한국어 사전 버전을 v1.2.0으로 1초 스위칭
python scripts/update_version.py --lang ko --type dict --version v1.2.0

# 한국어 모델 버전을 v1.1.0으로 스위칭
python scripts/update_version.py --lang ko --type model --version v1.1.0
```

### 4.2 1초 원자적 롤백 방법
이전 버전으로 복구하려면 `models/manifest.json`에서 `"active_dict_version": "v1.0.0"`으로 숫자만 변경하면 1초 내로 복구됩니다.
