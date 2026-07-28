# SNAP 버전 관리 베스트 프랙티스 (Version Management Best Practices)

> **작성일**: 2026-07-28  
> **적용 대상**: SNAP 모델, 발음/역정규화 사전, C++/Python SDK

---

## 1. 현재 관리 방식의 4대 문제점과 해결책

| 기존 문제점 | 새로운 해결책 (Best Practice) | 효과 |
| :--- | :--- | :--- |
| **1. 버전 메타데이터 부재** | `manifest.json` 및 `version_manifest.json` 구조 도입 | 모델/사전의 출처와 정합성 100% 추적 |
| **2. 비일관적 임시 백업** (`.bak`, `_dirty`) | 버전 디렉터리(`v1.0.0/`, `v1.1.0/`) 기반의 엄격한 분리 | 백업 파일에 의한 오염 완전 차단 |
| **3. 원자적 롤백 불가능** | `manifest.json` 활성 버전 앵커링 스위칭 | 오류 발생 시 **1초 내 롤백** |
| **4. 사전/모델 결합 종속성** | **사전(Lexicon)**과 **모델(Variant)**의 독립 버전 제어 | 모델 재학습 없이 **1분 내 사전 반영** |

---

## 2. 의미적 버전 관리 (Semantic Versioning) 규약

```
vMAJOR.MINOR.PATCH (예: v2.1.0)
- MAJOR (v2.0.0): 모델 백본 아키텍처 변경 또는 C++ API 파괴적 변경
- MINOR (v2.1.0): 사전 대규모 단어 추가, 수사 파서 기능 추가 (하위 호환)
- PATCH (v2.1.1): 오독 긴급 수정, 예외 발음 1~2개 핀포인트 수정
```

---

## 3. 작업 시나리오별 표준 워크플로우

### 시나리오 1: 사전(dict_eng_merged.json 등) 단어 추가/수정
1. `python scripts/update_version.py --type dict --lang ko --version v1.2.0` 실행
2. 자동으로 `models/ko/dictionaries/v1.2.0/` 폴더가 생성되고 SHA256 체크섬이 기록됨.
3. `manifest.json`에서 `"active_dict_version": "v1.2.0"`으로 1초 만에 스위칭 및 테스트.

### 시나리오 2: 새로운 BERT 백본 모델 추가 (예: RoBERTa 경량 모델)
1. `models/ko/model_variants/roberta-small-fp16/v1.0.0/` 디렉터리에 모델 배치.
2. `manifest.json`에 `roberta-small-fp16` variant 항목 등록.
3. 기존 KcBERT 기반 사용자에 영향 없이 새로운 백본 병련 제공.

---

## 4. SHA256 무결성 검증 및 롤백 안전장치

```python
import hashlib

def verify_file_sha256(filepath, expected_hash):
    hasher = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(8192):
            hasher.update(chunk)
    computed = hasher.hexdigest()
    assert computed == expected_hash, f"File integrity check failed: {filepath}"
```
