# SNAP C/C++ Native SDK API Reference Manual

> **📌 Document Guidance**  
> * **This Document (`SNAP_SDK_API_MANUAL.md`):** C/C++ Native Embedded Library (Headers, DLL/SO, Memory Management) Reference Specification.  
> * **Korean Version (한국어 매뉴얼):** Please refer to [`SNAP_SDK_API_MANUAL_KO.md`](SNAP_SDK_API_MANUAL_KO.md).  
> * **Web / Cloud HTTP REST API:** For cloud JSON endpoints, please refer to [`SNAP_REST_API_MANUAL.md`](SNAP_REST_API_MANUAL.md) ([한국어](SNAP_REST_API_MANUAL_KO.md)).

> Header: `include/snap/snap_api.h` & `include/snap/snap_version.h`  
> Library: `snap_cpp.dll` / `libsnap_cpp.so` / `libsnap_cpp.dylib`

---

## Overview

`snap_cpp` exposes a minimal C-linkage API built on an **opaque handle** pattern.
Each call to `snap_create()` returns an independent engine instance, enabling
multiple languages to run concurrently in a single process without shared state
or mutex contention.

**Lifecycle**

```
snap_create()
    │
    ├── snap_process()    ──► snap_free(result)
    ├── snap_normalize()  ──► snap_free(result)
    │
snap_destroy()
```

**Path Resolution**: `snap_create(weights_dir, lang)` resolves model assets by checking:
1. `weights_dir` (Explicit argument — Absolute or Relative path)
2. `SNAP_HOME` environment variable (if `weights_dir` is `NULL` or `""`)
3. Current working directory (`.`)

