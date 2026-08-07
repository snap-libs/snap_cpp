#pragma once
#include "snap/snap_api.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace snap {

/// UTF-8 codepoint with byte offset in original text
struct CodePoint {
    uint32_t cp;
    size_t byte_start;
    size_t byte_end;
};

/// Parse UTF-8 string to codepoints with byte offsets
std::vector<CodePoint> parse_utf8(const std::string& text);

/// Encode a single codepoint to UTF-8
std::string utf8_encode(uint32_t cp);

/// BERT WordPiece tokenizer (pure C++, no external dependency)
class SNAP_API BertTokenizer {
public:
    struct EncodeResult {
        std::vector<int64_t> input_ids;
        std::vector<int64_t> attention_mask;
        std::vector<int64_t> token_type_ids;
        std::vector<std::pair<size_t, size_t>> offsets;  // byte offsets per token
    };

    bool load(const std::string& tokenizer_json_path);
    EncodeResult encode(const std::string& text) const;

    int cls_id() const { return cls_id_; }
    int sep_id() const { return sep_id_; }
    int unk_id() const { return unk_id_; }

private:
    std::unordered_map<std::string, int> vocab_;
    int cls_id_ = 2, sep_id_ = 3, unk_id_ = 1, pad_id_ = 0;
    bool do_lower_case_ = true;
    bool handle_chinese_ = false;
    int max_input_chars_ = 100;
    std::string continuing_prefix_ = "##";
    bool add_special_tokens_ = true;

    // ─── Internal pipeline ───

    /// Normalize: lowercase + clean control chars + CJK spacing
    std::vector<CodePoint> normalize(const std::vector<CodePoint>& cps) const;

    /// Pre-tokenized word: codepoints + their original byte offsets
    struct PreToken {
        std::vector<uint32_t> codepoints;
        std::vector<std::pair<size_t, size_t>> orig_offsets;
    };

    /// Split on whitespace and punctuation, tracking offsets
    std::vector<PreToken> pre_tokenize(const std::vector<CodePoint>& cps) const;

    /// Sub-token produced by WordPiece
    struct SubToken {
        int id;
        size_t byte_start;
        size_t byte_end;
    };

    /// WordPiece on a single pre-token
    std::vector<SubToken> wordpiece(const PreToken& word) const;

    // ─── Unicode helpers ───
    static bool is_whitespace(uint32_t cp);
    static bool is_punctuation(uint32_t cp);
    static bool is_chinese_char(uint32_t cp);
    static bool is_control(uint32_t cp);
    static uint32_t to_lower(uint32_t cp);
};

}  // namespace snap
