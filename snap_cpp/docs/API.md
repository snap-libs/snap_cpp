# SNAP C API Reference

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
| `weights_dir` | `const char*` | Path to the weights root directory (UTF-8). The engine reads `<weights_dir>/<lang>/snap_config.json` to locate model files. |
| `lang` | `const char*` | Language code. Accepted values: `"ko"`, `"ja"`, `"en"` |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `void*` | Success — opaque handle to the engine instance |
| `NULL` | Failure — invalid path, missing model files, or internal exception |

**Notes**
- The returned handle must be released with `snap_destroy()` when no longer needed.
- Multiple handles (same or different languages) can coexist in the same process.

---

### `snap_process`

Run full inference on UTF-8 text: text normalization → BERT encoding → classification heads.

```c
const char* snap_process(void* handle, const char* text_utf8);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | A valid engine handle returned by `snap_create()` |
| `text_utf8` | `const char*` | Input text, UTF-8 encoded |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — null-terminated JSON string, **heap-allocated, caller must call `snap_free()`** |
| `NULL` | Failure — null argument or internal inference exception |

**Result JSON structure**

```json
{
  "phonology":   "커피 세잔을 마셨다",
  "annotations": [ { "start": 0, "end": 2, "label": "TENS" } ],
  "numbers":     [ { "span": "3", "reading": "세" } ],
  "morphemes":   [ { "surface": "커피", "pos": "NNG", "start": 0, "end": 2 } ],
  "heteronym":   [],
  "beon":        []
}
```

> **Memory**: call `snap_free(result)` after use. Do not pass the pointer to
> `free()`, `delete`, or `delete[]` — the buffer was allocated with `new char[]`
> inside the library.

---

### `snap_normalize`

Run text normalization only (number expansion, symbol handling). No BERT inference.

```c
const char* snap_normalize(void* handle, const char* text_utf8);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | A valid engine handle returned by `snap_create()` |
| `text_utf8` | `const char*` | Input text, UTF-8 encoded |

**Return value**

| Value | Meaning |
|:---|:---|
| non-null `const char*` | Success — null-terminated normalized string, **heap-allocated, caller must call `snap_free()`** |
| `NULL` | Failure |

---

### `snap_free`

Release a heap buffer returned by `snap_process()` or `snap_normalize()`.

```c
void snap_free(void* result);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `result` | `void*` | Pointer returned by `snap_process()` or `snap_normalize()`. Passing `NULL` is safe (no-op). |

> **Note**: the parameter type is `void*` following C convention. In Python ctypes,
> declare `snap_free.argtypes = [ctypes.c_void_p]`.

---

### `snap_destroy`

Destroy an engine instance and free all associated resources (ONNX sessions, dictionaries, buffers).

```c
void snap_destroy(void* handle);
```

**Parameters**

| Name | Type | Description |
|:---|:---|:---|
| `handle` | `void*` | Engine handle returned by `snap_create()`. Passing `NULL` is safe (no-op). |

> **Warning**: after `snap_destroy()` returns, the handle is invalid. Do not call
> `snap_process()` or any other function with it.

---

## Thread Safety

| Scenario | Safe? |
|:---|:---:|
| Concurrent `snap_process()` on **different** handles | ✅ |
| Concurrent `snap_process()` on the **same** handle (multiple threads) | ✅ |
| Concurrent `snap_create()` / `snap_destroy()` on different handles | ✅ |
| Calling `snap_destroy()` and `snap_process()` on the **same** handle simultaneously | ❌ |

Engine lifecycle (`snap_create` / `snap_destroy`) must be managed by a single
controlling thread or synchronized externally.

---

## Configuration (`snap_config.json`)

The engine reads `<weights_dir>/<lang>/snap_config.json` at initialization time.

### Keys

| Key | Type | Default | Description |
|:---|:---:|:---:|:---|
| `bert_model` | string | `"model.onnx"` | BERT ONNX filename inside the language directory |
| `use_int8` | bool | `false` | When `true`, automatically loads `*_int8.onnx` instead |
| `num_threads` | int | `0` | ONNX intra-op thread count. `0` = auto (`logical_cores / 2`, max 8) |
| `device` | string | `"cpu"` | Inference device: `"cpu"` or `"cuda"` / `"gpu"` |
| `g2p_threshold` | float | `0.0` | Minimum confidence threshold for G2P head predictions |

### Example (recommended: CPU + INT8)

```json
{
    "bert_model":  "model_bert.onnx",
    "use_int8":    true,
    "num_threads": 0,
    "device":      "cpu"
}
```

### GPU / CUDA

Set `"device": "cuda"` to enable the CUDA Execution Provider. The library
locates the provider symbol at runtime via `GetProcAddress` / `dlsym` — no
recompile needed when swapping between CPU-only and GPU builds of
`onnxruntime`.

If the symbol is absent or GPU initialization fails, the engine silently falls
back to CPU without crashing.

---

## Build Options

| CMake option | Default | Effect |
|:---|:---:|:---|
| `USE_GPU=OFF` | ✅ | Downloads CPU-only ONNX Runtime (~10 MB DLL) |
| `USE_GPU=ON` | — | Downloads GPU build of ONNX Runtime with CUDA / TensorRT providers (~400 MB) |

```bash
# CPU-only build (default)
cmake -B build -DUSE_GPU=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# GPU-enabled build
cmake -B build -DUSE_GPU=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## Language Bindings

