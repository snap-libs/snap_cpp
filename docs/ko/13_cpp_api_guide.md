# SNAP C++ API 레퍼런스

> 작성일: 2026-06-28  
> 대상 버전: `snap_cpp` main  
> 헤더: `include/snap/snap_api.h`

---

## 목차

1. [개요](#1-개요)
2. [API 함수 레퍼런스](#2-api-함수-레퍼런스)
   - [snap_create](#snap_create)
   - [snap_process](#snap_process)
   - [snap_normalize](#snap_normalize)
   - [snap_free](#snap_free)
   - [snap_destroy](#snap_destroy)
3. [스레드 안전성 및 인스턴스 격리](#3-스레드-안전성-및-인스턴스-격리)
4. [Python ctypes 연동 가이드](#4-python-ctypes-연동-가이드)
5. [설정 레퍼런스 (`snap_config.json`)](#5-설정-레퍼런스-snap_configjson)
6. [성능 벤치마크](#6-성능-벤치마크)
7. [빌드 옵션](#7-빌드-옵션)

---

## 1. 개요

`snap_cpp.dll`은 SNAP TTS 프론트엔드 C++ 엔진을 외부 언어(Python, C#, Rust 등)에서 호출할 수 있도록 C-Linkage(`extern "C"`)로 내보내는 공유 라이브러리입니다.

### 핵심 설계 원칙

| 항목 | 내용 |
|:---|:---|
| **인터페이스** | 불투명 핸들(Opaque Handle) 기반 — 내부 C++ 타입 노출 없음 |
| **멀티 인스턴스** | `snap_create` 호출마다 독립적인 엔진 인스턴스를 힙에 생성 |
| **메모리 소유권** | `snap_process` / `snap_normalize` 반환 포인터는 호출자가 `snap_free`로 해제 |
| **스레드 안전성** | 동일 핸들에 대한 동시 `snap_process` 호출 가능 (라이프사이클 관리 제외) |

### 라이프사이클 흐름

```
snap_create()  →  snap_process() / snap_normalize()  →  snap_destroy()
                       ↓
                  snap_free(result)   ← 매 호출 후 반드시 실행
```

---

## 2. API 함수 레퍼런스

---

### `snap_create`

엔진 인스턴스를 초기화하고 핸들 포인터를 반환합니다.

```c
SNAP_API void* snap_create(const char* weights_dir, const char* lang);
```

#### 파라미터

| 이름 | 타입 | 설명 |
|:---|:---|:---|
| `weights_dir` | `const char*` | 가중치 디렉터리 경로 (UTF-8). **루트** 또는 **언어 폴더 직접** 모두 허용 — 자동 탐색. |
| `lang` | `const char*` | 언어 코드. 지원값: `"ko"`, `"ja"`, `"en"` |

#### `weights_dir` 경로 자동 탐색

`snap_create`는 아래 순서로 `snap_config.json`을 탐색합니다:

| 경로 형태 | 탐색 순서 | 예시 |
|:---|:---:|:---|
| `weights_dir/models/lang/snap_config.json` | **1순위 (표준)** | `"path/to/snap_root"` (`models/ko/` 탐색) |
| `weights_dir/lang/snap_config.json` | **2순위 (Fallback)** | `"path/to/weights"` (`weights/ko/` 탐색) |
| `weights_dir/snap_config.json` | **3순위 (직접 지정)** | `"path/to/models/ko"` (`ko/` 폴더 직접 지정) |

```c
// 방법 1: snap 루트 경로 (표준 models/ko 탐색)
void* h = snap_create("path/to/snap_root", "ko");

// 방법 2: lang 폴더 직접 지정
void* h = snap_create("path/to/models/ko", "ko");
```

> [!IMPORTANT]
> 영어 발음 사전(`dict_eng_merged.json`)을 찾을 수 없으면 `snap_create`가 `nullptr`을 반환하고  
> 에러 메시지를 stderr에 출력합니다. 사전 없이 스펠링 읽기 모드로 fallback하지 않습니다.

#### 반환값

| 값 | 의미 |
|:---|:---|
| `void*` (non-null) | 성공. 힙에 할당된 엔진 인스턴스 핸들 |
| `nullptr` | 실패. 경로 오류, 사전 누락, 또는 초기화 예외 발생 |

#### 주의사항

- 반환된 핸들은 사용 완료 후 반드시 `snap_destroy`로 해제해야 합니다.
- 동일 `weights_dir`에 대해 언어별로 독립적인 핸들을 복수 생성할 수 있습니다.

---

### `snap_process`

입력 텍스트에 대해 텍스트 정규화, BERT 추론, 분류 헤드 추론을 수행하고 결과를 JSON 문자열로 반환합니다.

```c
SNAP_API const char* snap_process(void* handle, const char* text_utf8);
```

#### 파라미터

| 이름 | 타입 | 설명 |
|:---|:---|:---|
| `handle` | `void*` | `snap_create`가 반환한 유효한 엔진 핸들 |
| `text_utf8` | `const char*` | 분석할 텍스트 (UTF-8 인코딩) |

#### 반환값

| 값 | 의미 |
|:---|:---|
| `const char*` (non-null) | 성공. **C++ 힙에 할당된** JSON 문자열 포인터 |
| `nullptr` | 실패. `handle` 또는 `text_utf8`이 null이거나 추론 중 예외 발생 |

#### 반환 JSON 구조

```json
{
  "phonology": "커피 세잔을 마셨다",
  "annotations": [...],
  "numbers": [...],
  "morphemes": [...],
  "heteronym": [...],
  "beon": [...]
}
```

#### 메모리 관리

> [!IMPORTANT]
> 반환된 포인터는 호출자가 소유합니다. 사용 후 반드시 `snap_free(result)`를 호출해야 합니다.  
> `delete`, `free()` 등 다른 해제 함수를 사용하면 힙 손상이 발생합니다.

---

### `snap_normalize`

BERT 추론 없이 텍스트 정규화(숫자 변환, 기호 처리 등)만 수행합니다.

```c
SNAP_API const char* snap_normalize(void* handle, const char* text_utf8);
```

#### 파라미터

| 이름 | 타입 | 설명 |
|:---|:---|:---|
| `handle` | `void*` | `snap_create`가 반환한 유효한 엔진 핸들 |
| `text_utf8` | `const char*` | 정규화할 텍스트 (UTF-8 인코딩) |

#### 반환값

| 값 | 의미 |
|:---|:---|
| `const char*` (non-null) | 성공. **C++ 힙에 할당된** 정규화 완료 문자열 포인터 |
| `nullptr` | 실패 |

#### 메모리 관리

`snap_process`와 동일 — 반드시 `snap_free(result)`로 해제해야 합니다.

---

### `snap_free`

`snap_process` 또는 `snap_normalize`가 반환한 힙 버퍼를 해제합니다.

```c
SNAP_API void snap_free(void* result);
```

#### 파라미터

| 이름 | 타입 | 설명 |
|:---|:---|:---|
| `result` | `void*` | `snap_process` 또는 `snap_normalize`의 반환값 |

#### 주의사항

- `nullptr`를 전달해도 안전합니다 (no-op).
- C 관례에 따라 파라미터 타입은 `void*`입니다. Python ctypes에서 `snap_free.argtypes = [ctypes.c_void_p]`로 선언해야 합니다.

---

### `snap_destroy`

엔진 인스턴스를 소멸하고 모든 연결된 리소스(ONNX 세션, 사전, 버퍼)를 해제합니다.

```c
SNAP_API void snap_destroy(void* handle);
```

#### 파라미터

| 이름 | 타입 | 설명 |
|:---|:---|:---|
| `handle` | `void*` | `snap_create`가 반환한 엔진 핸들 |

#### 주의사항

- `snap_destroy` 호출 후 해당 핸들로 `snap_process` 등을 호출하면 Undefined Behavior입니다.
- `snap_destroy`와 `snap_process`를 동일 핸들에 대해 동시에 호출하면 안 됩니다.

---

## 3. 스레드 안전성 및 인스턴스 격리

| 시나리오 | 안전 여부 | 설명 |
|:---|:---:|:---|
| 서로 다른 핸들로 `snap_process` 동시 호출 | ✅ 안전 | 핸들별로 완전히 독립된 메모리 공간 |
| 동일 핸들로 `snap_process` 동시 호출 (멀티스레드) | ✅ 안전 | ONNX Runtime `Session::Run`은 thread-safe |
| `snap_create` / `snap_destroy` 동시 호출 | ✅ 안전 | 서로 다른 핸들에 대한 경우 |
| 동일 핸들에 `snap_destroy` + `snap_process` 동시 | ❌ 위험 | 라이프사이클 관리는 호출자 책임 |

### 권장 패턴: 언어별 독립 엔진 동시 운용

```c
void* ko_handle = snap_create(weights, "ko");
void* ja_handle = snap_create(weights, "ja");

// 두 핸들은 완전히 독립적 — 동시 추론 가능
const char* ko_result = snap_process(ko_handle, ko_text);
const char* ja_result = snap_process(ja_handle, ja_text);

snap_free(ko_result);
snap_free(ja_result);
snap_destroy(ko_handle);
snap_destroy(ja_handle);
```

---

## 4. Python ctypes 연동 가이드

`snap_process`의 반환 타입을 `c_char_p`로 선언하면 ctypes가 자동으로 `bytes`로 변환하여 원래 포인터 값이 소실됩니다. 반드시 `c_void_p`로 선언하고 `ctypes.string_at`으로 복사한 뒤 `snap_free`로 해제해야 합니다.

### 단순화 예제 (권장 — lang 폴더 직접)

```python
import ctypes, os, json, pathlib

_HERE = pathlib.Path(__file__).resolve().parent
dll_dir = str(_HERE / "snap_cpp" / "build" / "Release")
ko_dir  = str(_HERE / "snap" / "ko")  # ko 폴더를 직접 지정

if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(dll_dir)
snap = ctypes.CDLL(os.path.join(dll_dir, "snap_cpp.dll"))

snap.snap_create.argtypes  = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype   = ctypes.c_void_p
snap.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype  = ctypes.c_void_p
snap.snap_free.argtypes    = [ctypes.c_void_p]
snap.snap_free.restype     = None
snap.snap_destroy.argtypes = [ctypes.c_void_p]
snap.snap_destroy.restype  = None

# ko 폴더를 직접 넘겨도 자동 탐색됩니다
handle = snap.snap_create(ko_dir.encode('utf-8'), b"ko")
if not handle:
    raise RuntimeError("SNAP 엔진 초기화 실패 — stderr 확인")

try:
    result_ptr = snap.snap_process(handle, "커피 3잔을 마셨다.".encode('utf-8'))
    if result_ptr:
        data = json.loads(ctypes.string_at(result_ptr).decode('utf-8'))
        print("TTS Metadata:", data)
        snap.snap_free(result_ptr)
finally:
    snap.snap_destroy(handle)
```

### 전체 예제 (기존 방식 — 루트 경로)

```python
import ctypes
import os
import json
import pathlib

# 1. Path setup (relative to this script's location)
_HERE = pathlib.Path(__file__).resolve().parent
dll_dir     = str(_HERE / "snap_cpp" / "build" / "Release")
weights_dir = str(_HERE / "snap_py" / "weights")

# 2. Load DLL
if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(dll_dir)
snap = ctypes.CDLL(os.path.join(dll_dir, "snap_cpp.dll"))

# 3. C API function signatures
snap.snap_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype = ctypes.c_void_p  # engine handle

# snap_process returns a heap-allocated C string pointer.
# Using c_char_p would cause ctypes to auto-convert it to bytes, losing the
# raw pointer needed for snap_free. Use c_void_p + ctypes.string_at instead.
snap.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype = ctypes.c_void_p

snap.snap_free.argtypes = [ctypes.c_void_p]  # void* matches C API signature
snap.snap_free.restype = None

snap.snap_destroy.argtypes = [ctypes.c_void_p]
snap.snap_destroy.restype = None

# 4. Create engine instance (Korean engine)
handle = snap.snap_create(weights_dir.encode('utf-8'), b"ko")
if not handle:
    raise RuntimeError("SNAP 엔진 초기화 실패 — stderr 확인 (영어 사전 누락 가능)")

try:
    # 5. Run inference and free the result
    text = "커피 3잔을 마셨다."
    result_ptr = snap.snap_process(handle, text.encode('utf-8'))

    if result_ptr:
        # Copy bytes from the C++ heap pointer and decode as UTF-8
        result_json = ctypes.string_at(result_ptr).decode('utf-8')
        data = json.loads(result_json)
        print("TTS Metadata:", data)

        # (Required) Free the C++ heap allocation to prevent memory leaks
        snap.snap_free(result_ptr)
finally:
    # 6. Destroy engine instance
    snap.snap_destroy(handle)
```

> [!TIP]
> 전체 예제는 저장소 루트의 [`e2e_benchmark.py`](../../e2e_benchmark.py)를 참고하세요.  
> `--dll-dir`, `--weights-dir` 인수로 경로를 직접 지정할 수 있습니다.

---

## 5. 설정 레퍼런스 (`snap_config.json`)

각 언어 가중치 폴더(`<weights_dir>/<lang>/snap_config.json`)에서 아래 키를 설정할 수 있습니다.

### 5.1 전체 키 목록

| 키 | 타입 | 기본값 | 설명 |
|:---|:---:|:---:|:---|
| `bert_model` | `string` | `"model.onnx"` | BERT ONNX 모델 파일명 |
| `use_int8` | `bool` | `false` | `true` 시 INT8 양자화 모델(`*_int8.onnx`) 자동 로드 |
| `num_threads` | `int` | `0` | ONNX intra-op 스레드 수. `0` = 자동 감지 (`logical_cores / 2`, 최대 8) |
| `device` | `string` | `"cpu"` | 추론 장치. `"cpu"` 또는 `"cuda"` / `"gpu"` |
| `g2p_threshold` | `float` | `0.0` | G2P 헤드 예측 신뢰도 하한값 |

### 5.2 설정 예시

**CPU + INT8 (권장 기본 설정):**
```json
{
    "bert_model": "model_bert.onnx",
    "use_int8": true,
    "num_threads": 0,
    "device": "cpu",
    "interpret_as": "yomi"
}
```

**GPU (CUDA) + INT8:**
```json
{
    "bert_model": "model_bert.onnx",
    "use_int8": true,
    "num_threads": 0,
    "device": "cuda",
    "interpret_as": "yomi"
}
```

### 5.3 `num_threads` 자동 감지 동작

```
num_threads = 0  →  target = max(logical_cores / 2, 1), capped at 8
num_threads = N  →  target = N  (N ≥ 1)
```

### 5.4 `device: "cuda"` 폴백 메커니즘

CUDA Execution Provider는 런타임에 `onnxruntime.dll`에서 `OrtSessionOptionsAppendExecutionProvider_CUDA` 심볼을 동적으로 탐색합니다.

| 상황 | 동작 |
|:---|:---|
| GPU 빌드 DLL + NVIDIA GPU 존재 | CUDA EP 활성화 |
| CPU 빌드 DLL (심볼 없음) | 경고 없이 CPU 모드로 자동 폴백 |
| GPU 빌드 DLL + GPU 없음 | ORT 내부에서 CPU 폴백 |

---

## 6. 성능 벤치마크

> 측정 환경: 30회 반복, Warmup 10회 선행, 짧은 문장 단위(Batch=1) 추론

### 6.1 FP32 vs INT8 비교 (CPU)

| 언어 | FP32 latency | INT8 latency | 속도 개선 |
|:---:|:---:|:---:|:---:|
| **KO** | 15.50 ms | **6.79 ms** | **×2.28** |
| **JA** | 13.05 ms | **5.48 ms** | **×2.38** |
| **EN** | 9.91 ms  | **4.24 ms** | **×2.34** |

### 6.2 CPU vs GPU (CUDA) 비교 — INT8 기준

| 언어 | CPU INT8 | GPU INT8 | GPU 추가 이득 |
|:---:|:---:|:---:|:---:|
| **KO** | 6.79 ms | 7.26 ms | ▼ (짧은 문장 전송 오버헤드) |
| **JA** | 5.48 ms | **4.25 ms** | **+22%** |
| **EN** | 4.24 ms | **3.96 ms** | **+7%** |

> [!NOTE]
> **짧은 문장에서 GPU가 오히려 느린 이유**  
> TTS 전처리는 Batch Size = 1, 시퀀스 길이가 매우 짧습니다.  
> Host→Device 데이터 전송 및 커널 실행 고정 오버헤드가 GPU 병렬 연산 이득보다 큽니다.  
> 문장이 길어질수록 GPU 우위가 커집니다 (JA: +22%).

### 6.3 장치 선택 가이드

| 환경 | 권장 설정 |
|:---|:---|
| 짧은 문장, 경량 서버 | `device: cpu` + `use_int8: true` |
| 긴 문장 또는 고처리량 서버 | `device: cuda` + `use_int8: true` |
| 배포 용량 제약 (≤ 15 MB) | CPU 빌드 (`USE_GPU=OFF`) |
| 최대 성능 | GPU 빌드 (`USE_GPU=ON`) + `device: cuda` + `use_int8: true` |

---

## 7. 빌드 옵션

`snap_cpp`는 CMake `USE_GPU` 옵션으로 패키지 크기를 제어합니다.

### 7.1 옵션 비교

| 옵션 | ONNX Runtime | 결과물 크기 | GPU 지원 |
|:---|:---:|:---:|:---:|
| `USE_GPU=OFF` (기본값) | CPU 전용 빌드 | **~10 MB** | ❌ |
| `USE_GPU=ON` | GPU 빌드 (CUDA + TensorRT) | **~400 MB** | ✅ |

### 7.2 빌드 명령

**CPU 전용:**
```bash
cmake -B build -DUSE_GPU=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

출력물: `build/Release/snap_cpp.dll`, `onnxruntime.dll`

**GPU 지원:**
```bash
cmake -B build -DUSE_GPU=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

출력물: `snap_cpp.dll`, `onnxruntime.dll`, `onnxruntime_providers_cuda.dll`, `onnxruntime_providers_shared.dll`, `onnxruntime_providers_tensorrt.dll`

> [!TIP]
> GPU 빌드 DLL을 배포한 뒤 `snap_config.json`에서 `"device": "cpu"`로 설정하면 GPU 없는 환경에서도 정상 동작합니다 (자동 폴백).
