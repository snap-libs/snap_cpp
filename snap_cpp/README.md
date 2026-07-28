# SNAP C++ SDK (Public Release)

High-performance, zero-dependency C/C++ Frontend Inference Engine for Multilingual TTS (Korean, Japanese, English).

## 📦 Package Layout

```
snap_cpp/
├── include/              # Public C/C++ Header Files (snap.h, etc.)
├── lib/                  # Prebuilt Binaries
│   ├── windows/x64/      # snap_cpp.dll, snap_cpp.lib, onnxruntime.dll
│   └── linux/x64/        # libsnap_cpp.so, libonnxruntime.so
├── examples/             # Quickstart Examples & Test Scripts (test_e2e.cpp)
├── README.md             # Usage & Integration Guide
└── LICENSE
```

## 🚀 Quickstart Example (C++)

```cpp
#include "snap/snap_api.h"
#include <iostream>

int main() {
    // 1. Create engine instance (loads models from ./models directory)
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

## 📜 License

SNAP C++ SDK is released under the Apache-2.0 License.
