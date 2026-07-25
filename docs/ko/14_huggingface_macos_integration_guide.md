# macOS (Apple Silicon & Intel) & Hugging Face 모델 연동/테스트 가이드

이 문서는 **Hugging Face Hub**에 등록된 `snap` TTS 프론트엔드 모델 가중치를 **macOS (Apple Silicon M1/M2/M3/M4 및 Intel Mac)** 환경에서 다운로드받고 Python / C++ 엔진으로 독립 실행 및 테스트하는 방법을 안내합니다.

---

## 1. 개요 및 macOS 호환성 특징

- **Universal2 ONNX Runtime SDK 지원**: Apple Silicon (arm64) 및 Intel (x86_64) 맥 아키텍처를 동시에 지원하는 `onnxruntime-osx-universal2` 바이너리가 빌드 타임에 자동 다운로드됩니다.
- **`.dylib` 동적 라이브러리**: macOS 환경에 최적화된 `libsnap_cpp.dylib`를 컴파일하고 C++ C-API(`snap_create`, `snap_process`)를 통해 Python ctypes 및 C++ 응용 프로그램과 연동할 수 있습니다.
- **Strict Exit Policy**: 누락되거나 잘못된 가중치 디렉터리가 주어지면 섣부른 Fallback 없이 즉시 `FileNotFoundError` / `Strict Policy Error`를 발생시켜 오작동을 방지합니다.

---

## 2. 사전 준비 사항 (macOS)

macOS에 `brew` 및 C++ 컴파일러(Xcode Command Line Tools), Python 패키지가 준비되어 있어야 합니다:

```bash
# Xcode 커맨드라인 툴 및 CMake 설치
xcode-select --install
brew install cmake git

# Python 의존성 설치
pip install huggingface_hub onnxruntime tokenizers num2words
```

---

## 3. Hugging Face에서 모델 다운로드 및 macOS 테스트

### 방법 A: 원클릭 Python 샌드박스 테스트 (`test_hf_macos.py`)

Hugging Face 저장소로부터 가중치 및 설정 파일(`models/`, `resources/`)을 자동으로 다운로드하여 macOS 상에서 Python 파이프라인 및 C++ dylib 추론을 검증합니다:

```bash
# 저장소 클론
git clone https://github.com/Antigravity/snap.git
cd snap

# Hugging Face 모델 자동 다운로드 및 macOS E2E 테스트 실행
python test_hf_macos.py --repo-id <HuggingFace_ID>/snap-models --lang ko
```

### 방법 B: `git lfs`를 통한 모델 다운로드 및 `SNAP_HOME` 지정

1. Hugging Face Model Hub에서 저장소를 수동 클론합니다:
   ```bash
   git lfs install
   git clone https://huggingface.co/<HuggingFace_ID>/snap-models ~/hf_snap_models
   ```
2. 환경변수로 `SNAP_HOME` 앵커 지정:
   ```bash
   export SNAP_HOME=~/hf_snap_models
   ```

---

## 4. macOS C++ Native 라이브러리 (`libsnap_cpp.dylib`) 빌드

`CMake`를 이용하여 macOS 동적 공유 라이브러리 및 C++ 실행 바이너리를 빌드합니다:

```bash
# 자동 빌드 & E2E 테스트 셸 스크립트 실행
chmod +x build_and_test_macos.sh
./build_and_test_macos.sh
```

수동 CMake 명령:
```bash
cmake -B snap_cpp/build_macos -S snap_cpp -DCMAKE_BUILD_TYPE=Release
cmake --build snap_cpp/build_macos --config Release -j$(sysctl -n hw.ncpu)

# C++ Native 배치 추론 실행
./snap_cpp/build_macos/test_e2e $SNAP_HOME ko "커피 3잔을 마셨다."
```

---

## 5. macOS 테스트 성공 검증 결과 예시

```text
==================================================================
  SNAP TTS Engine - Hugging Face macOS Integration Test
==================================================================
[1/3] Downloading models from Hugging Face Hub...
      Downloaded to: /Users/macuser/snap/hf_models

[2/3] Testing Python Pipeline on macOS (Language: ko)...
  - 원문: iPhone 16 Pro 128GB 모델을 15층에서 백달러에 구매했다.
  - 정규화/음운 결과: 아이폰 시뷱 프로 백이십팔지비 모델을 시보층에서 백딸러에 구매핻따.
  - [OK] Python Pipeline verified successfully on macOS!

[3/3] Testing C++ Native Shared Library (libsnap_cpp.dylib)...
  - C++ dylib G2P Result: 아이폰 시뷱 프로 백이십팔지비 모델을 시보층에서 백딸러에 구매핻따.
  - [OK] macOS C++ Shared Library (libsnap_cpp.dylib) verified successfully!
==================================================================
  [SUCCESS] macOS Hugging Face Integration Test PASSED 100%!
==================================================================
```