**Memory ownership rule**: Every non-null pointer returned by `snap_process()` or `snap_normalize()` is a heap-allocated buffer owned by the caller. It **must** be released exactly once using `snap_free()`.  
*(Note: Prebuilt binaries use static CRT linkage `/MT`. Returning pointers are managed within the DLL's internal heap. Always release buffers via `snap_free()` to prevent cross-boundary heap corruption).*

---

## Core Inference Functions (`snap_api.h`)

---

### `snap_create`

Allocate and initialize a SNAP engine instance for one language (loads BERT backbone and neural probing heads).

```c
SNAP_API void* snap_create(const char* weights_dir, const char* lang);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `weights_dir` | `const char*` | Path to weights root or language directory (UTF-8). If `NULL` or `""`, falls back to reading the `SNAP_HOME` environment variable. If an explicit path is provided, it **overrides** `SNAP_HOME`. |
| `lang` | `const char*` | Language code (`"ko"`, `"ja"`, `"en"`, case-insensitive). |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `void*` | Success — opaque handle to the engine instance |
| `NULL` | Failure — invalid path, missing model files (`model_index.json`), or initialization exception |

**Notes**
- **Recommended Usage (`SNAP_HOME`)**:
  ```cpp
  void* handle = snap_create(NULL, "ko"); // Recommended: Uses SNAP_HOME environment variable and auto EP
  ```
- **Multilingual Concurrent Setup**:
  ```cpp
  void* handle_ko = snap_create(NULL, "ko");
  void* handle_ja = snap_create(NULL, "ja");
  void* handle_en = snap_create(NULL, "en");
  ```

---

### `snap_create_device`

Allocate and initialize a SNAP engine instance with explicit target device / Execution Provider (EP).

```c
SNAP_API void* snap_create_device(const char* weights_dir, const char* lang, const char* device);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `weights_dir` | `const char*` | Path to weights root or language directory (UTF-8). Falls back to `SNAP_HOME` if `NULL` or `""`. |
| `lang` | `const char*` | Language code (`"ko"`, `"ja"`, `"en"`). |
| `device` | `const char*` | Target device / Execution Provider: `"auto"` (default), `"cuda"` (or `"cuda:0"`), `"directml"`, `"coreml"`, `"openvino"`, or `"cpu"`. |

**Hardware Execution Provider (EP) Resolution Hierarchy**
1. Explicit `device` argument in `snap_create_device()`
2. `SNAP_DEVICE` environment variable (e.g. `export SNAP_DEVICE=cuda` or `SNAP_DEVICE=cpu`)
3. `models/snap_config.json` global `"device"` setting
4. Default `"auto"` (automatically detects CUDA ➔ CoreML ➔ DirectML ➔ CPU with 0ms inference overhead)

*Note: If a requested hardware EP is not available or fails to initialize, SNAP logs a warning and gracefully falls back to CPU without crashing.*

---

### `snap_create_with_version`

Allocate and initialize a SNAP engine instance with explicit model variant and manifest version pinning.

```c
SNAP_API void* snap_create_with_version(
    const char* weights_dir,
    const char* lang,
    const char* variant,
    const char* dict_version,
    const char* model_version
);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `weights_dir` | `const char*` | Weights root directory path. Falls back to `SNAP_HOME` if `NULL` or `""`. |
| `lang` | `const char*` | Language code (`"ko"`, `"ja"`, `"en"`). |
| `variant` | `const char*` | Model variant (e.g., `"kcbert-base-int8"`, or `NULL` for default). |
| `dict_version` | `const char*` | Dictionary lexicon version tag (e.g., `"v1.0.0"`, or `NULL` for active version). |
| `model_version` | `const char*` | Neural backbone model version tag (e.g., `"v1.0.0"`, or `NULL` for active version). |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `void*` | Success — engine handle pinned to explicit model/dict versions |
| `NULL` | Failure — requested version/variant manifest or files not found |

---

### `snap_process`

Run full context-aware text normalization and G2P inference on UTF-8 input text using BERT probing heads.

```c
SNAP_API const char* snap_process(void* handle, const char* text_utf8);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()`. |
| `text_utf8` | `const char*` | Null-terminated UTF-8 encoded input string. Must not be `NULL`. |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — heap-allocated, null-terminated UTF-8 JSON metadata string. **Caller must release with `snap_free()`.** |
| `NULL` | Failure — `handle` is `NULL`, `text_utf8` is `NULL`, or processing exception |

**Return JSON Schema**

```json
{
  "normalized_text": "이천이십육년 팔월 십이일 서울의 날씨는 매우 맑고 기온은 이십팔도입니다.",
  "phonology": "이처니심늉년 파뤌 시비일 서우레 날씨는 매우 말꼬 기오는 이십팔또임니다.",
  "pauses": [
    {"word_idx": 0, "word": "이처니심늉년", "pause": "NONE"},
    {"word_idx": 1, "word": "파뤌", "pause": "NONE"},
    {"word_idx": 2, "word": "시비일", "pause": "NONE"},
    {"word_idx": 3, "word": "서우레", "pause": "NONE"},
    {"word_idx": 4, "word": "날씨는", "pause": "P1"},
    {"word_idx": 5, "word": "매우", "pause": "NONE"},
    {"word_idx": 6, "word": "말꼬", "pause": "P2"},
    {"word_idx": 7, "word": "기오는", "pause": "NONE"},
    {"word_idx": 8, "word": "이십팔또임니다.", "pause": "P3"}
  ],
  "annotations": [],
  "numbers": [],
  "morphemes": [],
  "heteronym": [],
  "counter": []
}
```

> **Note on STT Sentence Segmentation**:  
> In Speech-to-Text (STT) post-processing pipelines, words with `"pause": "P3"` indicate sentence boundaries determined by neural context analysis of sentence-final endings (`EF`). Splitting unpunctuated STT transcripts on `P3` boundaries provides reliable sentence segmentation.

---

### `snap_process_ext`

Run SNAP inference with per-sentence dynamic options (in-memory JSON string), allowing zero-disk-IO overriding of default `snap_config.json` settings at runtime.

```c
SNAP_API const char* snap_process_ext(
    void* handle, 
    const char* text_utf8, 
    const char* options_json
);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()`. |
| `text_utf8` | `const char*` | Input UTF-8 string to process. |
| `options_json` | `const char*` | Null-terminated in-memory JSON string containing per-sentence option overrides (e.g. `"{\"to_ssml\": true}"`). If `NULL` or `""`, default config settings apply. |

**Supported Dynamic Option Keys (`options_json`)**

| Key | Type | Values | Description |
|:---|:---:|:---|:---|
| `to_ssml` | `bool` | `true`, `false` | Enable standard W3C SSML `<break>` tags attached directly to G2P phonetic transcription. |
| `to_ipa` / `return_ipa` | `bool` | `true`, `false` | Enable IPA transcription output string. |
| `prosody_format` | `string` | `"tags"`, `"ssml"`, `"none"` | Format of prosodic pauses (automatically maps to SSML when set to `"ssml"`). |
| `vowel_length` | `bool` | `true`, `false` | (Korean) Enable vowel length colon notation (`:`). |
| `pitch_accent` | `bool` | `true`, `false` | (Japanese) Enable pitch accent kernel notation. |
| `script` | `string` | `"katakana"`, `"hiragana"`, `"romaji"` | (Japanese) Output orthography/script type. |
| `to_json` | `bool` | `true`, `false` | When set to `false`, returns plain phonetic text directly instead of a full JSON metadata object. |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — heap-allocated output string formatted according to dynamic options. **Caller must release with `snap_free()`.** |
| `NULL` | Failure |

**Usage Example**

```cpp
// 1. Default G2P phonetic transcription (Returns full JSON)
const char* res1 = snap_process_ext(handle, "국무회의를 열고 예산안을 확정했습니다.", "{}");

// 2. Request raw plain G2P text directly without JSON wrapping
const char* res2 = snap_process_ext(handle, "국무회의를 열고 예산안을 확정했습니다.", "{\"to_json\": false}");
// Output: "궁무회이를 열고 내년도 예사난 편성 지치믈 최종 확쩡핻씀니다."

// 3. Target standard W3C SSML with G2P phonetic transcription and pause breaks
const char* res3 = snap_process_ext(handle, "국무회의를 열고 예산안을 확정했습니다.", "{\"to_ssml\": true, \"to_json\": false}");
// Output: "<speak>궁무회이를 열고 <break strength=\"medium\"/> 내년도 예사난 편성 지치믈 최종 확쩡핻씀니다.</speak>"

snap_free((void*)res1);
snap_free((void*)res2);
snap_free((void*)res3);
```

---

### `snap_process_batch`

Run batch context-aware text normalization and G2P inference on multiple UTF-8 input texts simultaneously using True Dynamic Batching.

BERT backbone and Morph Probing Heads are executed in a **single matrix forward pass**, maximizing hardware throughput while guaranteeing **100% exact match parity** with sequential `snap_process()` calls.

```c
SNAP_API const char* snap_process_batch(
    void* handle, 
    const char** texts_utf8, 
    int count
);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()` or `snap_create_device()`. |
| `texts_utf8` | `const char**` | Array of null-terminated UTF-8 input strings (`const char* texts[count]`). |
| `count` | `int` | Number of strings in the array. (Recommended: `16~32` for CPU, `64~128` for GPU). |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — JSON array string `[ {...}, {...}, ... ]` where each element corresponds to the full SNAP result object of each input text. **Caller must release with `snap_free()`.** |
| `NULL` | Failure |

**Return Format Comparison (Single vs Batch)**

* **Single (`snap_process`)**: Returns a single JSON Object.
  ```json
  {
    "phonology": "궁무회이를 열고 예사난 만 삼배고시붜늘 확쩡핻씀니다.",
    "normalized_text": "국무회의를 열고 예산안 만 삼백오십원을 확정했습니다.",
    "pauses": [...],
    "morphemes": [...]
  }
  ```
* **Batch (`snap_process_batch`)**: Returns a JSON Array containing an object for each sentence.
  ```json
  [
    {
      "phonology": "궁무회이를 열고 예사난 만 삼배고시붜늘 확쩡핻씀니다.",
      "normalized_text": "국무회의를 열고 예산안 만 삼백오십원을 확정했습니다.",
      "pauses": [...]
    },
    {
      "phonology": "여기서 삼번 뻐스를 타고 세번 가라타세요.",
      "normalized_text": "여기서 삼번 버스를 타고 세번 갈아타세요.",
      "pauses": [...]
    }
  ]
  ```

**Hardware Batch Sizing Best Practices**

| Environment | Optimal Batch Size | Rationale |
|:---|:---:|:---|
| **CPU (Intel / AMD / ARM)** | `16 ~ 32` | Optimally fits within CPU L3 cache memory (16~32MB) without cache misses or RAM memory bottlenecks. |
| **GPU / NPU (CUDA / DirectML / CoreML)** | `64 ~ 128` | Fully saturates thousands of Tensor Cores and utilizes high-bandwidth VRAM for maximum TPS. |

**C / C++ Usage Example**

```cpp
#include <stdio.h>
#include "snap/snap_api.h"

int main() {
    void* handle = snap_create(NULL, "ko");
    if (!handle) return 1;

    const char* texts[3] = {
        "정부는 오늘 오전 국무회의를 열고 2024년도 예산안을 확정했습니다.",
        "여기서 3번 버스를 타고 2번 갈아타야 판교역에 갈 수 있어.",
        "오후 4시 20분에 서울역 4번 출구에서 만납시다."
    };

    // 1 single forward pass for all 3 sentences
    const char* json_array = snap_process_batch(handle, texts, 3);
    printf("Batch JSON Output:\n%s\n", json_array);

    // Release buffer exactly once
    snap_free((void*)json_array);
    snap_destroy(handle);
    return 0;
}
```

---

### `snap_process_batch_ext`

Run batch inference on multiple UTF-8 texts with per-request dynamic option overrides (e.g., SSML tags, IPA phonetic transcription, or plain text output).

```c
SNAP_API const char* snap_process_batch_ext(
    void* handle, 
    const char** texts_utf8, 
    int count, 
    const char* options_json
);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()`. |
| `texts_utf8` | `const char**` | Array of null-terminated UTF-8 input strings. |
| `count` | `int` | Number of strings in the array. |
| `options_json` | `const char*` | Dynamic JSON options (e.g. `"{\"to_ssml\": true}"` or `"{\"to_json\": false}"`). |

**Plain Text Output Mode (`"to_json": false`)**

When `"to_json": false` is passed, `snap_process_batch_ext` returns each sentence's phonetic result separated by a newline (`\n`), allowing zero-JSON-parsing overhead for high-speed pipelines:

```cpp
const char* plain_results = snap_process_batch_ext(handle, texts, 3, "{\"to_json\": false}");
// Output:
// 궁무회이를 열고 내년도 예사난 편성 지치믈 최종 확쩡핻씀니다.
// 여기서 삼번 뻐스를 타고 두번 가라타야 판교여게 갈 수 이써.
// 오후 네시 이시부네 서울력 사번 출구에서 만납씨다.

snap_free((void*)plain_results);
```

---

### Large-Scale Dataset Chunking Pattern (Python Example)

When processing large datasets (e.g., 10,000 ~ 100,000 sentences), chunking inputs into optimal batch sizes (`B=32` for CPU, `B=64` for GPU) achieves linear scalability and constant, flat RAM usage (~150MB):

```python
import ctypes
import json

lib = ctypes.CDLL("snap_cpp.dll")
lib.snap_create.restype = ctypes.c_void_p
lib.snap_process_batch.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_int]
lib.snap_process_batch.restype = ctypes.c_void_p
lib.snap_free.argtypes = [ctypes.c_void_p]

