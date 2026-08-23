// snap_cpp/include/snap/config.h — Global configuration manager for SNAP C++ core
#ifndef SNAP_CONFIG_H
#define SNAP_CONFIG_H

#include <string>
#include <unordered_map>

namespace snap {

struct KoreanOptions {
    bool vowel_length = false;
    bool to_ipa = false;
    bool to_ssml = false;
    bool tn_only = false;
};

struct JapaneseOptions {
    std::string script = "katakana"; // "katakana", "hiragana", "romaji"
    bool pitch_accent = false;
    bool to_ipa = false;
    bool to_ssml = false;
    bool tn_only = false;
};

struct EnglishOptions {
    bool to_ipa = false;
    bool to_ssml = false;
    bool tn_only = false;
};

class SnapGlobalConfig {
public:
    std::string version = "1.0.0";
    std::string device = "auto"; // "auto", "cuda", "directml", "coreml", "cpu"
    int num_threads = 0;         // 0: auto-detect
    KoreanOptions ko;
    JapaneseOptions ja;
    EnglishOptions en;

    // Graceful Fallback: Load from file or return default in-memory config if missing
    static SnapGlobalConfig LoadOrDefault(const std::string& config_path = "");
};

} // namespace snap

#endif // SNAP_CONFIG_H
