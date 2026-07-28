# SNAP C++ SDK (공식 사용자 가이드)

다국어(한국어, 일본어, 영어) 음성 합성(TTS) 프론트엔드를 위한 초고속, 의존성 Zero C/C++ 추론 엔진 SDK입니다.

---

## ⚡ 1분 만에 테스트 시작하기 (Quick Testing)

새로 SDK를 다운받으신 후 README 가이드대로 1분 만에 테스트를 수행하는 방법입니다:

### 1단계: 모델 자산 준비
Hugging Face (`snap-libs/snap-models`) 저장소에서 모델 및 사전이 포함된 `models/` 폴더를 프로젝트 루트에 배치합니다.

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

:: 2) DLL 경로 지정 및 실행
set PATH=lib\windows\x64\v1.0.0;%PATH%
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

## 🚀 C++ 사용법 (Quickstart)

```cpp
#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <iostream>

int main() {
    // 1. SDK 라이브러리 버전 확인
    std::cout << "[SNAP] 라이브러리 버전: " << snap_version() << "\n";

    // 2. 엔진 인스턴스 생성 (models/manifest.json에서 활성 버전을 자동 감지)
    void* handle = snap_create("./models", "ko");
    if (!handle) {
        std::cerr << "SNAP 엔진 초기화 실패.\n";
        return 1;
    }

    // 3. 텍스트 정규화 및 음소 변환 실행
    const char* text = "2024년 5월 28일 오후 3시에 만납시다.";
    const char* result = snap_process(handle, text);

    if (result) {
        std::cout << "입력: " << text << "\n";
        std::cout << "출력: " << result << "\n";
        snap_free(result);
    }

    // 4. 엔진 인스턴스 해제
    snap_destroy(handle);
    return 0;
}
```

---

## 📜 License

SNAP C++ SDK는 Apache-2.0 라이선스로 제공됩니다.
