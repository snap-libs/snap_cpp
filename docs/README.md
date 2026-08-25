# SNAP TTS Frontend — 문서 색인

> 최종 업데이트: 2026-08-10 (Windows MSVC & Linux WSL Native 2대 OS 전용 컴파일 및 배포 체계 재편)

## 구조

```
snap/docs/
├── README.md
├── ko/                    ← 한국어
├── ja/                    ← 일본어
└── en/                    ← 영어 (언어 비교 등 범용 문서)
```

## 한국어 (ko/)

| # | 문서 | 내용 |
|:-:|------|------|
| 01 | [mecab_migration](ko/01_mecab_migration.md) | MeCab → morph_head 전환 기록 |
| 02 | [g2pk_comparison](ko/02_g2pk_comparison.md) | g2pk 대비 개선 분석 |
| 03 | [retrain_guide](ko/03_retrain_guide.md) | head 재학습 명령어, 데이터 경로, 검증 |
| 04 | [heads_overview](ko/04_heads_overview.md) | 전체 head 구조, 성능, 데이터 총정리 |
| 05 | [experiment_log](ko/05_experiment_log.md) | 실험 누적 기록 |
| 06 | [heteronym_errors](ko/06_heteronym_errors.md) | heteronym 89.2% 에러 전수 분석 |
| 07 | [normalize_test_report](ko/07_normalize_test_report.md) | text_normalize_kr 정규화 검증 보고서 |
| 08 | [frontend_comparison_report](ko/08_frontend_comparison_report.md) | 한국어 TTS Frontend 대량 문장 비교 및 성능 최적화 보고서 |
| 09 | [human_evaluation_report](ko/09_human_evaluation_report.md) | 불일치 표본 수동 검수 정확도 보고서 (방안 A) |
| 10 | [standalone_deployment](ko/10_standalone_deployment.md) | VITS 독립 패키지 가능성 검증 — INT8 BERT 양자화, C++ DLL 동치성/속도 벤치마크 및 일본어(JA) C++ 포팅 결과 포함 |
| 11 | [manage_dict_and_counter_integration](ko/11_manage_dict_and_counter_integration.md) | 사전 관리 및 카운터/beon 통합 |
| 12 | [counter_head_evaluation](ko/12_counter_head_evaluation.md) | 카운터 헤드 평가 결과 |
| 13 | [cpp_api_guide](ko/13_cpp_api_guide.md) | C++ DLL C API 레퍼런스 — 함수 시그니처, 파라미터/반환값 표, 메모리 관리, ctypes 연동, `snap_config.json` 설정, 성능 벤치마크, 빌드 옵션 |

## 일본어 (ja/)

| # | 문서 | 내용 |
|:-:|------|------|
| 01 | [morph_head_implementation](ja/01_morph_head_implementation.md) | morph_head 구현 계획 (MeCab 대체) |
| 02 | [morph_head_training_log](ja/02_morph_head_training_log.md) | 일본어 morph_head 학습 로그 (정확도 97.66%) |
| 03 | [kwdlc_g2p_optimization](ja/03_kwdlc_g2p_optimization.md) | KWDLC 일본어 G2P 최적화 및 평가 결과 |

## 영어 (en/)

| # | 문서 | 내용 |
|:-:|------|------|
| 01 | [morphology_by_language](en/01_morphology_by_language.md) | 세계 언어별 형태소 분석 필요성 분석 |

## 검증 데이터 (scripts/data/eval/)

### head별 검증셋 (eval/)

| 파일 | 건수 | 설명 |
|------|-----:|------|
| `heteronym_eval.jsonl` | ~1,100 | 동철이음이의어 경음화 TENS/NONE 레이블 |
| `tensification_eval.jsonl` | ~800 | 경음화 컨텍스트 검증 |
| `liaison_eval.jsonl` | ~900 | 연음 경계 검증 |
| `josa_ui_eval.jsonl` | ~1,000 | 조사 의→에 변환 검증 |
| `number_eval.jsonl` | ~400 | 숫자 sino/native 읽기 검증 |
| `semiotic_eval.jsonl` | ~200 | 기호 분류 검증 |
| `vowel_eval.jsonl` | ~150 | 장단음 검증 |
| `beon_eval.jsonl` | ~50 | 번(番) 읽기 검증 |

### E2E 파이프라인 검증셋 (eval/e2e/) ← **정식 한국어 E2E 기준**

| 파일 | 건수 | 설명 |
|------|-----:|------|
| [`deepseek_ko_9997.jsonl`](../scripts/data/eval/e2e/deepseek_ko_9997.jsonl) | **9,997** | 뉴스 코퍼스 E2E 발음 검증 (DeepSeek ground truth) |
| [`deepseek_ko_sxmp_9965.jsonl`](../scripts/data/eval/e2e/deepseek_ko_sxmp_9965.jsonl) | **10,000** | 구어/방송 E2E 발음 검증 (NIKL SXMP, DeepSeek ground truth) |

- **형식**: `{"text": "...", "reference": "발음표기..."}`
- **출처**: 뉴스 코퍼스 10,000건 × DeepSeek(`deepseek-chat`, temp=0.1)
- **평가 스크립트**: `scripts/compare_with_deepseek_ko.py`
- **최신 결과**: SNAP CER 18.76% vs g2pk 24.17% (2026-06-01)
- **상세**: [02_g2pk_comparison.md § 8](ko/02_g2pk_comparison.md)

## 원시 코퍼스 (scripts/data/corpus/)

| 폴더/파일 | 내용 | 규모 |
|-----------|------|-----:|
| `news_corpus.jsonl` | 한국 뉴스 기사 | 309,962건 |
| `nikl/NIKL_MP_v1.1_JSON.zip` | 국립국어원 모두의말뭉치 v1.1 | 신문 15만 + 구어 22만 문장 |
| `nikl/NIKL_MP_CSV.zip` | 동일 데이터 CSV 버전 | — |

- **NIKL 상세**: [nikl/README.md](../scripts/data/corpus/nikl/README.md)
- **원본 위치**: `C:\work\RaconVoice\mal\`
