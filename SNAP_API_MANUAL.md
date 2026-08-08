# SNAP C API Reference Manual

> Header: `include/snap/snap_api.h`  
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

## Functions

---

### `snap_create`

Allocate and initialize a SNAP engine instance for one language.

```c
void* snap_create(const char* weights_dir, const char* lang);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `weights_dir` | `const char*` | Path to the weights root directory (UTF-8). If `NULL` or `""`, the engine falls back to reading the `SNAP_HOME` environment variable. If an explicit path is provided, it **overrides** `SNAP_HOME`. |
| `lang` | `const char*` | Language code. Accepted values: `"ko"`, `"ja"`, `"en"` (case-insensitive). |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `void*` | Success — opaque handle to the engine instance |
| `NULL` | Failure — invalid path, missing model files (`snap_config.json`), or internal exception |

**Notes**
- **Recommended Usage (`SNAP_HOME`)**: Since `snap_init` configures the `SNAP_HOME` environment variable automatically, passing `NULL` or `std::getenv("SNAP_HOME")` is recommended for single-instance applications:
  ```cpp
  void* handle = snap_create(NULL, "ko"); // Recommended: Uses SNAP_HOME
  ```
- **Custom Absolute Path Override**: You can bypass `SNAP_HOME` and pass any explicit absolute or relative path:
  ```cpp
  void* handle = snap_create("C:/apps/snap_models_v1", "ko"); // Overrides SNAP_HOME
  ```
- **Multilingual Concurrent Setup**:
  From a single `SNAP_HOME` root initialized via `snap_init`, you can concurrently instantiate handles for Korean (`"ko"`), Japanese (`"ja"`), and English (`"en"`):
  ```cpp
  // Concurrent Multilingual handles using SNAP_HOME
  void* handle_ko = snap_create(NULL, "ko");
  void* handle_ja = snap_create(NULL, "ja");
  void* handle_en = snap_create(NULL, "en");

  const char* res_ko = snap_process(handle_ko, "오후 3시에 만납시다.");
  const char* res_ja = snap_process(handle_ja, "午後3時に会いましょう。");
  const char* res_en = snap_process(handle_en, "Let's meet at 3 PM.");

  snap_free((void*)res_ko);
  snap_free((void*)res_ja);
  snap_free((void*)res_en);

  snap_destroy(handle_ko);
  snap_destroy(handle_ja);
  snap_destroy(handle_en);
  ```
- The returned handle must be released with `snap_destroy()` when no longer needed.

---

### `snap_create_with_version`

Allocate and initialize a SNAP engine instance with explicit model variant and version pinning.

```c
void* snap_create_with_version(const char* weights_dir, const char* lang, const char* variant, const char* version);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `weights_dir` | `const char*` | Weights root directory path. Falls back to `SNAP_HOME` if `NULL` or `""`. |
| `lang` | `const char*` | Language code (`"ko"`, `"ja"`, `"en"`). |
| `variant` | `const char*` | Model variant specifier (e.g. `"standard"`, `"lite"`, `"base"`). If `NULL` or `""`, defaults to `"standard"`. |
| `version` | `const char*` | Model version tag (e.g. `"1.0.0"`, `"v1"`). If `NULL` or `""`, defaults to `"latest"`. |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `void*` | Success — engine handle pinned to requested variant/version |
| `NULL` | Failure — requested variant/version files not found |

---

### `snap_process`

Run full pre-processing on UTF-8 input text for the language specified when `handle` was created.

```c
const char* snap_process(void* handle, const char* text);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle returned by `snap_create()` or `snap_create_with_version()`. |
| `text` | `const char*` | Null-terminated UTF-8 input string. Must not be `NULL`. |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — heap-allocated, null-terminated UTF-8 output string. **Caller must release with `snap_free()`.** |
| `NULL` | Failure — `handle` is `NULL`, `text` is `NULL`, or pipeline execution error |

**Memory Notice**

The return pointer is allocated via internal C allocator (`malloc` / `strdup`). **Do not use C++ `delete` or standard C `free()` directly across DLL boundaries on Windows.** Always pass it to `snap_free()`.

---

### `snap_normalize`

Alias for `snap_process()`. Provided for API consistency.

```c
const char* snap_normalize(void* handle, const char* text);
```

**Behavior**: Identical to `snap_process()`.

---

### `snap_free`

Release a string buffer returned by `snap_process()` or `snap_normalize()`.

```c
void snap_free(void* ptr);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `ptr` | `void*` | Pointer previously returned by `snap_process()` or `snap_normalize()`. Safe to pass `NULL` (no-op). |

---

### `snap_destroy`

Destroy an engine instance and release all associated ONNX Runtime sessions, memory, and model weights.

```c
void snap_destroy(void* handle);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine instance handle to destroy. Safe to pass `NULL` (no-op). |

---

### `snap_version`

Retrieve the engine runtime version string.

```c
const char* snap_version(void);
```

**Return value**: Pointer to a static, null-terminated version string (e.g. `"1.0.0"`). **Do not pass this pointer to `snap_free()`.**

---

### `snap_last_error`

Retrieve the thread-local error message for the last failed operation.

```c
const char* snap_last_error(void);
```

**Return value**: Pointer to a thread-local static string describing the last error. **Do not pass to `snap_free()`.**

---

## Complete C++ Usage Example

```cpp
#include <iostream>
#include <cstdlib>
#include "snap/snap_api.h"

int main() {
    // 1. Initialize SNAP engine for Korean using SNAP_HOME
    void* handle = snap_create(NULL, "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine: " 
                  << (snap_last_error() ? snap_last_error() : "Unknown error") << std::endl;
        return 1;
    }

    // 2. Perform text normalization
    const char* input = "여기서 3번 버스를 타고 3번 갈아타야 해.";
    const char* result = snap_process(handle, input);

    if (result) {
        std::cout << "Input:  " << input << std::endl;
        std::cout << "Output: " << result << std::endl;

        // 3. Free output string
        snap_free((void*)result);
    } else {
        std::cerr << "Processing error: " << snap_last_error() << std::endl;
    }

    // 4. Destroy engine handle
    snap_destroy(handle);
    return 0;
}
```
