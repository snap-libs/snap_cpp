# SNAP C++ SDK (Public Release)

High-performance, zero-dependency C/C++ Frontend Inference Engine for Multilingual TTS (Korean, Japanese, English).

## 📦 Package Layout

```
snap_cpp/
├── include/              # Public C/C++ Header Files (snap/snap_api.h, etc.)
├── lib/                  # Prebuilt Binaries
│   ├── windows/x64/      # snap_cpp.dll, snap_cpp.lib, onnxruntime.dll
│   ├── linux/x64/        # libsnap_cpp.so, libonnxruntime.so (Linux AMD x64)
│   └── macos/            # libsnap_cpp.dylib (macOS Universal ARM64/Intel)
├── examples/             # Quickstart Examples & Test Scripts (test_e2e.cpp)
├── README.md             # Usage & Integration Guide
└── LICENSE
```

## 🚀 Quickstart Example (C++)

```cpp
#include "snap/snap_api.h"
#include <iostream>

int main() {
    // 1. Create engine instance (loads models from ./models directory for language "ko", "ja", or "en")
    void* handle = snap_create("./models", "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine.\n";
        return 1;
    }

    // 2. Process text to phonetic representation
    const char* text = "2024년 5월 28일 오후 3시에 만납시다.";
    char* result = snap_process(handle, text);

    if (result) {
        std::cout << "Input : " << text << "\n";
        std::cout << "Output: " << result << "\n";
        snap_free(result);
    }

    // 3. Destroy engine instance
    snap_destroy(handle);
    return 0;
}
```

## 🛠️ Building & Linking Guide

### 🐧 Linux (AMD x64)
```bash
g++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/linux/x64 -lsnap_cpp -Wl,-rpath,lib/linux/x64 -o test_e2e
./test_e2e
```

### 🍏 macOS (Universal ARM64 / Intel)
```bash
clang++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/macos -lsnap_cpp -Wl,-rpath,lib/macos -o test_e2e
./test_e2e
```

### 🪟 Windows (MSVC x64)
```cmd
cl /std:c++17 /Iinclude examples\test_e2e.cpp /link /LIBPATH:lib\windows\x64 snap_cpp.lib /out:test_e2e.exe
set PATH=lib\windows\x64;%PATH%
test_e2e.exe
```

## 📜 License

SNAP C++ SDK is released under the Apache-2.0 License.
