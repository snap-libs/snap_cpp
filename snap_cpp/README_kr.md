# SNAP TTS Frontend C++ Library (`snap_cpp`) - 한국어 가이드

SNAP(Speech Normalization and Pronunciation) C++ 라이브러리는 Bert를 사용하여 문장의 문맥과 의미에 따른 정밀한 발음 변화(G2P)를 처리하는 고성능 TTS 프론트엔드 엔진입니다. 

초기에는 VITS TTS 모델 내부에서 연산된 BERT 구조를 그대로 재활용하여 동작하도록 설계되었습니다. 그러나 지속적인 경량화와 최적화를 거쳐, **현재는 BERT를 사용하지 않는 일반적인 전통적/신경망 TTS 엔진에서도 프론트엔드로 독립적으로 활용할 수 있을 만큼 가볍고 완결된 구조**를 갖추었습니다. 

특히 기존의 복잡한 딥러닝 패키지(`transformers` 등)와 형태소 분석기(`MeCab`, `g2pk` 등)에 대한 외부 의존성을 완전히 탈피했습니다. 결과적으로 **단 2개의 빌드 바이너리(`snap_cpp` & `onnxruntime`)와 가중치 데이터 및 사전 파일**만으로 한국어, 일본어, 영어의 전처리 및 G2P 변환을 문자 단위 100%의 동일성으로 고속 처리하는 초경량 독립형 런타임을 제공합니다.

---

## 1. 주요 특징