handle = lib.snap_create(b"models", b"ko")
sentences = [...]  # 10,000 sentences

BATCH_SIZE = 32  # Optimal for CPU
results = []

for i in range(0, len(sentences), BATCH_SIZE):
    chunk = sentences[i:i + BATCH_SIZE]
    c_arr = (ctypes.c_char_p * len(chunk))(*(s.encode('utf-8') for s in chunk))
    
    ptr = lib.snap_process_batch(handle, c_arr, len(chunk))
    batch_json = ctypes.string_at(ptr).decode('utf-8')
    results.extend(json.loads(batch_json))
    lib.snap_free(ptr)

lib.snap_destroy(handle)
```

---

### `snap_normalize`

Standard SNAP text normalization & G2P processing (Alias for `snap_process`).

```c
SNAP_API const char* snap_normalize(void* handle, const char* text_utf8);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()`. |
| `text_utf8` | `const char*` | Input UTF-8 string to normalize. |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — heap-allocated normalized text string. **Caller must release with `snap_free()`.** |

---

### `snap_free`

Release a heap-allocated result string buffer returned by `snap_process()` or `snap_normalize()`.

```c
SNAP_API void snap_free(const void* result);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `result` | `const void*` | Pointer previously returned by `snap_process()` or `snap_normalize()`. Safe to pass `NULL` (no-op). |

