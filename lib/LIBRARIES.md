# SNAP C++ SDK Library Catalog

## Current Stable Version: 1.0.0

### Windows x64 (`snap_cpp.dll`)
- **Location**: `lib/windows/x64/v1.0.0/`
- **Compiler**: MSVC 2019 / 2022 (x64)
- **Runtime Linkage**: Static CRT (`/MT` on MSVC, `-static-libstdc++` on GCC)
- **Dependencies**: `onnxruntime.dll` (v1.18.1)
- **Build Version**: `1.0.0.0`

### Linux AMD x64 (`libsnap_cpp.so`)
- **Location**: `lib/linux/x64/v1.0.0/`
- **Compiler**: GCC 11.2 (Linux AMD64)
- **SONAME**: `libsnap_cpp.so.1`

### macOS Universal (`libsnap_cpp.dylib`)
- **Location**: `lib/macos/v1.0.0/`
- **Architectures**: Universal (ARM64 + Intel x86_64)

---

## Version History

### v1.0.0 (2026-07-28) ✅ Stable Initial Release
- Multilingual support (Korean, Japanese, English)
- Zero-overhead architecture with ONNX Runtime v1.18.1
- Dual-path version auto-detect (`manifest.json`)
