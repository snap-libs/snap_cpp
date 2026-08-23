# SNAP C++ SDK Installation & Quick Testing Guide

High-performance, zero-dependency C/C++ Frontend Inference Engine for Multilingual TTS (Korean, Japanese, English).

---

## 📋 System Prerequisites & Dependencies

Before using or building the SNAP C++ SDK, ensure your environment meets the following requirements:

### 1. Runtime Requirements (Prebuilt Binaries)
Prebuilt shared libraries (`snap_cpp.dll` / `libsnap_cpp.so`) use static C++ runtime linkage.

* **🪟 Windows (x64 / ARM64)**: Windows 10 / 11 / Server 2016+ (x64 & ARM64).
* **🐧 Linux (x64)**: GLIBC 2.27+ (Standard on Ubuntu 18.04+, RHEL 8+, Debian 10+).

### 2. Build Requirements (Building from Source)
If compiling `snap_cpp` from source code using CMake:

* **CMake**: Version 3.16 or higher (`cmake --version`)
* **Compiler**:
  - Windows: MSVC 2019 / 2022 (Visual Studio or C++ Build Tools)
  - Linux: GCC 9+ or Clang 10+ (C++17 support)

---

## ⚡ Quick Testing in 1 Minute

Follow these simple steps to build and test the SDK in under 1 minute:

### Step 1: One-Click Asset Initialization (`snap_init`)
Run the initialization script to automatically fetch model assets from Hugging Face into your target installation folder (e.g. `./models` or current workspace):

```bash
# 🐧 Linux (Zsh/Bash)
./scripts/snap_init.sh -y

# 🪟 Windows (PowerShell)
powershell -ExecutionPolicy Bypass -File .\scripts\snap_init.ps1 -Yes
```

### Step 2: Compile & Run by OS (Environment-Variable-Free)

#### 🐧 Linux (AMD x64)
```bash
# 1) Compile test runner
g++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/linux/x64/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/linux/x64/v1.0.0 -o test_e2e

# 2) Run inference test using explicit installation folder path "."
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

#### 🪟 Windows (MSVC x64)
```cmd
:: 1) Compile Example (Use /EHsc for C++ exception handling)
cl /EHsc /std:c++17 /Iinclude examples\test_e2e.cpp /link /LIBPATH:lib\windows\x64\v1.0.0 snap_cpp.lib /out:test_e2e.exe

:: 2) Copy DLLs & Execute with explicit target folder path "."
copy lib\windows\x64\v1.0.0\*.dll .
test_e2e.exe . ko "2024년 5월 28일 오후 3시에 만납시다."
```

---

## 📦 SDK Package Layout

```
snap_cpp/
├── bin/                  # Standalone TUI Option Tuner Executable (Manual: SNAP_OPTION_TUNER.md)
│   ├── snap-setup.exe    # Windows TUI Tuner Executable
│   └── snap-setup-linux  # Linux TUI Tuner Executable
├── scripts/              # One-click Asset Initializer Scripts
│   ├── snap_init.sh      # Linux Initialization Script
│   └── snap_init.ps1     # Windows PowerShell Initialization Script
├── include/              # Public C/C++ Header Files
│   └── snap/
│       ├── snap_api.h    # Main Inference API
│       └── snap_version.h# Version & ABI Compatibility API (v1.0.0)
├── lib/                  # Prebuilt Native Binaries & Version Catalog
│   ├── windows/x64/v1.0.0/ # Win32 DLL, Import Lib & ONNX Runtime (v1.18.1)
│   └── linux/x64/v1.0.0/   # Linux AMD Shared Library (SO)
├── examples/             # Quickstart Examples (test_e2e.cpp, version_check.cpp)
├── README.md             # Overview & Quickstart Guide
├── SNAP_SDK_INSTALL.md   # Integration & Installation Guide
├── SNAP_API_MANUAL.md   # C API Reference Manual
├── SNAP_OPTION_TUNER.md  # Optional TUI Option Tuner Guide
└── LICENSE
```

---

## 📂 Hugging Face Model & Dictionary Layout

Models and lexicons downloaded from Hugging Face (`softguy777/snap-weights`) follow the Manifest-Driven Versioning layout:

```
models/
├── snap_config.json                          # Global configuration (TTS target, options)
├── manifest.json                             # Root version & variant controller
├── ko/
│   ├── KO_model_index.json                   # Korean index specification
│   ├── KO_model_bert_int8.onnx               # Backbone & task probing heads
│   ├── KO_morph_head_trie.onnx               # Morphological POS tagger head
│   ├── KO_heteronym_head.onnx                # Heteronym neural classification head
│   ├── KO_dict_idioms.json                   # Idioms & loanword pronunciation lexicon
│   ├── KO_dict_loanwords.json                # Foreign loanword lexicon
│   ├── KO_dict_eng_merged.json               # English phonetic lexicon
│   ├── KO_dict_eng_custom.json               # Custom English pronunciation dictionary
│   └── KO_tokenizer.json                     # WordPiece tokenizer specification
├── ja/
│   ├── JA_model_index.json                   # Japanese index specification
│   ├── JA_model_bert_int8.onnx               # Backbone & task probing heads
│   ├── JA_yomi_head.onnx                     # Kanji reading prediction head
│   ├── JA_kanji_dict.json                    # Kanji reading dictionary
│   ├── JA_accent_dict.json                   # Pitch accent lexicon
│   ├── JA_custom_eng_dict_ja.json            # English-to-Katakana phonetic dictionary
│   └── JA_tokenizer.json                     # SentencePiece tokenizer specification
└── en/
    ├── EN_model_index.json                   # English index specification
    ├── EN_model_bert_int8.onnx               # Backbone & task probing heads
    ├── EN_heteronym_head.onnx                # Heteronym part-of-speech classifier head
    ├── EN_targets.json                       # Heteronym vocabulary target mapping
    └── EN_tokenizer.json                     # BPE tokenizer specification
```

---

## 🚀 Quickstart & Advanced Usage (C++)

### 1) Direct Explicit Folder Path Pattern (Recommended — Zero Env Dependency)
Pass the installation folder path directly to `snap_create` for 100% deterministic asset loading without environment variable side-effects:

```cpp
#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <iostream>

int main() {
    // 1. Pass explicit asset installation folder path (Recommended)
    const char* target_folder = "./models"; 
    void* handle = snap_create(target_folder, "ko");
    if (!handle) {
        std::cerr << "Failed to initialize SNAP engine from target folder: " << target_folder << "\n";
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
Pass specific target asset paths directly to instantiate concurrent handles for Korean, Japanese, and English:

```cpp
#include "snap/snap_api.h"
#include <iostream>

int main() {
    // 1. Allocate handles for KO, JA, and EN using explicit target folder path
    const char* target_folder = "./models";
    void* handle_ko = snap_create(target_folder, "ko");
    void* handle_ja = snap_create(target_folder, "ja");
    void* handle_en = snap_create(target_folder, "en");

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

## 📜 License & Dual Licensing

SNAP C++ SDK is licensed under a **Dual License** model:

1. **Open Source & Research (GNU AGPLv3)**:
   - Free for non-commercial, open-source projects, academic use, and research under the **GNU Affero General Public License v3 (AGPL-3.0)**.
   - Any service, cloud SaaS/API, or software built with or embedding SNAP must make its full source code available under AGPLv3.
2. **Commercial License**:
   - Required for proprietary, closed-source applications, on-premise solutions, or commercial cloud TTS services wishing to keep source code private.
   - For licensing terms, pricing, and inquiries, please contact: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
