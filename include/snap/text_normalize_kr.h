#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace snap {

struct NumberItem; // forward declaration

struct NormalizedSpan {
    size_t start;
    size_t end;
    std::string replacement;
};

/// Korean text normalizer — converts numbers, English, units to Korean
class TextNormalizeKr {
public:
    TextNormalizeKr();
    ~TextNormalizeKr();

    /// Load dictionaries from weights_dir/ko/
    bool init(const std::string& weights_dir);

    /// Full normalization pipeline:
    /// preprocess_symbols → normalize_units_and_numbers →
    /// transliterate_english_fallback → adversarial_spelling_ko →
    /// adversarial_jongseong_ko → normalize
    std::string normalize(const std::string& text) const;

    /// Scan normalized spans from original text (equivalent to python scan)
    std::vector<NormalizedSpan> scan(const std::string& text,
        const std::vector<NumberItem>& numbers = {}) const;

    /// Apply scanned spans to text (equivalent to python apply_spans)
    std::string apply_spans(const std::string& text,
        const std::vector<NormalizedSpan>& spans) const;

private:
    // Dictionaries
    std::unordered_map<std::string, std::string> english_dictionary_exact_; // KEY=original case
    std::unordered_map<std::string, std::string> english_dictionary_;       // KEY=UPPERCASE
    std::unordered_map<std::string, std::string> etc_dictionary_;
    std::vector<std::pair<std::string, std::string>> known_words_;     // ordered pairs

    // Pipeline stages
    std::string preprocess_symbols(const std::string& text) const;
    std::string normalize_units_and_numbers(const std::string& text) const;
    std::string transliterate_english_fallback(const std::string& text) const;
    std::string adversarial_spelling_ko(const std::string& text) const;
    std::string adversarial_jongseong_ko(const std::string& text) const;
    std::string normalize_final(const std::string& text) const;

    // English word transliteration helpers (EngWordReader & G2P-lite)
    std::string english_word_to_korean(const std::string& word) const;
    std::string eng_word_post_process(const std::string& reading) const;
    std::string eng_word_probe_b(const std::string& w) const;
    void eng_word_best_blocks(const std::string& w, size_t& pre, size_t& suf) const;
    struct EngChunk {
        std::string text;
        std::string reading;
        std::string kind;
    };
    std::vector<EngChunk> eng_word_segment(const std::string& rest, bool has_prev = false) const;
    std::string eng_word_read_camel_part(const std::string& part) const;
    std::string eng_spell_word(const std::string& word) const;
    static bool is_readable_chunk(const std::string& chunk);
    static std::string phonetic_read_chunk(const std::string& chunk);

    // Dictionary matching helper
    std::string normalize_with_dictionary(const std::string& text,
        const std::unordered_map<std::string, std::string>& dict) const;

    // Utility
    static std::string to_upper(const std::string& s);
    static std::string to_lower(const std::string& s);
};

}  // namespace snap
