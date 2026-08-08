# SNAP C++ SDK Installation & Quick Testing Guide

High-performance, zero-dependency C/C++ Frontend Inference Engine for Multilingual TTS (Korean, Japanese, English).

---

## ⚡ Quick Testing in 1 Minute

Follow these simple steps to build and test the SDK in under 1 minute:

### Step 1: One-Click Asset & Environment Initialization (`snap_init`)
Run the initialization script to automatically fetch model assets from Hugging Face (`softguy777/snap-models`) and configure the `SNAP_HOME` environment variable:

```bash
# 🐧 Linux / 🍏 macOS (Zsh/Bash)
./scripts/snap_init.sh

# 🪟 Windows (PowerShell)
powershell -ExecutionPolicy Bypass -File .\scripts\snap_init.ps1
```

### Step 2: Compile & Run by OS

#### 🐧 Linux (AMD x64)
```bash
# 1) Compile test runner
g++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/linux/x64/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/linux/x64/v1.0.0 -o test_e2e

# 2) Run inference test
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

#### 🪟 Windows (MSVC x64)
```cmd
:: 1) Compile Example
cl /std:c++17 /Iinclude examples\test_e2e.cpp /link /LIBPATH:lib\windows\x64\v1.0.0 snap_cpp.lib /out:test_e2e.exe

:: 2) Copy DLLs & Execute (Safe across PowerShell and CMD)
copy lib\windows\x64\v1.0.0\*.dll .
test_e2e.exe . ko "2024년 5월 28일 오후 3시에 만납시다."
```

#### 🍏 macOS (Universal ARM64 / Intel)
```bash
# 1) Compile test runner
clang++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/macos/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/macos/v1.0.0 -o test_e2e

# 2) Run inference test
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

---

## 📦 SDK Package Layout

```
snap_cpp/
├── bin/                  # Prebuilt TUI Setup Manager Tool (Manual: setup/SNAP_SETUP_MANUAL.md)
│   ├── snap-setup.exe    # Windows TUI Setup Executable
│   └── snap-setup-linux  # Linux / macOS TUI Setup Executable
├── scripts/              # One-click Asset Initializer Scripts
│   ├── snap_init.sh      # macOS / Linux Initialization Script
│   └── snap_init.ps1     # Windows PowerShell Initialization Script
├── include/              # Public C/C++ Header Files
│   └── snap/
│       ├── snap_api.h    # Main Inference API
│       └── snap_version.h# Version & ABI Compatibility API (v1.0.0)
├── lib/                  # Prebuilt Native Binaries & Version Catalog
│   ├── LIBRARIES.md      # Binary Catalog Documentation
│   ├── windows/x64/v1.0.0/ # Win32 DLL, Import Lib & ONNX Runtime (v1.18.1)
│   ├── linux/x64/v1.0.0/   # Linux AMD Shared Library (SO)
│   └── macos/v1.0.0/       # macOS Universal Dylib (ARM64/Intel)
├── examples/             # Quickstart Examples (test_e2e.cpp, version_check.cpp)
├── README.md             # Overview & Quickstart Guide
├── INSTALL.md            # Integration & Installation Guide
├── SNAP_API_MANUAL.md   # C API Reference Manual
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

## 🚀 Quickstart & Advanced Usage (C++)

### 1) Standard Pattern (`snap init` + `SNAP_HOME` — Recommended)
Pass `NULL` or `nullptr` to automatically use the `SNAP_HOME` environment variable configured by `snap_init`:

```cpp
#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <iostream>

int main() {
    // Passing nullptr automatically resolves SNAP_HOME environment variable (Recommended)
    void* handle = snap_create(nullptr, "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine.\n";
        return 1;
    }

    const char* text = "2024년 5월 28일 오후 3시에 만납시다.";
    const char* result = snap_process(handle, text);

    if (result) {
        std::cout << "Output: " << result << "\n";
        snap_free(result);
    }

    snap_destroy(handle);
    return 0;
}
```

### 2) Concurrent Multilingual (KO / JA / EN) Usage
From a single `SNAP_HOME` asset root, you can instantiate concurrent handles for Korean, Japanese, and English to build a multilingual TTS frontend:

```cpp
#include "snap/snap_api.h"
#include <iostream>

int main() {
    // 1. Allocate handles for KO, JA, and EN concurrently (using SNAP_HOME)
    void* handle_ko = snap_create(nullptr, "ko");
    void* handle_ja = snap_create(nullptr, "ja");
    void* handle_en = snap_create(nullptr, "en");

    if (!handle_ko || !handle_ja || !handle_en) {
        std::cerr << "Failed to initialize engine handles!\n";
        return 1;
    }

    // 2. Process text per language
    const char* res_ko = snap_process(handle_ko, "오후 3시에 만납시다.");
    const char* res_ja = snap_process(handle_ja, "午後3時に会いましょう。");
    const char* res_en = snap_process(handle_en, "Let's meet at 3 PM.");

    if (res_ko) { std::cout << "[KO] " << res_ko << "\n"; snap_free((void*)res_ko); }
    if (res_ja) { std::cout << "[JA] " << res_ja << "\n"; snap_free((void*)res_ja); }
    if (res_en) { std::cout << "[EN] " << res_en << "\n"; snap_free((void*)res_en); }

    // 3. Destroy handles
    snap_destroy(handle_ko);
    snap_destroy(handle_ja);
    snap_destroy(handle_en);
    return 0;
}
```

---

## 📜 License

SNAP C++ SDK is released under the Apache-2.0 License.
