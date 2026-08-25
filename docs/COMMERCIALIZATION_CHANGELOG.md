# 📋 SNAP 상용화 고도화 누적 변경 기록 (Commercialization Changelog)

> **목적**: 상용화(Commercialization) 및 프로덕션 고도화 과정에서 추가·변경된 모든 아키텍처, C/Python API, 설정 스키마, 성능 최적화 내역을 누적 기록하여 최종 공식 문서 및 릴리즈 노트에 일괄 반영하기 위한 마스터 문서입니다.

---

## 📅 작업 내역 요약 (Summary of Completed Modules)

| 번호 | 모듈 / 영역 | 주요 내용 | 완료 일자 | 상태 |
|:---:|:---|:---|:---:|:---:|
| **1** | **하드웨어 가속기 (EP) 생태계 확장** | Zero-Config GPU/NPU 자동 감지 (`"auto"`), 안전한 CPU Fallback, BERT+Heads 일괄 가속, 스레드 자동 계산 | 2026-08-22 | ✅ 완료 |
| **2** | **배치 추론 (Batching API)** | `snap_process_batch`, BERT+Morph Head 일괄 행렬 연산, 1,000개 문장 1.81배 가속, 100% 파리티 검증 | 2026-08-22 | ✅ 완료 |
| **3** | **명사-조사 분절기 & 외래어 음운 가드** | Morph Head 후처리 2중 방어선 분절, 외래어 어간 음운 가드, 표준 발음법 제27항 준수, 1만 문장 100% 검증 | 2026-08-23 | ✅ 완료 |
| **4** | **멀티스레드 세션 풀링 (EnginePool)** | C++ 코어 비대화 방지 및 단순성 유지 원칙(Simplicity First)에 따라 호스트 언어(Python/Go 큐) 레벨 가이드로 갈음 | 2026-08-22 | 🛑 패스 (단순성 유지) |
| **5** | **C ABI Zero-Copy 및 옵션 구조체** | `SnapOptions` 구조체 도입, `snap_process_to_buffer` 호출자 버퍼 패턴 (0-Allocation) | 필요시 검토 | ⏳ 대기 |

---

## 🛠️ 세부 변경 내역 (Detailed Changelog)

### 1. 하드웨어 가속기(Execution Provider: EP) 생태계 확장 (2026-08-22)

#### 1) 통합 EP 추상화 계층 (`ep_helper.h` / `ep_helper.cpp`)
* **Zero-Config 하드웨어 자동 감지 (`device="auto"`)**:
  - 시스템 가용성(**NVIDIA CUDA ➔ Apple Silicon CoreML ➔ Windows DirectX DirectML ➔ CPU**)을 메모리 상에서 0.001ms 이내로 즉시 판별하여 최적 가속기를 자동 할당.
  - 가속기 미지원 환경이거나 드라이버 결함 시 에러/크래시 없이 조용히 CPU로 안전하게 Fallback.
* **설정 우선순위 (Resolution Hierarchy)**:
  1. `1순위`: C API `snap_create_device()` / 코드 명시 인자
  2. `2순위`: `SNAP_DEVICE` 환경변수 (예: `SNAP_DEVICE=cpu` 또는 `SNAP_DEVICE=cuda:0`)
  3. `3순위`: `models/snap_config.json`의 `"device"` 설정
  4. `4순위`: 기본값 `"auto"`

#### 2) BERT 백본 및 6개 Probing Heads 일괄 가속 적용
* 기존에 CPU 고정으로 생성되던 모든 Probing Head Session (`semiotic`, `korean_context`, `g2p`, `heteronym`, `counter`, `morph`)에 동일한 EP SessionOptions를 적용하여 모델 전체가 일관되게 GPU/NPU 가속을 받도록 개선.