* **BERT 및 다중 신경망 헤드를 통한 고품질 G2P**: 기존 BERT 모델을 동결(Frozen)하고, 각 언어적 요구 사항에 맞게 학습된 다중 의미론적 헤드(Semantic Heads)들을 추가하여 정교한 발음을 계산합니다. 이 신경망 헤드들은 각각 기존 MeCab을 대치하는 형태소 분석, 문장 의미에 따른 발음/악센트 판별, 동음이의어 발음 교정 등을 전담하여 차원이 높은 음소 표현력을 실현합니다.
* **3개 국어 예제 제공 및 높은 확장성**: 기본 예시로 한국어, 일본어, 영어 3개 국어의 전용 BERT 모델과 ONNX 가중치 세트를 학습하여 제공합니다. 가중치 모델들은 [허깅페이스 리포지토리](https://huggingface.co/softguy777/snap-weights)에서 내려받을 수 있으며, 동일한 아키텍처 규칙을 응용하여 다른 언어나 임의의 커스텀 BERT 모델로 쉽게 확장 및 학습시킬 수 있습니다.
* **의존성 없는 초경량 독립 구동**: 모든 백본 모델을 Dynamic INT8으로 양자화하여 파일 크기를 100MB 내외로 컴팩트하게 압축했습니다. 양자화 모델을 사용하지만, 각 의미론적 헤드(Semantic Heads)들에 필요한 정보는 충분히 전달되어 FP32 모델과 거의 같은 실험 결과를 얻었습니다.
* **극도로 간결한 C-Compatible API**: 단 4개의 C-Linkage API 함수만 제공하여 C#, Rust, Go, Python 등 다양한 언어 바인딩 및 Unity/Unreal 같은 게임 엔진에 즉시 탑재할 수 있습니다.

---

## 2. 퀵 테스트 및 Windows 퀵 스타트 (Quick Start for Windows)

빌드 완료된 `test_e2e` 실행 파일이나 제공되는 퀵스타트 배치 파일을 사용하여 텍스트 분석 및 G2P 반환 결과(JSON)를 간편히 확인해 볼 수 있습니다.

### 💡 Quick Start for Windows: 원클릭(One-Click) 즉시 테스트 (강력 추천 🚀)
Windows 환경에서는 **파이썬(Python) 및 추가 라이브러리 설치가 전혀 필요 없이**, 배치 파일 실행만으로 다운로드부터 테스트 구동까지 100% 완전 자동화하여 즉시 테스트할 수 있습니다.

각 언어별 배치 파일을 기동하여 바로 가동합니다:
* 🇰🇷 **[run_quick_test_ko.bat](run_quick_test_ko.bat) (한국어)**: 한국어 전용 경량 패키지(약 79 MB)를 내려받아 `안과에 갔다.` 문장의 G2P 변환을 테스트합니다.
* 🇯🇵 **[run_quick_test_ja.bat](run_quick_test_ja.bat) (일본어)**: 일본어 전용 경량 패키지(약 85 MB)를 내려받아 `彼女は料理が上手だ。` 문장의 G2P/악센트 변환을 테스트합니다.
* 🇺🇸 **[run_quick_test_en.bat](run_quick_test_en.bat) (영어)**: 영어 전용 경량 패키지(약 65 MB)를 내려받아 `Hello, this is a quickstart test.` 문장의 G2P 변환을 테스트합니다.

**자동 진행 내역 (Zero Dependency)**:
* 윈도우 기본 PowerShell 기능을 통해 허깅페이스에서 언어별 퀵스타트 압축 패키지를 직접 다운로드받아 자동으로 압축 해제하고 임시 다운로드 파일을 제거합니다.
* 사전 컴파일된 바이너리(`test_e2e.exe`)를 활용해 곧바로 지정 문장의 G2P 변환 결과(JSON)를 터미널에 즉각 출력해 줍니다.

---

### 3분 퀵 스타트 최소 검증 가이드 (소스 컴파일 빌드용)

가장 가볍고 빠르게 한국어 INT8 모델 조합(~110MB)을 직접 컴파일하여 엔진을 기동하고 결과를 대조해 보는 표준 절차입니다:

1. **프로젝트 빌드 실행**:
   ```bash
   # Windows MSVC
   cmake -B build -S .
   cmake --build build --config Release

   # Linux GCC
   cmake -B build_linux -S .
   cmake --build build_linux --config Release

   # macOS Clang
   cmake -B build_osx -S .
   cmake --build build_osx --config Release
   ```

2. **한국어 INT8 리소스만 신속 다운로드**:
   ```bash
   pip install huggingface_hub
   huggingface-cli download softguy777/snap-weights --local-dir ../snap/weights --include "ko/*" --exclude "ko/model.onnx*"
   ```

3. **양자화 설정 및 테스트 구동**:
   `../snap/weights/ko/snap_config.json`을 텍스트 에디터로 열어 `"bert_model": "model.onnx"` 값을 `"bert_model": "model_int8.onnx"`로 수정하고 저장합니다.
   
   이후 터미널에서 다음 테스트 명령을 날려 결과를 확인합니다:
   ```bash
   # Windows (CMD / UTF-8 환경 전환)
   cmd.exe /c "chcp 65001 && echo 안과에 갔다. | build\Release\test_e2e.exe ../snap/weights ko"
   
   # Linux / WSL
   echo "안과에 갔다." | ./build_linux/test_e2e ../snap/weights ko

   # macOS
   echo "안과에 갔다." | ./build_osx/test_e2e ../snap/weights ko
   ```
   * **기대 결과**: 콘솔 화면에 발음 정규화 결과물인 `"phonology":"안과에 갇따. "`가 정상적으로 박힌 JSON 출력창이 나타나면 테스트가 성공한 것입니다.

---

### 배치 모드 (Batch Mode)
전달받은 단일 문장의 G2P 결과를 터미널에 반환하고 즉시 종료됩니다:
```bash
# Windows
build\Release\test_e2e.exe <weights_dir> <lang> "<text>"

# 실행 예시 (한국어 G2P):
build\Release\test_e2e.exe ../snap/weights ko "안과에 갔다."

# Linux / WSL
./build_linux/test_e2e ../snap/weights ko "안과에 갔다."

# macOS
./build_osx/test_e2e ../snap/weights ko "안과에 갔다."
```

### 대화형 CLI 모드 (Interactive CLI Mode)
세 번째 문장 인자를 생략하고 기동하면, 지속적으로 텍스트 입력을 입력받아 처리하는 콘솔 프롬프트 루프가 활성화됩니다:
```bash
# 일본어 프롬프트 루프 기동
build\Release\test_e2e.exe ../snap/weights ja

# 입력 예시:
# [snap_cpp:ja]> 彼女は料理が上手だ
# {"accent_overrides":{"カノジョ":1,"リョウリ":1,"上手":0}, ... }
#
# 프롬프트 루프를 마치려면 'exit' 또는 'quit'을 입력하고 엔터를 치세요.
```
> **Windows 콘솔 사용자 대상 안내**: Windows의 CMD 및 PowerShell 셸은 UTF-8 인자를 넘길 때 글자가 깨지기 쉽습니다. 따라서 인코딩이 정상적으로 디코딩되도록 **대화형 CLI 모드**로 진입해 입력하거나, 셸 코드페이지를 UTF-8로 선제 변경(`chcp 65001`)한 뒤 파이프라인 구조(예: `echo 안과에 갔다. | test_e2e.exe ...`)로 문자열을 흘려주는 방식을 강력히 권장합니다.

---

## 3. 프로젝트 디렉토리 구조

```
snap_cpp/
├── CMakeLists.txt             # CMake 빌드 구성 파일
├── README.md                  # 영문 메인 가이드
├── README_kr.md               # 한글 가이드 (본 파일)
├── test_e2e.cpp               # 대화형 및 배치식 E2E 테스트 러너 소스
├── include/
│   └── snap/
│       ├── bert_session.h     # ONNX Runtime BERT 세션 래퍼
│       ├── classifier.h       # 신경망 헤드 분류 오케스트레이터
│       ├── phonology.h        # 한국어 음운 규칙 및 전처리 엔진
│       ├── phonology_ja.h     # 일본어 음운 규칙 및 악센트 오버라이드 엔진
│       ├── tokenizer.h        # SentencePiece 토크나이저 래퍼
│       └── snap_api.h         # C-Linkage 내보내기 API 정의 헤더
└── src/
    ├── bert_session.cpp
    ├── classifier.cpp
    ├── phonology.cpp
    ├── phonology_ja.cpp
    ├── tokenizer.cpp
    └── snap_api.cpp
```

---

## 4. 시스템 요구사항 및 종속성

### 개발 도구 및 컴파일러 사양
* **C++ 컴파일러**: **C++17** 표준 규격을 지원하는 컴파일러가 필요합니다.
  * **Windows**: Visual Studio 2019 / 2022 (MSVC v142 이상)
  * **Linux/WSL**: GCC 9 이상 또는 Clang 10 이상
  * **macOS**: Xcode 12.5 이상 (Apple Clang)
* **빌드 시스템**: CMake 3.15 이상

### 외부 패키지 자동 구성
본 CMake 빌드 프로젝트는 의존성 해결을 위해 `FetchContent` 메커니즘을 사용합니다. 빌드 구동 시 아래 패키지들이 자동으로 다운로드 및 배치됩니다:
1. **nlohmann/json (v3.11.3)**: 헤더 온리 라이브러리로 자동 다운로드 및 링크됩니다.
2. **ONNX Runtime C++ SDK (v1.18.1)**: 빌드를 수행하는 운영체제 환경에 부합하는 SDK 버전을 탐색하여 자동으로 획득합니다:
   * **Windows**: `onnxruntime-win-x64-1.18.1.zip`
   * **Linux/WSL**: `onnxruntime-linux-x64-1.18.1.tgz`
   * **macOS**: `onnxruntime-osx-universal-1.18.1.tgz`

---

## 5. 빌드 방법

### Windows (MSVC x64)

1. **CMake 빌드 구성**:
   ```bash
   cmake -B build -S .
   ```
2. **Release 모드로 컴파일**:
   ```bash
   cmake --build build --config Release
   ```
   * 컴파일이 완료되면 `build/Release/` 경로에 `snap_cpp.dll` 공유 라이브러리와 `test_e2e.exe` 실행 파일이 생성됩니다.
   * 빌드 포스트 프로세스(`POST_BUILD`)에 의해 의존 라이브러리인 `onnxruntime.dll`도 출력 디렉터리로 자동 복사됩니다.

### Linux / WSL (GCC)

1. **CMake 빌드 구성**:
   ```bash
   cmake -B build_linux -S .
   ```
2. **Release 모드로 컴파일**:
   ```bash
   cmake --build build_linux --config Release
   ```
   * 빌드 완료 후 `build_linux/` 경로에 `libsnap_cpp.so` 공유 라이브러리와 `test_e2e` 실행 파일이 생성됩니다.
   * 의존 라이브러리인 `libonnxruntime.so`도 해당 경로로 자동 복사됩니다.

### macOS (Clang)

1. **CMake 빌드 구성**:
   ```bash
   cmake -B build_osx -S .
   ```
2. **Release 모드로 컴파일**:
   ```bash
   cmake --build build_osx --config Release
   ```
   * 빌드 완료 후 `build_osx/` 경로에 `libsnap_cpp.dylib` 공유 라이브러리와 `test_e2e` 실행 파일이 생성됩니다.
   * 의존 라이브러리인 `libonnxruntime.dylib`도 해당 경로로 자동 복사됩니다.

---

## 6. 모델 가중치 다운로드

대용량의 원본 ONNX 모델 및 사전 파일들은 별도로 관리되며 허깅페이스(Hugging Face)에 호스팅되어 있습니다. 컴파일 완료 후 테스트 실행 파일을 가동하기 전에 가중치 파일들을 다운로드해야 합니다.

### 모델 정밀도 옵션 (FP32 vs INT8)
각 언어별 가중치 디렉토리 내부에는 두 가지 정밀도 타입의 BERT 백본 모델이 함께 배치되어 있습니다:
* **`model.onnx` (FP32)**: 오리지널 단밀도(Full-precision) 모델 (~770MB ~880MB)로, 왜곡 없는 최대치의 분석 정확도를 냅니다.
* **`model_int8.onnx` (INT8)**: 동적 양자화(Dynamic Quantized) 모델 (~90MB)로, 메모리 점유율을 약 1/8 수준으로 극단적으로 경량화하고 CPU 연산 속도를 크게 높여줍니다. **G2P 변환 정확도는 FP32 대비 거의 손실되지 않고 동일한 수준을 유지합니다.**

개발 타겟 애플리케이션의 목표(초경량 최적화 vs 최상의 정밀도)에 맞추어 모델을 선택해 적용하세요. 모델 토글은 언어별 경로에 존재하는 `snap_config.json`의 `"bert_model"` 키 값을 수정하여 즉시 변경할 수 있습니다:
```json
// 예시: weights/ko/snap_config.json
{
    "bert_model": "model_int8.onnx", // FP32를 쓰려면 "model.onnx"로, INT8로 교체하려면 "model_int8.onnx"로 기입
    "bert_model_id": "kykim/bert-kor-base",
    ...
}
```

### 다운로드 방법 및 선택적 다운로드 가이드

전체 리포지토리의 용량은 **약 2.32 GB**이지만, 이를 모두 받을 필요는 없습니다. 사용하려는 언어나 정밀도(INT8 전용 등)만 필터링하여 **선택적으로 다운로드(Selective Download)**하여 디스크 용량과 대역폭을 아낄 수 있습니다.

#### 옵션 A: huggingface-cli를 통한 선택적 다운로드 (추천)
로컬에 `huggingface_hub` 패키지가 설치되어 있어야 합니다: `pip install huggingface_hub`.

* **한국어(`ko`) 리소스만 다운로드하려는 경우 (약 980 MB)**:
  ```bash
  huggingface-cli download softguy777/snap-weights --local-dir ../snap/weights --include "ko/*"
  ```
* **모든 국어의 무거운 FP32 모델을 제외하고 가벼운 INT8 및 사전 리소스만 받으려는 경우 (약 300 MB)**:
  `model.onnx` 및 `model.onnx.data` 다운로드를 스킵합니다:
  ```bash
  huggingface-cli download softguy777/snap-weights --local-dir ../snap/weights --exclude "*/model.onnx*"
  ```
* **한국어의 가벼운 INT8 가중치 데이터 세트만 받으려는 경우 (약 110 MB)**:
  ```bash
  huggingface-cli download softguy777/snap-weights --local-dir ../snap/weights --include "ko/*" --exclude "ko/model.onnx*"
  ```
* **전체 가중치(FP32 + INT8 전부)를 받으려는 경우 (약 2.32 GB)**:
  ```bash
  huggingface-cli download softguy777/snap-weights --local-dir ../snap/weights
  ```

#### 옵션 B: Git LFS를 사용한 전체 클론
FP32 및 INT8을 포함한 전체 2.32 GB의 리포지토리를 복제하려면:
```bash
git lfs install
git clone https://huggingface.co/softguy777/snap-weights ../snap/weights
```

---

## 7. API 명세

`snap_cpp` 라이브러리는 타 애플리케이션 및 플랫폼에 수월하게 적재할 수 있도록 심플한 C-Linkage API 인터페이스를 지원합니다:

```cpp
#ifdef __cplusplus
extern "C" {
#endif

/**
 * SNAP 엔진을 초기화하고 지정된 언어의 BERT 및 관련 신경망 헤드 세션을 로드합니다.
 * @param weights_dir  모델 가중치들이 들어있는 최상위 폴더 (내부에 ko, ja, en 하위 경로 포함)
 * @param lang         기동 대상 언어 코드 ("ko", "ja" 또는 "en")
 * @return             성공 시 0, 모델 로드 실패 시 -1 반환
 */
int snap_init(const char* weights_dir, const char* lang);

/**
 * 입력받은 UTF-8 일반 문자열을 대상으로 전처리 및 G2P 예측을 수행합니다.
 * @param text_utf8    분석할 UTF-8 인코딩 일반 텍스트
 * @return             결과 JSON 데이터를 담은 C-String (호출한 측에서 반드시 snap_free로 메모리를 해제해야 함)
 */
const char* snap_process(const char* text_utf8);

/**
 * snap_process 반환 문자열 버퍼에 할당되었던 메모리를 해제합니다.
 * @param result       snap_process가 반환했던 포인터 주소
 */
void snap_free(const char* result);

/**
 * SNAP 엔진 작동을 정지하고 할당된 ONNX 런타임 세션 및 사전 메모리를 모두 안전하게 해제합니다.
 */
void snap_shutdown();

#ifdef __cplusplus
}
#endif
```

---

## 8. 경량 배포 가이드

실제 프로덕션 상용 서비스 배포 시 저장소 용량을 극적으로 아끼기 위해, **사용하지 않는 언어의 리소스 폴더는 통째로 제외**하고 구성할 수 있습니다:
```
weights/
└── ko/                      # 한국어 서비스만 운영 시 ko/ 디렉토리만 유지 (ja/, en/ 폴더 삭제 가능)
    ├── model.onnx           # BERT 모델 본체
    ├── snap_config.json
    ├── heteronym_targets.json
    ├── morph_words.bin
    └── ...
```
이 상태에서 `snap_init(weights_dir, "ko")`를 호출하면 다른 언어 디렉토리가 없더라도 문제없이 성공적으로 구동됩니다.

---

## 9. 문제 해결 (Troubleshooting)

### 1. Windows 콘솔(CMD/PowerShell)에서 한글/일본어가 깨져서 출력되거나 결과가 나지 않는 현상
* **원인**: Windows의 셸 기본 인코딩 값(CP949 등 ANSI 코드페이지)이 C++ API의 UTF-8 문자열 해석 구조와 충돌하여 발생합니다.
* **해결 방법**:
  * CMD 창의 인코딩을 UTF-8 코드페이지인 `chcp 65001`을 입력해 전환한 후 실행하세요.
  * 보다 안전하게 파이프라인 형태(`echo 문장 | test_e2e`)로 표준 입력을 흘려보내 테스트해 주세요.
  * PowerShell을 쓴다면 아래 환경 변수를 셸 세션에 정의해 줍니다:
    ```powershell
    $OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::InputEncoding = [System.Text.Encoding]::UTF8
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    ```

### 2. DLL / 공유 라이브러리 로드 오류 ("Cannot find `onnxruntime.dll`" 또는 "Error loading `libonnxruntime.so`/`libonnxruntime.dylib`")
* **원인**: 컴파일된 바이너리가 런타임에 의존하는 ONNX Runtime shared library 파일이 환경 변수(`PATH`, `LD_LIBRARY_PATH` 또는 `DYLD_LIBRARY_PATH`) 상에 없거나 실행 파일과 같은 경로에 없을 때 발생합니다.
* **해결 방법**:
  * **Windows**: CMake 빌드 스크립트의 포스트 빌드 룰에 의해 `onnxruntime.dll`이 자동으로 실행 파일 경로로 복사됩니다. 직접 빌드한 실행 바이너리를 이식할 때는 `snap_cpp.dll`과 `onnxruntime.dll`이 실행 주체인 `.exe` 파일과 반드시 동일 디렉토리에 나란히 있어야 합니다.
  * **Linux/WSL**: 빌드 출력 폴더를 시스템의 동적 링킹 로더 검색 경로에 주입해 줍니다:
    ```bash
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/build_linux
    ./build_linux/test_e2e ../snap/weights ko "안과에 갔다."
    ```
  * **macOS**: 빌드 출력 폴더를 동적 링킹 라이브러리 검색 경로에 주입해 줍니다:
    ```bash
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/path/to/build_osx
    ./build_osx/test_e2e ../snap/weights ko "안과에 갔다."
    ```

### 3. 프로그램 기동 시 즉시 크래시(exit code -1 반환하며 `snap_init` 로드 에러 출력)
* **원인**: `snap_init`에 입력한 `weights_dir` 주소가 잘못되었거나 가중치 디렉토리 내부에서 사전 및 모델이 누락되어 로드에 실패한 상태입니다.
* **해결 방법**:
  * 전달한 경로 하위에 실제 기동할 언어 디렉토리(예: `ko/`)가 직접 존재하고 있는지, 해당 폴더 안에 `snap_config.json`, `tokenizer.json`, `model.onnx`, `model.onnx.data` 등의 핵심 파일들이 온전히 다 채워져 있는지 경로를 정밀히 체크하세요. 누락된 파일이 있다면 **Section 5: 모델 가중치 다운로드** 가이드를 참고하여 허깅페이스에서 온전한 버전을 다시 받아 배치하십시오.

---

## 10. 라이선스 (License)

이 프로젝트는 **GNU General Public License v3.0 (GPL-3.0)** 하에 배포됩니다.
* 개인적 사용, 교육, 연구 목적으로는 자유롭게 수정 및 복제, 배포가 가능합니다.
* 이 라이선스가 적용된 코드를 수정 및 활용하여 배포하거나 서비스를 제공하는 경우, 해당 파생 소프트웨어의 전체 소스 코드 또한 GPL-3.0에 따라 공개해야 하는 의무(Copyleft)가 발생합니다.
* 소스 코드 공개 의무 없이 상업적 용도로 비공개 임베딩 및 제품화를 하고자 하시는 경우, 별도의 상용 라이선스 계약(Dual License)이 필요합니다. 상세 문의는 메인 관리자에게 연락 바랍니다.

자세한 내용은 [LICENSE](file:///c:/work/RaconVoice/RaconVoice_V6/snap_cpp/LICENSE) 파일을 참고하십시오.
