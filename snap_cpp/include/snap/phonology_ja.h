#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <map>
#include <set>

namespace snap {

struct MorphItem;

class PhonologyJa {
public:
    PhonologyJa();
    ~PhonologyJa();

    /// Load Japanese dictionaries (ja_kanji_dict.json, ja_accent_dict.json, targets.json)
    /// @param weights_dir Path to snap/weights directory
    bool init(const std::string& weights_dir);

    /// Apply Japanese phonology rules (Katakana conversion + accent extraction)
    /// @param text The normalized input text
    /// @param annotations SNAP annotations [(start, end, label), ...]
    /// @param morphemes Japanese morphological analysis output
    /// @param out_accent_overrides Output map to store extracted accent types
    std::string apply_rules(const std::string& text, 
                            const std::vector<std::tuple<int, int, std::string>>& annotations,
                            const std::vector<MorphItem>& morphemes,
                            std::unordered_map<std::string, int>& out_accent_overrides);

private:
    // ── Counter Liaison Rules (ja_counter_liaison.json) ─────────────────────
    struct SokuonRule {
        std::string ends;        // num_kata 말미 패턴
        int         strip;       // 제거할 가타카나 글자 수 (각 3 bytes)
        std::string out;         // 대체 문자열
        std::vector<std::string> groups;  // 적용 자음군
        bool        H_handaku;   // H군 단위에 반탁음화 적용
    };
    struct SpecialSanRule {
        std::string group;
        std::vector<std::string> units;
        std::string unit_change;
    };
    struct LiaisonRules {
        std::unordered_map<std::string, std::string> unit_groups;
        std::unordered_map<std::string, std::string> handaku;
        std::unordered_map<std::string, std::string> daku;
        std::vector<std::string>                     aspirated_units;
        std::vector<SokuonRule>                      sokuon;
        std::vector<std::string>                     rendaku_H_suffixes;
        std::vector<SpecialSanRule>                  special_san;
        bool loaded = false;
    };
    LiaisonRules liaison_rules_;

    std::unordered_map<std::string, std::vector<std::string>> kanji_dict_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> targets_accent_;
    std::unordered_map<std::string, int> accent_dict_;
    std::vector<std::string> sorted_patches_;

    // Helpers
    std::string hira2kata(const std::string& text);
    std::string preprocess_symbols(const std::string& text);
    std::string normalize_punct(const std::string& text);
    std::string replace_datetime_and_numbers(const std::string& text);
    std::string convert_numbers(const std::string& text);
    std::string convert_alpha(const std::string& text);
    std::string lookup(const std::string& text, 
                       const std::unordered_map<std::string, std::string>& overrides, 
                       std::unordered_map<std::string, int>& out_accent_overrides);

    std::string int_to_kata_ja(int64_t n);
    std::string chunk4_to_kata_ja(int64_t n);
    std::string convert_minutes(int64_t n);
    std::string int_to_ja(int64_t n);
    std::string chunk4_to_ja(int64_t n);
    std::pair<std::string, std::string> apply_liaison(
        const std::string& num_kata, const std::string& unit_kata,
        const std::string& unit_surf) const;
};

} // namespace snap
