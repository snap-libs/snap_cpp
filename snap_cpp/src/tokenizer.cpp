/**
 * SNAP BertTokenizer — C++ WordPiece implementation
 * =================================================
 * Loads vocab from HuggingFace tokenizer.json, implements BERT tokenization
 * with full byte-level offset tracking through the pipeline.
 */

#include "snap/tokenizer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <cassert>

namespace snap {

// ═══════════════════════════════════════════════════════
// UTF-8 Utilities
// ═══════════════════════════════════════════════════════

static uint32_t utf8_decode_at(const char* s, size_t len, size_t pos, size_t& bytes) {
    if (pos >= len) { bytes = 0; return 0; }
    unsigned char c = static_cast<unsigned char>(s[pos]);
    if (c < 0x80) {
        bytes = 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && pos + 1 < len) {
        bytes = 2;
        return ((uint32_t)(c & 0x1F) << 6) |
               (uint32_t)(s[pos + 1] & 0x3F);
    }
    if ((c & 0xF0) == 0xE0 && pos + 2 < len) {
        bytes = 3;
        return ((uint32_t)(c & 0x0F) << 12) |
               ((uint32_t)(s[pos + 1] & 0x3F) << 6) |
               (uint32_t)(s[pos + 2] & 0x3F);
    }
    if ((c & 0xF8) == 0xF0 && pos + 3 < len) {
        bytes = 4;
        return ((uint32_t)(c & 0x07) << 18) |
               ((uint32_t)(s[pos + 1] & 0x3F) << 12) |
               ((uint32_t)(s[pos + 2] & 0x3F) << 6) |
               (uint32_t)(s[pos + 3] & 0x3F);
    }
    bytes = 1;
    return 0xFFFD;
}

std::string utf8_encode(uint32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

std::vector<CodePoint> parse_utf8(const std::string& text) {
    std::vector<CodePoint> result;
    result.reserve(text.size());
    size_t pos = 0;
    while (pos < text.size()) {
        size_t bytes = 0;
        uint32_t cp = utf8_decode_at(text.c_str(), text.size(), pos, bytes);
        if (bytes == 0) break;
        result.push_back({cp, pos, pos + bytes});
        pos += bytes;
    }
    return result;
}

// ═══════════════════════════════════════════════════════
// Unicode Category Helpers
// ═══════════════════════════════════════════════════════

bool BertTokenizer::is_whitespace(uint32_t cp) {
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') return true;
    // Unicode Zs category (common)
    return cp == 0x00A0 || (cp >= 0x2000 && cp <= 0x200A) ||
           cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

bool BertTokenizer::is_control(uint32_t cp) {
    if (cp == '\t' || cp == '\n' || cp == '\r') return false;
    if (cp < 0x20) return true;
    if (cp >= 0x7F && cp <= 0x9F) return true;
    return false;
}

bool BertTokenizer::is_punctuation(uint32_t cp) {
    // ASCII punctuation
    if ((cp >= 33 && cp <= 47) || (cp >= 58 && cp <= 64) ||
        (cp >= 91 && cp <= 96) || (cp >= 123 && cp <= 126))
        return true;

    // Latin-1 punctuation/symbols (such as middle dot U+00B7, section sign, multiplication/division, etc.)
    if (cp == 0x00B7 || cp == 0x00D7 || cp == 0x00F7 || 
        cp == 0x00A1 || cp == 0x00BF || cp == 0x00AB || cp == 0x00BB ||
        cp == 0x00AC || cp == 0x00B1 || cp == 0x00A7 || cp == 0x00B6)
        return true;

    // Unicode general punctuation block (covers U+2000 to U+206F)
    if (cp >= 0x2000 && cp <= 0x206F)
        return true;

    // CJK Symbols and Punctuation (covers U+3000 to U+303F)
    if (cp >= 0x3000 && cp <= 0x303F)
        return true;

    // Fullwidth forms
    if (cp >= 0xFF01 && cp <= 0xFF0F) return true;
    if (cp >= 0xFF1A && cp <= 0xFF20) return true;
    if (cp >= 0xFF3B && cp <= 0xFF40) return true;
    if (cp >= 0xFF5B && cp <= 0xFF65) return true;

    // Mathematical Operators (covers U+2200 to U+22FF)
    if (cp >= 0x2200 && cp <= 0x22FF) return true;

    // Miscellaneous Technical / Geometric Shapes / Miscellaneous Symbols
    if (cp >= 0x25A0 && cp <= 0x25FF) return true; // Geometric Shapes
    if (cp >= 0x2600 && cp <= 0x26FF) return true; // Miscellaneous Symbols

    return false;
}

bool BertTokenizer::is_chinese_char(uint32_t cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x20000 && cp <= 0x2A6DF) ||
           (cp >= 0x2A700 && cp <= 0x2B73F) ||
           (cp >= 0x2B740 && cp <= 0x2B81F) ||
           (cp >= 0x2B820 && cp <= 0x2CEAF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0x2F800 && cp <= 0x2FA1F);
}

uint32_t BertTokenizer::to_lower(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    // Latin-1 Supplement
    if (cp >= 0xC0 && cp <= 0xD6) return cp + 32;
    if (cp >= 0xD8 && cp <= 0xDE) return cp + 32;
    // Latin Extended-A (partial)
    if (cp >= 0x100 && cp <= 0x12E && (cp % 2 == 0)) return cp + 1;
    return cp;
}

// ═══════════════════════════════════════════════════════
// BertTokenizer::load — parse tokenizer.json
// ═══════════════════════════════════════════════════════

static nlohmann::json parse_json_utf8(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return nlohmann::json();
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return nlohmann::json::parse(str, nullptr, false);
}

bool BertTokenizer::load(const std::string& path) {
    auto j = parse_json_utf8(path);
    if (j.is_discarded() || !j.is_object()) return false;

    // Vocab: model.vocab → {"token": id, ...}
    if (!j.contains("model") || !j["model"].contains("vocab")) return false;

    auto& vocab = j["model"]["vocab"];
    vocab_.reserve(vocab.size());
    for (auto it = vocab.begin(); it != vocab.end(); ++it) {
        vocab_[it.key()] = it.value().get<int>();
    }

    // Model config
    auto& model = j["model"];
    if (model.contains("continuing_subword_prefix"))
        continuing_prefix_ = model["continuing_subword_prefix"].get<std::string>();
    if (model.contains("max_input_chars_per_word"))
        max_input_chars_ = model["max_input_chars_per_word"].get<int>();

    // Normalizer config
    if (j.contains("normalizer") && !j["normalizer"].is_null()) {
        auto& norm = j["normalizer"];
        if (norm.contains("lowercase"))
            do_lower_case_ = norm["lowercase"].get<bool>();
        if (norm.contains("handle_chinese_chars"))
            handle_chinese_ = norm["handle_chinese_chars"].get<bool>();
    }

    // Special token IDs
    auto find_id = [&](const std::string& token, int def) {
        auto it = vocab_.find(token);
        return it != vocab_.end() ? it->second : def;
    };
    cls_id_ = find_id("[CLS]", 2);
    sep_id_ = find_id("[SEP]", 3);
    unk_id_ = find_id("[UNK]", 1);
    pad_id_ = find_id("[PAD]", 0);

    // Detect if special tokens should be added based on post_processor in tokenizer.json
    if (j.contains("post_processor") && j["post_processor"].is_null()) {
        add_special_tokens_ = false;
    } else {
        add_special_tokens_ = true;
    }

    return !vocab_.empty();
}

// ═══════════════════════════════════════════════════════
// Normalize
// ═══════════════════════════════════════════════════════

std::vector<CodePoint> BertTokenizer::normalize(const std::vector<CodePoint>& cps) const {
    std::vector<CodePoint> result;
    result.reserve(cps.size());

    for (const auto& cp : cps) {
        uint32_t c = cp.cp;

        // NFKC normalization for compatibility with HF tokenizer
        if (c >= 0xFF10 && c <= 0xFF19) {
            c = c - 0xFF10 + '0';
        }
        else if (c >= 0xFF21 && c <= 0xFF3A) {
            c = c - 0xFF21 + 'A';
        }
        else if (c >= 0xFF41 && c <= 0xFF5A) {
            c = c - 0xFF41 + 'a';
        }
        else if (c == 0x3000) {
            c = 0x0020;
        }
        else if (c == 0xFF05) c = '%';
        else if (c == 0xFF0F) c = '/';
        else if (c == 0xFF5E) c = '~';
        else if (c == 0xFF08) c = '(';
        else if (c == 0xFF09) c = ')';
        else if (c == 0xFF0C) c = ',';
        else if (c == 0xFF0E) c = '.';
        else if (c == 0xFF1A) c = ':';
        else if (c == 0xFF1B) c = ';';
        else if (c == 0xFF1F) c = '?';
        else if (c == 0xFF01) c = '!';
        else if (c == 0xFF0D) c = '-';

        // 1. Clean: skip control chars and NUL
        if (c == 0 || c == 0xFFFD || is_control(c)) continue;

        // 2. Whitespace → space
        if (is_whitespace(c)) {
            result.push_back({' ', cp.byte_start, cp.byte_end});
            continue;
        }

        // 3. CJK chars: surround with spaces
        if (handle_chinese_ && is_chinese_char(c)) {
            result.push_back({' ', cp.byte_start, cp.byte_start});
            uint32_t lc = do_lower_case_ ? to_lower(c) : c;
            result.push_back({lc, cp.byte_start, cp.byte_end});
            result.push_back({' ', cp.byte_end, cp.byte_end});
            continue;
        }

        // 4. Lowercase
        if (do_lower_case_) c = to_lower(c);

        result.push_back({c, cp.byte_start, cp.byte_end});
    }
    return result;
}

// ═══════════════════════════════════════════════════════
// Pre-tokenize: split on whitespace and punctuation
// ═══════════════════════════════════════════════════════

std::vector<BertTokenizer::PreToken>
BertTokenizer::pre_tokenize(const std::vector<CodePoint>& cps) const {
    std::vector<PreToken> result;
    PreToken current;

    for (const auto& cp : cps) {
        if (is_whitespace(cp.cp)) {
            if (!current.codepoints.empty()) {
                result.push_back(std::move(current));
                current = PreToken{};
            }
            continue;
        }

        if (is_punctuation(cp.cp)) {
            // Flush current word
            if (!current.codepoints.empty()) {
                result.push_back(std::move(current));
                current = PreToken{};
            }
            // Punctuation as its own token
            PreToken punct;
            punct.codepoints.push_back(cp.cp);
            punct.orig_offsets.push_back({cp.byte_start, cp.byte_end});
            result.push_back(std::move(punct));
            continue;
        }

        current.codepoints.push_back(cp.cp);
        current.orig_offsets.push_back({cp.byte_start, cp.byte_end});
    }

    if (!current.codepoints.empty()) {
        result.push_back(std::move(current));
    }
    return result;
}

// ═══════════════════════════════════════════════════════
// WordPiece: greedy longest-match subword tokenization
// ═══════════════════════════════════════════════════════

std::vector<BertTokenizer::SubToken>
BertTokenizer::wordpiece(const PreToken& word) const {
    std::vector<SubToken> tokens;
    size_t n = word.codepoints.size();

    // Too long → [UNK]
    if (n > static_cast<size_t>(max_input_chars_)) {
        tokens.push_back({unk_id_,
                          word.orig_offsets.front().first,
                          word.orig_offsets.back().second});
        return tokens;
    }

    size_t start = 0;

    while (start < n) {
        size_t end = n;
        int best_id = -1;
        size_t best_end = start;

        while (end > start) {
            // Build candidate string from codepoints [start, end)
            std::string sub;
            for (size_t i = start; i < end; i++) {
                sub += utf8_encode(word.codepoints[i]);
            }
            if (start > 0) sub = continuing_prefix_ + sub;

            auto it = vocab_.find(sub);
            if (it != vocab_.end()) {
                best_id = it->second;
                best_end = end;
                break;
            }
            end--;
        }

        if (best_id < 0) {
            // No match → entire word is UNK
            tokens.clear();
            tokens.push_back({unk_id_,
                              word.orig_offsets.front().first,
                              word.orig_offsets.back().second});
            return tokens;
        }

        size_t orig_start = word.orig_offsets[start].first;
        size_t orig_end = word.orig_offsets[best_end - 1].second;
        tokens.push_back({best_id, orig_start, orig_end});
        start = best_end;
    }

    return tokens;
}

// ═══════════════════════════════════════════════════════
// BertTokenizer::encode — full pipeline
// ═══════════════════════════════════════════════════════

BertTokenizer::EncodeResult BertTokenizer::encode(const std::string& text) const {
    // 1. UTF-8 → codepoints with byte offsets
    auto cps = parse_utf8(text);

    // 2. Normalize
    auto norm = normalize(cps);

    // 3. Pre-tokenize
    auto words = pre_tokenize(norm);

    // 4. WordPiece each word
    std::vector<SubToken> all_tokens;
    for (const auto& word : words) {
        auto sub = wordpiece(word);
        all_tokens.insert(all_tokens.end(), sub.begin(), sub.end());
    }

    // 5. Truncate to 510 tokens max ([CLS] + tokens + [SEP] = 512)
    if (all_tokens.size() > 510) {
        all_tokens.resize(510);
    }
    EncodeResult result;
    size_t total = all_tokens.size() + (add_special_tokens_ ? 2 : 0);
    result.input_ids.reserve(total);
    result.attention_mask.reserve(total);
    result.token_type_ids.reserve(total);
    result.offsets.reserve(total);

    // [CLS]
    if (add_special_tokens_) {
        result.input_ids.push_back(cls_id_);
        result.attention_mask.push_back(1);
        result.token_type_ids.push_back(0);
        result.offsets.push_back({0, 0});
    }

    for (const auto& tok : all_tokens) {
        result.input_ids.push_back(tok.id);
        result.attention_mask.push_back(1);
        result.token_type_ids.push_back(0);
        result.offsets.push_back({tok.byte_start, tok.byte_end});
    }

    // [SEP]
    if (add_special_tokens_) {
        result.input_ids.push_back(sep_id_);
        result.attention_mask.push_back(1);
        result.token_type_ids.push_back(0);
        result.offsets.push_back({0, 0});
    }

    return result;
}

}  // namespace snap