---

### `snap_destroy`

Destroy an engine instance and release all associated ONNX Runtime sessions, memory, and model weights.

```c
SNAP_API void snap_destroy(void* handle);
```

---

## Versioning & ABI Compatibility Functions (`snap_version.h`)

---

### `SNAP_VERSION_*` Compile-time Macros

```c
#define SNAP_VERSION_MAJOR    1
#define SNAP_VERSION_MINOR    0
#define SNAP_VERSION_PATCH    0
#define SNAP_VERSION_STRING   "1.0.0"
#define SNAP_VERSION_NUMBER   10000 // (MAJOR * 10000 + MINOR * 100 + PATCH)
```

---

### `snap_version`

Get the runtime shared library version string.

```c
SNAP_API const char* snap_version(void);
```
**Return value**: Static null-terminated version string (e.g. `"1.0.0"`). Do **not** pass to `snap_free()`.

---

### `snap_version_number`

Get runtime numeric version value for version comparison.

```c
SNAP_API int snap_version_number(void);
```
**Return value**: Integer version (e.g., `10000` for `1.0.0`).

---

### `snap_version_info`

Get detailed major, minor, and patch version numbers.

```c
SNAP_API void snap_version_info(int* major, int* minor, int* patch);
```

---

### `snap_version_check`