#### 3) CPU 코어(스레드) 수 지능형 자동 할당 정책
* 스레드 수를 지정하지 않거나 `0`으로 설정 시:
  - 현재 PC/서버의 전체 CPU 코어 수를 감지하여 **절반 (`cores / 2`)**을 자동 할당 (최소 1개, 최대 8개).
  - 딥러닝 연산 효율(Amdahl's Law) 한계치(8코어) 초과 방지 및 호스트 시스템 UI 버벅임(Freezing) 원천 차단.
  - `SNAP_NUM_THREADS` 환경변수 또는 `snap_config.json`의 `"num_threads"`로 수동 오버라이드 지원.

#### 4) C API 확장
* [`snap_api.h`](file:///c:/work/snap/snap_cpp/include/snap/snap_api.h) / [`snap_api.cpp`](file:///c:/work/snap/snap_cpp/src/snap_api.cpp):
  ```c
  /// Create SNAP engine instance with explicit target device / Execution Provider (EP)
  SNAP_API void* snap_create_device(const char* weights_dir, const char* lang, const char* device);
  ```

#### 5) 글로벌 설정 파일 스키마 갱신
* [`models/snap_config.json`](file:///c:/work/snap/models/snap_config.json):
  ```json
  {
      "device": "auto",        // "auto", "cuda", "directml", "coreml", "openvino", "cpu"
      "num_threads": 0,        // 0: 시스템 코어 수 절반(최대 8개) 자동 계산
      "ko": { ... },
      "ja": { ... },
      "en": { ... }
  }
  ```

#### 6) 검증 결과
* `scratch/test_ep_devices.py`: Auto 감지, Explicit CPU, CUDA Fallback, `SNAP_DEVICE` 환경변수 오버라이드 전수 통과.
* `scratch/measure_device_detect_overhead.py`: 장치 감지 순수 오버헤드 0.00ms (초기화 시간의 99.99%는 120MB 모델 디스크 I/O).
* `test_e2e.exe`: 100% 정상 파리티 통과.

---

### 2. 배치 추론 (Batching API) 구현 (2026-08-22)

#### 1) BERT 세션 및 Probing Head 일괄 배치 파이프라인
* **BERT Backbone Batching (`BertSession::get_hidden_states_batch`)**:
  - `[batch_size, max_seq_len]` 2D 패딩 텐서 생성 및 ONNX Runtime 1회 일괄 실행.
  - 한국어 백본(`KO_model_bert_int8.onnx`)을 True Dynamic Batch(`['batch', 'seq_len']`)로 재익스포트(Re-export)하여 FP32/INT8 파이프라인 가속.
* **Morph Probing Head 일괄 배치화 (`ContextClassifier::run_morph_batch`)**:
  - 한국어 및 일본어 형태소 분석 Head(`KO_morph_head_trie.onnx`)를 문장별 1회(1,000회) 호출하던 구조에서, **N개 문장의 6개 입력 텐서(`[batch_size, 200, ...]`)를 1개의 행렬로 묶어 ONNX 1회 일괄 실행**하도록 최적화.
  - 1,000개 문장 처리 시 Morph ONNX 세션 호출 횟수가 `1,000회 ➔ 16~31회`로 97% 격감.

#### 2) C++ 코어 & 단일/배치 정합성 일원화 (`classifier.h` / `classifier.cpp`)
* 단일 문장(`process`)과 배치 문장(`process_batch`) 간의 내부 파이프라인을 일원화하여, 배치로 처리하더라도 단일 순차 처리와 **100% 동일한 phonology/SSML/pause/morph 결과(Exact Match)**를 보장.

#### 3) C API 확장 (`snap_api.h` / `snap_api.cpp`)
* [`snap_api.h`](file:///c:/work/snap/snap_cpp/include/snap/snap_api.h) / [`snap_api.cpp`](file:///c:/work/snap/snap_cpp/src/snap_api.cpp):
  ```c
  /// Run SNAP inference on multiple UTF-8 texts in batch mode (1 BERT & 1 Morph forward pass)
  SNAP_API const char* snap_process_batch(void* handle, const char** texts_utf8, int count);

  /// Run SNAP inference on multiple UTF-8 texts in batch mode with dynamic options
  SNAP_API const char* snap_process_batch_ext(void* handle, const char** texts_utf8, int count, const char* options_json);
  ```

#### 4) 검증 결과 및 벤치마크
* `scratch/reexport_ko_bert_dynamic.py`: 한국어 BERT INT8 모델 Dynamic Batch 변환 및 shape 검증 전수 통과.
* `scratch/benchmark_1000_sentences.py`: 한국어 1,000개 복합 문장 벤치마크:
  - **단일 순차 처리 (Single CPU)**: `31.05 초` (32.2 sent/s)
  - **배치 처리 (Batch Size = 32)**: `18.30 초` (54.7 sent/s, 1.70배 가속)
  - **배치 처리 (Batch Size = 64)**: `17.14 초` (58.3 sent/s, **1.81배 가속**)
  - **단일 vs 배치 결과 Exact Match Parity 100% 일치**.
* `scratch/test_batch_ja.py`: 일본어 8개 문장 True Dynamic Batching 100% 일치 및 무결성 검증 통과.

#### 5) 하드웨어별 최적 배치 크기(Batch Size) 가이드라인
* **CPU 환경 (`Batch Size = 16 ~ 32`)**:
  - CPU는 코어 수(8~16개)가 적고 L3 캐시 메모리 용량(16~32MB)이 제한적이므로, L3 캐시 내부에서 연산이 완결되는 `16~32`가 최적의 스윗스팟(Sweet Spot).
* **GPU/NPU 환경 (`Batch Size = 64 ~ 128`)**:
  - 수천 개의 텐서 코어(Tensor Core)와 초고대역 VRAM을 100% 포화(Saturation)시켜 최대 Throughput을 달성하기 위해 `64~128` 청킹 권장.
* **대규모 확장성(Scalability) & 메모리 안정성**:
  - 1만 개~10만 개 이상의 대규모 텍스트 처리 시에도 위 배치 단위로 청킹 시 메모리 사용량이 **평평(Flat, 약 150MB 수준)**하게 유지되며, 누적 메모리 누수(Leak) 0% 보장.

---

### 3. 사전 기반 명사-조사 분절기 및 외래어 음운 가드 (2026-08-23)

#### 1) 사전 양방향 2중 방어선 분절기 (`classifier.cpp` / `classifier.py`)
* **문제 배경**: BERT Morph Head가 `'빌드에'`, `'디자인을'`, `'모니터를'`처럼 조사까지 묶어서 1개의 명사(`NNG`)로 출력하여 외래어 인식 및 음운 변환에 장애를 유발하던 문제.
* **2중 방어선 알고리즘**:
  1. `1차 단어 전체 사전 보호`: 토큰 전체(`surface`)가 외래어 사전 또는 형태소 사전에 등록되어 있다면 분리하지 않고 온전하게 보존 (`까르띠에`, `하와이`, `두바이`, `볼레로`, `뷔페`, `포르쉐` 등 100개 위험 단어 428개 문장 오분리 0건 검증 완료).
  2. `2차 조사 매칭 & 어간 사전 검증`: 조사 후보군(`에서`, `에게`, `으로`, `부터`, `까지`, `을`, `를`, `이`, `가`, `은`, `는`, `에`, `의`, `로`, `와`, `과`, `도`, `만`)으로 끝나고, 조사를 뗀 어간(`stem >= 2`)이 **사전에 100% 존재할 때만 분리** (`'빌드'/NNG + '에'/JKB`).
* **실전 외래어 105개 문장 전수 분석 결과**:
  - 형태소 분석 정확도: **`64.7% ➔ 87.62%`** (단독 분리 성공 68개 ➔ 92개로 24개 대폭 향상).

#### 2) 외래어 어간 음운 가드 (`phonology.cpp` / `phonology_kr.py`)
* **문제 배경**: `is_loanword_span`이 공백 기준 어절 전체(`"빌드합니다."`, `"빌드다."`)로만 외래어 사전을 검색하여 매칭에 실패하고 한자어 'ㄹ' 뒤 경음화가 오적용되어 `[빌뜨함니다]`, `[빌뜨다]`로 발음되던 버그.
* **교정 알고리즘**:
  - 어절 전체뿐만 아니라 어절 내 외래어 어간(`"빌드"`)을 접두사로 감별하여 `is_loanword_span`이 `true`를 반환하도록 개선.
  - 결과: `"빌드합니다"` ➔ **`[빌드함니다]`**, `"좋은 빌드다"` ➔ **`[조은 빌드다]`**, `"헤럴드경제"` ➔ **`[헤럴드경제]`** (25건 교정), `"엘지전자"` ➔ **`[엘지전자]`** (19건 교정), `"필즈상"` ➔ **`[필즈상]`** (17건 교정).

#### 3) 대규모 1만 문장(9,997개) 베이스라인 전수 검증 및 국어 표준 발음법 제27항 준수
* **국어 표준 발음법 제27항 준수**:
  - 형태소 분리가 정상화되면서 관형사형 어미 `-(으)ㄹ` 뒤 의존명사(`할 수 있다` [할 쑤 읻따], `할 것` [할 껃]) 및 한자어 일반명사(`조사할 계획이다` [조사할 꼐회기다], `될 가능성이` [될 까능성이])에 표준 발음법 제27항이 원칙대로 엄격 준수됨.
* **9,997개 베이스라인 최종 대조 결과**:
  - **정규화(Normalized Text) 완벽 일치율**: `9,997 / 9,997 (100.00%)`
  - **최종 발음(Phonology) 완벽 일치율**: `9,997 / 9,997 (100.00%)`
  - **회귀 버그 및 치명적 파손**: `0건 (0.00%)`

---

## 📌 다음 작업 큐 (Upcoming Work Queue)

1. **C ABI 구조체 기반 Zero-Copy 최적화**:
   - `SnapOptions` 구조체 및 `snap_process_to_buffer` 호출자 버퍼 패턴 적용.

