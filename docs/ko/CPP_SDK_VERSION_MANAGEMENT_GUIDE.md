# C++ SDK 라이브러리 버전 관리 구현 가이드

## 📌 개요
Win32 DLL, Linux SO, macOS dylib의 ABI 호환성 및 네이티브 버전 추적을 위한 실행 가이드입니다.

---

## 🎯 3단계 버전 관리 체계

### Phase 1: C API 버전 인터페이스
- `snap_version()`: 런타임 버전 문자열 (예: `"1.0.0"`)
- `snap_version_number()`: 수치 버전 (예: `10000`)
- `snap_version_check(major, minor)`: Major/Minor ABI 호환성 검증

### Phase 2: OS별 바이너리 버전 리소스
- Windows: `snap_version.rc` Resource 연동 (DLL 우클릭 ➔ 속성 ➔ 1.0.0.0)
- Linux: `SONAME` 링킹 (`libsnap_cpp.so.1`)

### Phase 3: SDK 레포 구조 정규화
- `lib/windows/x64/v1.0.0/`
- `lib/linux/x64/v1.0.0/`
- `lib/macos/v1.0.0/`
