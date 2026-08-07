# SNAP C++ SDK (공식 사용자 가이드)

다국어(한국어, 일본어, 영어) 음성 합성(TTS) 프론트엔드를 위한 초고속, 의존성 Zero C/C++ 추론 엔진 SDK입니다.

---

## ⚡ 1분 만에 테스트 시작하기 (Quick Testing)

새로 SDK를 다운받으신 후 README 가이드대로 1분 만에 테스트를 수행하는 방법입니다:

### 1단계: 원클릭 모델 자산 초기화 (`snap_init`)
초기화 스크립트를 실행하여 Hugging Face (`softguy777/snap-models`) 저장소로부터 모델 및 사전 자산을 프로젝트 루트에 자동으로 다운로드하고 환경변수(`SNAP_HOME`)를 설정합니다.

```bash
# 🐧 Linux / 🍏 macOS (Zsh/Bash)
./scripts/snap_init.sh

# 🪟 Windows (PowerShell)
powershell -ExecutionPolicy Bypass -File .\scripts\snap_init.ps1
```

### 2단계: OS별 1초 컴파일 및 구동 테스트

#### 🐧 Linux (AMD x64)
```bash
# 1) 예제 컴파일
g++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/linux/x64/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/linux/x64/v1.0.0 -o test_e2e

# 2) 한국어 텍스트 추론 실행
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

#### 🪟 Windows (MSVC x64)
```cmd
:: 1) 예제 컴파일
cl /std:c++17 /Iinclude examples\test_e2e.cpp /link /LIBPATH:lib\windows\x64\v1.0.0 snap_cpp.lib /out:test_e2e.exe

:: 2) DLL 복사 및 실행 (PowerShell, CMD 공통 100% 안전)
copy lib\windows\x64\v1.0.0\*.dll .
test_e2e.exe . ko "2024년 5월 28일 오후 3시에 만납시다."
```

#### 🍏 macOS (Universal ARM64 / Intel)
```bash
# 1) 예제 컴파일
clang++ -std=c++17 examples/test_e2e.cpp -Iinclude -Llib/macos/v1.0.0 -lsnap_cpp -Wl,-rpath,lib/macos/v1.0.0 -o test_e2e

# 2) 실행
./test_e2e . ko "2024년 5월 28일 오후 3시에 만납시다."
```

---

## 📦 SDK 패키지 구조

```
snap_cpp/
├── bin/                  # 사전 빌드 TUI 설정 관리자 도구 (매뉴얼: setup/SNAP_SETUP_MANUAL.md)
│   ├── snap-setup.exe    # Windows용 TUI 관리 프로그램
│   └── snap-setup-linux  # Linux / macOS용 TUI 관리 프로그램
├── scripts/              # 원클릭 자산 및 환경 초기화 스크립트
│   ├── snap_init.sh      # macOS / Linux 초기화 스크립트
│   └── snap_init.ps1     # Windows PowerShell 초기화 스크립트
├── include/              # 공개 C/C++ 헤더 파일
│   └── snap/
│       ├── snap_api.h    # 메인 추론 API
│       └── snap_version.h# 라이브러리 버전 및 ABI 호환성 API (v1.0.0)
├── lib/                  # 사전 빌드 네이티브 바이너리 카탈로그
│   ├── LIBRARIES.md      # 라이브러리 카탈로그 명세
│   ├── windows/x64/v1.0.0/ # Win32 DLL, Import Lib 및 ONNX Runtime (v1.18.1)
│   ├── linux/x64/v1.0.0/   # Linux AMD 공유 라이브러리 (SO)
│   └── macos/v1.0.0/       # macOS 유니버설 Dylib (ARM64/Intel)
├── examples/             # 예제 코드 (test_e2e.cpp, version_check.cpp)
├── README_kr.md          # 한국어 연동 가이드
└── LICENSE
```

---

## 📂 Hugging Face 모델 & 사전 디렉터리 레이아웃

Hugging Face (`snap-libs/snap-models`)에서 다운로드받은 모델 및 사전 파일은 매니페스트 기반 버저닝 구조를 따릅니다:

```
models/
├── manifest.json                             # 최상위 활성 버전 및 백본 제어 매니페스트
├── ko/
│   ├── dictionaries/v1.0.0/                  # 독립 발음 사전 (dict_eng_merged.json 등)
│   └── model_variants/kcbert-base-int8/v1.0.0/ # BERT 백본 및 헤더 (KR_number_classifier.onnx 등)
├── ja/
│   ├── dictionaries/v1.0.0/
│   └── model_variants/ja-kanji-bert-int8/v1.0.0/
└── en/
    ├── dictionaries/v1.0.0/
    └── model_variants/en-bert-base-int8/v1.0.0/
```

---

## 🚀 C++ 사용법 및 고급 가이드 (Quickstart)

### 1) 기본 패턴 (`snap init` 기반 `SNAP_HOME` 사용 - 권장)
`snap_init` 스크립트를 통해 설정된 `SNAP_HOME` 환경변수를 사용하여 엔진을 생성합니다:

```cpp
#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <iostream>

int main() {
    // Passing nullptr or "" automatically uses SNAP_HOME environment variable (Recommended).
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

### 2) 다중 언어 (한국어 / 일본어 / 영어) 동시 운용 예시
동일한 `SNAP_HOME` 자산 루트에서 여러 언어(`ko`, `ja`, `en`)의 엔진 인스턴스를 동시에 생성하여 멀티링구얼 음성 합성 프론트엔드를 독립적으로 구동할 수 있습니다:

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

SNAP C++ SDK는 Apache-2.0 라이선스로 제공됩니다.