Check ABI compatibility against target major and minor version numbers.

```c
SNAP_API int snap_version_check(int major, int minor);
```
**Return value**: `1` if compatible, `0` otherwise.

---

### `snap_version_verbose`

Get verbose runtime version information (includes compiler, build date, and target platform).

```c
SNAP_API const char* snap_version_verbose(void);
```

---

## Configuration & Engine Options (`snap_config.json`)

The SNAP C++ Engine controls G2P output modes, SSML tags, target TTS engine adapters, and language-specific phonological rules via `models/snap_config.json` (or dynamic in-memory settings passed to `snap_process_ext`).

```json
{
    "ko": {
        "vowel_length": false,
        "to_ipa": false,
        "to_ssml": false
    },
    "ja": {
        "script": "katakana",
        "pitch_accent": false,
        "to_ipa": false,
        "to_ssml": false
    },
    "en": {
        "to_ipa": false,
        "to_ssml": false
    }
}
```

### Key Configuration Definitions

| Key | Type | Default | Description |
|:---|:---|:---|:---|
| `to_ssml` | `bool` | `false` | When `true`, outputs standard W3C SSML tags (`<break strength="..."/>` for Korean pauses, `<phoneme alphabet="ipa" ph="...">word</phoneme>` for English). When `false`, SNAP outputs direct phonetic text. |
| `to_ipa` | `bool` | `false` | Enables International Phonetic Alphabet (IPA) output string transcription (e.g. `[aɪ rɛd ðə bʊk...]`). |
| `vowel_length` | `bool` | `false` | Korean vowel length notation (`:`). |
| `script` | `string` | `"katakana"` | Japanese script output (`"katakana"`, `"hiragana"`, or `"romaji"`). |
| `pitch_accent` | `bool` | `false` | Japanese pitch accent contour marking (`^` rise, `]` drop). |

---

## Complete C++ Integration Example

```cpp
#include <iostream>
#include "snap/snap_api.h"
#include "snap/snap_version.h"

int main() {
    // 0. Verify SDK Version
    std::cout << "SNAP SDK Version: " << snap_version() << std::endl;
    if (!snap_version_check(SNAP_VERSION_MAJOR, SNAP_VERSION_MINOR)) {
        std::cerr << "ABI incompatibility detected!\n";
        return 1;
    }

    // 1. Initialize SNAP engine for Korean using SNAP_HOME
    void* handle = snap_create(NULL, "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine handle.\n";
        return 1;
    }

    // 2. Perform context-aware text normalization & G2P processing
    const char* input = "여기서 3번 버스를 타고 3번 가라타야 해.";
    const char* result = snap_process(handle, input);

    if (result) {
        std::cout << "Input:  " << input << std::endl;
        std::cout << "Output: " << result << std::endl;

        // 3. Release output string buffer
        snap_free(result);
    }

    // 4. Destroy engine handle
    snap_destroy(handle);
    return 0;
}
```

---

## Advanced Backend Integration (For TTS Engines)

For deep tensor-level integration with neural TTS models (e.g. MeloTTS, VITS), SNAP SDK provides direct hidden-state export functions:

```c
SNAP_API float* snap_get_bert_features(
    void* handle, 
    const char* text_utf8, 
    int* out_seq_len, 
    int* out_hidden_dim, 
    int** out_word2ph, 
    int* out_word2ph_len
);

SNAP_API void snap_free_tensor(void* ptr);
```
*(Note: Internal integration API designed for bypassing PyTorch BERT downloads in open-source speech backends. General application developers should use `snap_process` or `snap_process_ext`.)*
