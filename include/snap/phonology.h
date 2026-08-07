#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "classifier.h"

namespace snap {

struct CharMeta {
    bool tens = false;
    std::string pos = "";
    bool morph_boundary = false;
    bool is_long = false;
};

struct HangulTuple {
    char type = 'O';      // 'H' for Hangul, 'O' for Other
    std::string cho = "";
    std::string jung = "";
    std::string jong = "";
    CharMeta meta;
    uint32_t original_cp = 0;
    size_t byte_start = 0;
    size_t byte_end = 0;
};

class PhonologyKr {
public:
    PhonologyKr();
    ~PhonologyKr();

    /// Load pronunciation exceptions dictionary from weights_dir/ko/idiom_exceptions.json
    bool init(const std::string& weights_dir);

    /// Apply all Korean phonology rules sequentially and return the phonetic transcription or IPA string
    std::string apply_rules(const std::string& text, const SnapResult& result, const std::string& vowel_length_style = "none", bool to_ipa = false);

    /// Convert Hangul tuples or text to IPA representation
    std::string hangul_to_ipa(const std::vector<HangulTuple>& tuples, bool include_vowel_length = false);


private:
    std::unordered_map<std::string, std::string> idiom_exceptions_;
    std::vector<std::pair<std::string, std::string>> sorted_idioms_;
    std::unordered_set<std::string> loanwords_set_;

    bool is_loanword_span(const std::vector<HangulTuple>& tuples, int start_idx, int nxt_idx);

    std::string apply_jamo_names(const std::string& text);
    std::string apply_idiom_exceptions(const std::string& text);
    
    std::vector<CharMeta> build_char_meta(const std::string& text, 
                                          const std::vector<uint32_t>& codepoints,
                                          const std::vector<std::pair<size_t, size_t>>& cp_byte_offsets,
                                          const SnapResult& result);

    std::vector<HangulTuple> text_to_tuples(const std::string& text, 
                                             const std::vector<uint32_t>& codepoints,
                                             const std::vector<std::pair<size_t, size_t>>& cp_byte_offsets,
                                             const std::vector<CharMeta>& char_meta);

    void apply_josa_ui(std::vector<HangulTuple>& tuples);
    void apply_vowel_simplification(std::vector<HangulTuple>& tuples);
    void apply_n_addition(std::vector<HangulTuple>& tuples);
    void apply_aspiration_and_h_drop(std::vector<HangulTuple>& tuples);
    void apply_palatalization(std::vector<HangulTuple>& tuples);
    void apply_tensification(std::vector<HangulTuple>& tuples);
    void apply_liaison(std::vector<HangulTuple>& tuples);
    void apply_neutralization(std::vector<HangulTuple>& tuples);
    void apply_assimilation(std::vector<HangulTuple>& tuples);

    std::string tuples_to_text(const std::vector<HangulTuple>& tuples, const std::string& vowel_length_style = "none");

    // Helpers
    int get_next_hangul_idx(const std::vector<HangulTuple>& tuples, int start_idx);
};

} // namespace snap
