---
name: snap-cpp-library-management
description: SNAP C++ SDK 바이너리(DLL/SO/dylib) 버저닝, ABI 호환성 검증, Win32 RC 리소스, Linux SONAME 및 lib/ 레포지토리 카탈로그 관리 지침
---

# SNAP C++ SDK 라이브러리 버전 관리 스킬 가이드

본 스킬 가이드는 SNAP C++ SDK 네이티브 바이너리(`snap_cpp.dll`, `libsnap_cpp.so`, `libsnap_cpp.dylib`)의 ABI 호환성 및 버전 관리를 위한 표준 운영 절차(SOP)입니다.

---

## 1. C API 버전 헤더 규약 (`snap_version.h`)

컴파일 타임 및 런타임 버전 획득을 위해 모든 C/C++ 바인딩은 아래 API를 활용합니다:

```c
#define SNAP_VERSION_MAJOR 1
#define SNAP_VERSION_MINOR 0
#define SNAP_VERSION_PATCH 0
#define SNAP_VERSION_STRING "1.0.0"

SNAP_API const char* snap_version(void);
SNAP_API int snap_version_number(void);
SNAP_API int snap_version_check(int major, int minor);
SNAP_API const char* snap_version_verbose(void);
```

---

## 2. OS별 바이너리 버저닝 관리

### 2.1 Windows Win32 Resource (`snap_version.rc`)
* 탐색기 속성(자세히 탭)에 **제품 버전: 1.0.0**, **파일 버전: 1.0.0.0** 표시.

### 2.2 Linux SONAME & Symlink
* `libsnap_cpp.so.1.0.0` (실제 바이너리)
* `libsnap_cpp.so.1` (SONAME 심볼릭 링크)
* `libsnap_cpp.so` (최신 링크)

### 2.3 macOS Dylib Version
* `libsnap_cpp.1.dylib` (compatibility version 1.0.0)

---

## 3. SDK 저장소 `lib/` 디렉터리 구조

```
snap_cpp/
├── include/snap/
│   ├── snap_api.h
│   └── snap_version.h
├── lib/
│   ├── LIBRARIES.md
│   ├── windows/x64/v1.0.0/
│   │   ├── snap_cpp.dll
│   │   ├── snap_cpp.lib
│   │   └── VERSION_INFO.json
│   ├── linux/x64/v1.0.0/
│   │   ├── libsnap_cpp.so.1.0.0
│   │   └── VERSION_INFO.json
│   └── macos/v1.0.0/
│       ├── libsnap_cpp.1.dylib
│       └── VERSION_INFO.json
└── examples/
    └── version_check.cpp
```

---

## 4. 버전 및 ABI 호환성 검증 스크립트 실행

```bash
# Windows MSVC 빌드 및 버전 검증
cl /std:c++17 /Iinclude examples/version_check.cpp /link /LIBPATH:lib/windows/x64/v1.0.0 snap_cpp.lib
./version_check.exe

# Linux g++ 빌드 및 버전 검증
g++ -std=c++17 -Iinclude examples/version_check.cpp -Llib/linux/x64/v1.0.0 -lsnap_cpp -o version_check
./version_check
```
