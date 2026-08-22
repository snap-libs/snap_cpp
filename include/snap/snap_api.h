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

/// Create SNAP engine instance with explicit device specification
/// @param weights_dir  Path to weights root or language directory (UTF-8).
/// @param lang         Language code ("ko", "ja", "en")
/// @param device       Device target ("auto", "cuda", "directml", "coreml", "cpu")
/// @return Engine instance handle (void*) on success, nullptr on failure
SNAP_API void* snap_create_device(const char* weights_dir, const char* lang, const char* device);

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

/// Run SNAP inference with per-sentence dynamic options (in-memory JSON string)
/// @param handle        Engine instance handle returned by snap_create
/// @param text_utf8     Input text (UTF-8 encoded)
/// @param options_json  JSON string containing dynamic options to override defaults 
///                      (e.g. "{\"to_ssml\": true}", "{\"to_ssml\": true, \"to_json\": false}")
/// @return Result JSON or plain text string (caller must call snap_free)
SNAP_API const char* snap_process_ext(void* handle, const char* text_utf8, const char* options_json);

/// Run SNAP inference on multiple UTF-8 texts in batch mode (1 BERT forward pass)
/// @param handle     Engine instance handle returned by snap_create
/// @param texts_utf8 Array of UTF-8 strings
/// @param count      Number of strings in the array
/// @return JSON array string with results (caller must call snap_free)
SNAP_API const char* snap_process_batch(void* handle, const char** texts_utf8, int count);

/// Run SNAP inference on multiple UTF-8 texts in batch mode with dynamic options
/// @param handle        Engine instance handle returned by snap_create
/// @param texts_utf8    Array of UTF-8 strings
/// @param count         Number of strings in the array
/// @param options_json  JSON string containing dynamic options to override defaults
/// @return Result JSON array string or newline-delimited string (caller must call snap_free)
SNAP_API const char* snap_process_batch_ext(void* handle, const char** texts_utf8, int count, const char* options_json);

/// Standard SNAP text normalization & G2P processing
/// @param handle     Engine instance handle returned by snap_create
/// @param text_utf8  Input text (UTF-8 encoded)
/// @return Normalized text string (caller must call snap_free)
SNAP_API const char* snap_normalize(void* handle, const char* text_utf8);

/// Get BERT Hidden States tensor [seq_len, hidden_dim] and word2ph mapping array
/// @param handle          Engine instance handle returned by snap_create
/// @param text_utf8       Input text (UTF-8 encoded)
/// @param out_seq_len     [Output] Sequence length (number of tokens)
/// @param out_hidden_dim  [Output] Hidden state dimension (e.g. 768)
/// @param out_word2ph     [Output] Pointer to word2ph array (allocated by SDK, caller calls snap_free_tensor)
/// @param out_word2ph_len [Output] Length of word2ph array
/// @return Pointer to float tensor array [seq_len * hidden_dim] (allocated by SDK, caller calls snap_free_tensor)
SNAP_API float* snap_get_bert_features(
    void* handle,
    const char* text_utf8,
    int* out_seq_len,
    int* out_hidden_dim,
    int** out_word2ph,
    int* out_word2ph_len
);

/// Free a tensor buffer or array allocated by SNAP SDK
SNAP_API void snap_free_tensor(void* ptr);

/// Free a result string returned by snap_process or snap_normalize
SNAP_API void snap_free(const void* result);

/// Destroy and release SNAP engine instance
/// @param handle  Engine instance handle to destroy
SNAP_API void snap_destroy(void* handle);

#ifdef __cplusplus
}
#endif
