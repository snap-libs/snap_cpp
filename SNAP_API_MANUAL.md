# SNAP C API Reference Manual

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

**Memory ownership rule**: every non-null pointer returned by `snap_process()`
or `snap_normalize()` is a heap-allocated buffer owned by the caller.
It must be released exactly once with `snap_free()`.

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
| `NULL` | Failure — invalid path, missing model files (`snap_config.json`), or initialization exception |

**Notes**
- **Recommended Usage (`SNAP_HOME`)**:
  ```cpp
  void* handle = snap_create(NULL, "ko"); // Recommended: Uses SNAP_HOME environment variable
  ```
- **Multilingual Concurrent Setup**:
  ```cpp
  void* handle_ko = snap_create(NULL, "ko");
  void* handle_ja = snap_create(NULL, "ja");
  void* handle_en = snap_create(NULL, "en");
  ```

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

Run full context-aware text normalization & G2P inference on UTF-8 input text using BERT probing heads.

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
| non-null `const char*` | Success — heap-allocated, null-terminated UTF-8 output string. **Caller must release with `snap_free()`.** |
| `NULL` | Failure — `handle` is `NULL`, `text_utf8` is `NULL`, or processing exception |

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
