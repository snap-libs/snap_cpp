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
/// @param weights_dir  Path to weights root or language directory (UTF-8).
/// @param lang         Language code ("ko", "ja", "en")
/// @return Engine instance handle (void*) on success, nullptr on failure
SNAP_API void* snap_create(const char* weights_dir, const char* lang);

/// Create SNAP engine instance with explicit version & variant specification
/// @param weights_dir    Root models directory
/// @param lang           Language code ("ko", "ja", "en")
/// @param variant        Model variant (e.g. "kcbert-base-int8", or nullptr for default)
/// @param dict_version   Dictionary version (e.g. "v1.0.0", or nullptr for active)
/// @param model_version  Model version (e.g. "v1.0.0", or nullptr for active)
SNAP_API void* snap_create_with_version(
    const char* weights_dir,
    const char* lang,
    const char* variant,
    const char* dict_version,
    const char* model_version
);

/// Run SNAP inference on UTF-8 text (includes text normalization)
/// @param handle     Engine instance handle returned by snap_create
/// @param text_utf8  Input text (UTF-8 encoded)
/// @return JSON string with results (caller must call snap_free)
SNAP_API const char* snap_process(void* handle, const char* text_utf8);

/// Standard SNAP text normalization & G2P processing
/// @param handle     Engine instance handle returned by snap_create
/// @param text_utf8  Input text (UTF-8 encoded)
/// @return Normalized text string (caller must call snap_free)
SNAP_API const char* snap_normalize(void* handle, const char* text_utf8);

/// Free a result string returned by snap_process or snap_normalize
SNAP_API void snap_free(const void* result);

/// Destroy and release SNAP engine instance
/// @param handle  Engine instance handle to destroy
SNAP_API void snap_destroy(void* handle);

#ifdef __cplusplus
}
#endif
