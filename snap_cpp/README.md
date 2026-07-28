# SNAP C++ SDK (Public Release)

High-performance, zero-dependency C/C++ Frontend Inference Engine for Multilingual TTS (Korean, Japanese, English).

---

## 📦 SDK Package Layout

```
snap_cpp/
├── include/              # Public C/C++ Header Files
│   └── snap/
│       ├── snap_api.h    # Main Inference API
│       └── snap_version.h# Version & ABI Compatibility API (v1.0.0)
├── lib/                  # Prebuilt Native Binaries & Version Catalog
│   ├── LIBRARIES.md      # Binary Catalog Documentation
│   ├── windows/x64/v1.0.0/ # Win32 DLL, Import Lib & ONNX Runtime
│   ├── linux/x64/v1.0.0/   # Linux AMD Shared Library (SO)
│   └── macos/v1.0.0/       # macOS Universal Dylib (ARM64/Intel)
├── examples/             # Quickstart Examples (test_e2e.cpp, version_check.cpp)
├── README.md             # Integration & Versioning Guide
└── LICENSE
```

---

## 📂 Hugging Face Model & Dictionary Layout

Models and lexicons downloaded from Hugging Face (`snap-libs/snap-models`) follow the Manifest-Driven Versioning layout:

```
models/
├── manifest.json                             # Root version & variant controller
├── ko/
│   ├── dictionaries/v1.0.0/                  # Independent Lexicon Versioning (dict_eng_merged.json, etc.)
│   └── model_variants/kcbert-base-int8/v1.0.0/ # Backbone Model & Heads (KR_number_classifier.onnx, etc.)
├── ja/
│   ├── dictionaries/v1.0.0/
│   └── model_variants/ja-kanji-bert-int8/v1.0.0/
└── en/
    ├── dictionaries/v1.0.0/
    └── model_variants/en-bert-base-int8/v1.0.0/
```

---

## 🚀 Quickstart Example (C++)

```cpp
#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <iostream>

int main() {
    // 1. Check SDK version
    std::cout << "[SNAP] Library Version: " << snap_version() << "\n";

    // 2. Create engine instance (Auto-detects active version from models/manifest.json)
    void* handle = snap_create("./models", "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine.\n";
        return 1;
    }

    // 3. Process text to phonetic representation
    const char* text = "2024년 5월 28일 오후 3시에 만납시다.";
    const char* result = snap_process(handle, text);

    if (result) {
        std::cout << "Input : " << text << "\n";
        std::cout << "Output: " << result << "\n";
        snap_free(result);
    }

    // 4. Destroy engine instance
    snap_destroy(handle);
    return 0;
}
```

---

## 🛠️ Building & Linking Guide

### 🐧 Linux (AMD x64)
```bash
g++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/linux/x64/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/linux/x64/v1.0.0 -o test_e2e
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

### 🍏 macOS (Universal ARM64 / Intel)
```bash
clang++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/macos/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/macos/v1.0.0 -o test_e2e
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

### 🪟 Windows (MSVC x64)
```cmd
cl /std:c++17 /Iinclude examples\test_e2e.cpp /link /LIBPATH:lib\windows\x64\v1.0.0 snap_cpp.lib /out:test_e2e.exe
set PATH=lib\windows\x64\v1.0.0;%PATH%
test_e2e.exe . ko "2024년 5월 28일 오후 3시에 만납시다."
```

---

## 📜 License

SNAP C++ SDK is released under the Apache-2.0 License.
