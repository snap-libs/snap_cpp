#pragma once

#ifdef _WIN32
  #ifdef SNAP_BUILD_DLL
    #define SNAP_API __declspec(dllexport)
  #else
    #define SNAP_API __declspec(dllimport)
  #endif
#else
  #define SNAP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Create SNAP engine instance: load BERT + all head ONNX models
/// @param weights_dir  가중치 루트 디렉터리 또는 언어 폴더 직접 경로 (UTF-8).
///                     - 루트: snap_config.json 이 weights_dir/<lang>/ 에 있는 경우
///                     - 직접: snap_config.json 이 weights_dir/ 에 있는 경우 (lang 폴더 자체)
///                     어느 경우든 자동으로 탐색합니다.
/// @param lang         Language code ("ko", "ja", "en")
/// @return Engine instance handle (void*) on success, nullptr on failure
SNAP_API void* snap_create(const char* weights_dir, const char* lang);

/// Run SNAP inference on UTF-8 text (includes text normalization)
/// @param handle     Engine instance handle returned by snap_create
/// @param text_utf8  Input text (UTF-8 encoded)
/// @return JSON string with results (caller must call snap_free)
SNAP_API const char* snap_process(void* handle, const char* text_utf8);

/// Normalize text only (no BERT inference)
/// @param handle     Engine instance handle returned by snap_create
/// @param text_utf8  Input text (UTF-8 encoded)
/// @return Normalized text string (caller must call snap_free)
SNAP_API const char* snap_normalize(void* handle, const char* text_utf8);

/// Free a result string returned by snap_process or snap_normalize
/// @note Use c_void_p (not c_char_p) as restype in ctypes to preserve the
///       raw pointer for this call — c_char_p would auto-convert to bytes.
SNAP_API void snap_free(const void* result);

/// Destroy and release SNAP engine instance
/// @param handle  Engine instance handle to destroy
SNAP_API void snap_destroy(void* handle);

#ifdef __cplusplus
}
#endif