### Python (ctypes)

```python
import ctypes, os, json, pathlib

_HERE       = pathlib.Path(__file__).resolve().parent
dll_dir     = str(_HERE / "snap_cpp" / "build" / "Release")
weights_dir = str(_HERE / "snap_py"  / "weights")

if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(dll_dir)
snap = ctypes.CDLL(os.path.join(dll_dir, "snap_cpp.dll"))

snap.snap_create.argtypes    = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype     = ctypes.c_void_p

# Use c_void_p (not c_char_p) so ctypes does not auto-convert the pointer to
# bytes — the raw address is needed to call snap_free() afterwards.
snap.snap_process.argtypes   = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype    = ctypes.c_void_p

snap.snap_free.argtypes      = [ctypes.c_void_p]
snap.snap_free.restype       = None

snap.snap_destroy.argtypes   = [ctypes.c_void_p]
snap.snap_destroy.restype    = None

handle = snap.snap_create(weights_dir.encode(), b"ko")
if handle:
    try:
        ptr = snap.snap_process(handle, "커피 3잔을 마셨다.".encode())
        if ptr:
            data = json.loads(ctypes.string_at(ptr))
            snap.snap_free(ptr)
            print(data["phonology"])
    finally:
        snap.snap_destroy(handle)
```

### C

```c
#include "snap/snap_api.h"
#include <stdio.h>

int main(void) {
    void* handle = snap_create("../weights", "ko");
    if (!handle) return 1;

    const char* result = snap_process(handle, "\xEC\BB\A4\xED\x94\xBC 3\xEC\x9E\x94\xEC\x9D\x84 \xEB\xA7\x88\xEC\x85\xA8\xEB\x8B\xA4.");
    if (result) {
        printf("%s\n", result);
        snap_free((void*)result);
    }

    snap_destroy(handle);
    return 0;
}
```

### C# (P/Invoke)

```csharp
using System;
using System.Runtime.InteropServices;
using System.Text;

static class SnapApi {
    const string Dll = "snap_cpp";

    [DllImport(Dll)] public static extern IntPtr snap_create(string weightsDir, string lang);
    [DllImport(Dll)] public static extern IntPtr snap_process(IntPtr handle, byte[] textUtf8);
    [DllImport(Dll)] public static extern void   snap_free(IntPtr result);
    [DllImport(Dll)] public static extern void   snap_destroy(IntPtr handle);
}

// Usage
var handle = SnapApi.snap_create(@"C:\weights", "ko");
var ptr    = SnapApi.snap_process(handle, Encoding.UTF8.GetBytes("커피 3잔을 마셨다."));
var json   = Marshal.PtrToStringUTF8(ptr);
SnapApi.snap_free(ptr);
SnapApi.snap_destroy(handle);
```

---

## Performance

Measured over 30 iterations (batch size = 1, short sentences).

| Lang | Precision | Device | Avg latency | Throughput |
|:---:|:---:|:---:|:---:|:---:|
| KO | FP32 | CPU | 15.50 ms | 1,370 chars/s |
| KO | **INT8** | CPU | **6.79 ms** | **3,128 chars/s** |
| JA | FP32 | CPU | 13.05 ms | 1,991 chars/s |
| JA | **INT8** | CPU | **5.48 ms** | **4,737 chars/s** |
| JA | **INT8** | **GPU** | **4.25 ms** | **6,107 chars/s** |
| EN | FP32 | CPU | 9.91 ms  | 6,791 chars/s |
| EN | **INT8** | CPU | **4.24 ms** | **15,863 chars/s** |
| EN | **INT8** | **GPU** | **3.96 ms** | **16,989 chars/s** |

> GPU overhead dominates for very short sequences (batch=1).
> For longer texts or sustained throughput workloads, `device: "cuda"` + INT8
> provides additional gains (up to ×3.07 vs CPU FP32).
