/**
 * SNAP ContextClassifier — all classification heads
 * ===================================================
 * Ports Python classifier.py to C++.
 * Includes: semiotic, number, vowel_length, korean_context heads.
 */

#include "snap/classifier.h"
#include "snap/tokenizer.h"
#include "snap/phonology.h"
#include "snap/phonology_ja.h"
#include "snap/phonology_en.h"
#include "snap/text_normalize_kr.h"
#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <fstream>
#include <regex>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <cassert>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
static std::wstring to_wstr(const std::string& u) {
    if (u.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, u.c_str(), (int)u.size(), 0, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, u.c_str(), (int)u.size(), w.data(), n);
    return w;
}
#endif

namespace snap {

// ═══════════════════════════════════════════════════════
// PIMPL
// ═══════════════════════════════════════════════════════

struct ContextClassifier::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SNAP_Heads"};

    std::unique_ptr<Ort::Session> semiotic_session;
    std::unique_ptr<Ort::Session> korean_context_session;
    std::unique_ptr<Ort::Session> g2p_session;
    std::unique_ptr<Ort::Session> heteronym_session;
    std::unique_ptr<Ort::Session> beon_session;
    std::unique_ptr<Ort::Session> morph_session;
    std::unordered_set<std::string> vowel_length_targets;
    std::unordered_set<std::string> vowel_length_long_set;

    std::map<int, std::string> semiotic_labels;
    std::map<int, std::string> kc_labels;
    std::map<int, std::string> g2p_labels;
    std::map<std::string, int> g2p_label_map;
    std::map<int, std::string> heteronym_labels;
    std::map<int, std::string> beon_labels;
    std::map<int, std::string> morph_labels;  // id → BIO-POS label

    // G2P targets
    std::vector<std::string> g2p_target_words;
    std::map<std::string, std::vector<int>> g2p_word_valid_ids;
    float g2p_threshold = 0.0f;

    // Heteronym
    std::vector<HeteronymTarget> heteronym_targets;
    std::map<std::string, std::string> heteronym_defaults;
    std::unordered_set<std::string> heteronym_always_tens;
    float heteronym_conf_threshold = 0.6f;

    // Morph: word_utf8 → pair<POS bitmask (45 bits), wcost>
    std::unordered_map<std::string, std::pair<uint64_t, int32_t>> morph_word_map;

    // TODO Phase 2: remove
    std::vector<std::string> tensification_targets;

    std::string language;
    PhonologyKr phonology_kr;
    PhonologyJa phonology_ja;
    PhonologyEn phonology_en;
    TextNormalizeKr text_normalize_kr;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> targets_ipa_map;
};

ContextClassifier::ContextClassifier() : impl_(std::make_unique<Impl>()) {}
ContextClassifier::~ContextClassifier() = default;
ContextClassifier::ContextClassifier(ContextClassifier&&) noexcept = default;
ContextClassifier& ContextClassifier::operator=(ContextClassifier&&) noexcept = default;

// ═══════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════

/// Load label map from JSON (handles both {"label": id} and {"id": "label"})
static nlohmann::json parse_json_utf8(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return nlohmann::json();
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return nlohmann::json::parse(str, nullptr, false);
}

/// Load label map from JSON (handles both {"label": id} and {"id": "label"})
static std::map<int, std::string> load_label_map(const std::string& path) {
    std::map<int, std::string> result;
    auto j = parse_json_utf8(path);
    if (j.is_discarded() || !j.is_object()) return result;
    for (auto& [key, val] : j.items()) {
        if (val.is_number_integer()) {
            result[val.get<int>()] = key;           // {"label": id}
        } else if (val.is_string()) {
            result[std::stoi(key)] = val.get<std::string>();  // {"id": "label"}
        }
    }
    return result;
}

/// Load an ONNX head session
static std::unique_ptr<Ort::Session> load_head(
        Ort::Env& env, const std::string& path) {
    Ort::SessionOptions opts;
    opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef _WIN32
    return std::make_unique<Ort::Session>(env, to_wstr(path).c_str(), opts);
#else
    return std::make_unique<Ort::Session>(env, path.c_str(), opts);
#endif
}

/// Run ONNX session, return output data + shape
struct OnnxOut {
    std::vector<float> data;
    std::vector<int64_t> shape;
};

static OnnxOut run_session(Ort::Session& sess,
                           const float* input, const std::vector<int64_t>& in_shape) {
    auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    size_t in_size = 1;
    for (auto d : in_shape) in_size *= static_cast<size_t>(d);

    auto tensor = Ort::Value::CreateTensor<float>(
        mem, const_cast<float*>(input), in_size,
        in_shape.data(), in_shape.size());

    Ort::AllocatorWithDefaultOptions alloc;
    auto in_name  = sess.GetInputNameAllocated(0, alloc);
    auto out_name = sess.GetOutputNameAllocated(0, alloc);
    const char* ins[]  = {in_name.get()};
    const char* outs[] = {out_name.get()};

    auto outputs = sess.Run(Ort::RunOptions{nullptr},
                            ins, &tensor, 1, outs, 1);

    auto& ot = outputs[0];
    auto sh = ot.GetTensorTypeAndShapeInfo().GetShape();
    size_t total = 1;
    for (auto d : sh) total *= static_cast<size_t>(d);
    const float* od = ot.GetTensorData<float>();

    return {std::vector<float>(od, od + total), sh};
}

/// argmax over a float array of length n
static int argmax(const float* arr, int n) {
    return static_cast<int>(
        std::max_element(arr, arr + n) - arr);
}

/// stable softmax: returns probabilities (length n)
static std::vector<float> softmax(const float* arr, int n) {
    float mx = *std::max_element(arr, arr + n);
    std::vector<float> out(n);
    float sum = 0.f;
    for (int i = 0; i < n; i++) { out[i] = std::exp(arr[i] - mx); sum += out[i]; }
    for (auto& v : out) v /= sum;
    return out;
}

// ═══════════════════════════════════════════════════════
// run_heteronym — Korean homograph tensification
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_heteronym(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    if (impl_->heteronym_targets.empty()) return;

    for (auto& tgt : impl_->heteronym_targets) {
        size_t pos = 0;
        while ((pos = text.find(tgt.word, pos)) != std::string::npos) {
            size_t sc = pos;
            size_t ec = pos + tgt.word.size();
            pos = ec;

            // Rule 1: always_tens → skip neural
            if (tgt.always_tens) {
                if (ec > sc + 1) {  // at least 2 chars (start+1 < end)
                    HeteronymItem hi;
                    hi.span  = tgt.word;
                    hi.start = static_cast<int>(sc);
                    hi.end   = static_cast<int>(ec);
                    hi.label = "TENS";
                    hi.conf  = 1.0f;
                    out.heteronym.push_back(hi);
                }
                continue;
            }

            // Rule 2: "잠자리" context patterns
            if (tgt.word == "\xEC\x9E\xA0\xEC\x9E\x90\xEB\xA6\xAC") {  // 잠자리
                std::string after = text.substr(ec);
                static const std::vector<std::string> tens_after = {
                    "\xEC\x97\x90",               // 에
                    "\xEB\xA5\xBC \xEC\xA0\x95\xEB\xA6\xAC",  // 를 정리
                    "\xEB\xA5\xBC \xEB\xA7\x88\xEB\xA0\xA8",  // 를 마련
                    "\xEB\xA5\xBC \xED\x8E\xB4",               // 를 펴
                    "\xEB\xA5\xBC \xEB\xB3\xB4",               // 를 보
                    "\xEA\xB0\x80 \xEC\xB6\xA5\xEC\x9B\x8C",  // 가 추워
                    "\xEA\xB0\x80 \xEC\x8B\x9C\xEB\x81\x84",  // 가 시끄러 (prefix)
                    "\xEA\xB0\x80 \xEB\x82\xA1\xEC\x95\x84",  // 가 낡아
                };
                bool is_tens = false;
                for (auto& p : tens_after) {
                    if (after.compare(0, p.size(), p) == 0) { is_tens = true; break; }
                }
                if (is_tens) {
                    HeteronymItem hi{tgt.word, (int)sc, (int)ec, "TENS", 1.0f};
                    out.heteronym.push_back(hi);
                    continue;
                }
                // default for 잠자리 = NONE → no annotation
                HeteronymItem hi{tgt.word, (int)sc, (int)ec, "NONE", 1.0f};
                out.heteronym.push_back(hi);
                continue;
            }

            // Rule 3: "감기" disease context → NONE
            if (tgt.word == "\xEA\xB0\x90\xEA\xB8\xB0") {  // 감기
                std::string after  = text.substr(ec);
                std::string before = text.substr(0, sc);
                static const std::vector<std::string> none_after = {
                    " \xEC\xA6\x9D\xEC\x83\x81",  //  증상
                    " \xEC\xA0\x84\xED\x98\x95",  //  전형
                    " \xEA\xB1\xB8",              //  걸
                    " \xEB\xAA\xB8\xEC\x82\xB4",  //  몸살
                    "\xEC\x95\xbd",               // 약
                    " \xEA\xB8\xB0\xEC\x9A\xB4",  //  기운
                };
                bool is_none = false;
                for (auto& p : none_after) {
                    if (after.compare(0, p.size(), p) == 0) { is_none = true; break; }
                }
                if (is_none) {
                    out.heteronym.push_back({tgt.word, (int)sc, (int)ec, "NONE", 1.0f});
                    continue;
                }
            }

            if (!impl_->heteronym_session) {
                // No model: use statistical default
                std::string def = tgt.default_label;
                if (def == "TENS" && ec > sc + 3) {
                    out.annotations.emplace_back((int)(sc+3), (int)ec, "TENS");
                }
                out.heteronym.push_back({tgt.word, (int)sc, (int)ec, def, 0.0f});
                continue;
            }

            // ONNX inference: find first overlapping token
            int ti = -1;
            for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
                auto [s, e] = offsets[i];
                if (e == 0) continue;
                if (s <= sc && sc < e) { ti = i; break; }
            }
            if (ti < 0) continue;

            // Run head: input [1, hdim] from token ti
            const float* feat = hidden + ti * hdim;
            auto res = run_session(*impl_->heteronym_session, feat, {1, (int64_t)hdim});
            int num_cls = static_cast<int>(res.data.size());
            auto probs = softmax(res.data.data(), num_cls);
            int pred = argmax(probs.data(), num_cls);
            float conf = probs[pred];

            // Confidence threshold → fallback to default
            std::string label;
            if (conf < impl_->heteronym_conf_threshold) {
                label = tgt.default_label;
            } else {
                auto it = impl_->heteronym_labels.find(pred);
                label = (it != impl_->heteronym_labels.end()) ? it->second : tgt.default_label;
            }

            // Annotations are collected inside process() later to prevent duplicates and filter based on confidence threshold
            //out.annotations.emplace_back((int)(sc+3), (int)ec, "TENS");
            out.heteronym.push_back({tgt.word, (int)sc, (int)ec, label, conf});
        }
    }
}

static uint32_t utf8_decode_adv(const std::string& s, size_t& pos);

// ═══════════════════════════════════════════════════════
// run_beon — 번(番) reading: native vs sino
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_beon(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    // (\d+)(번|대|장|동)
    static const std::regex pat(R"((\d+)(\xEB\xB2\x88|\xEB\x8C\x80|\xEC\x9E\xA5|\xEB\x8F\x99))");
    static const std::string jjae = "\xEC\xA7\xB8";        // 째

    auto it = std::sregex_iterator(text.begin(), text.end(), pat);
    auto end_it = std::sregex_iterator();
    for (; it != end_it; ++it) {
        auto& m = *it;
        std::string num_span = m[1].str();
        std::string unit     = m[2].str();
        int num_start = 0;
        size_t byte_pos = 0;
        size_t target_pos = static_cast<size_t>(m.position(1));
        while (byte_pos < target_pos && byte_pos < text.size()) {
            utf8_decode_adv(text, byte_pos);
            num_start++;
        }
        int num_end = num_start; // we don't strictly use num_end char offset matching in python, but let's approximate it.
        int match_end = static_cast<int>(m.position() + m.length());

        std::string after = (match_end < (int)text.size())
            ? text.substr(static_cast<size_t>(match_end)) : "";

        // Rule: 번째 → always native
        if (unit == "\xEB\xB2\x88" && after.compare(0, jjae.size(), jjae) == 0) {
            out.beon.push_back({num_span, num_start, num_end, "native"});
            continue;
        }

        // Rule: 번가, 번지 → always sino
        if (unit == "\xEB\xB2\x88" && after.size() >= 3) {
            std::string next_char = after.substr(0, 3);
            if (next_char == "\xEA\xB0\x80" || next_char == "\xEC\xA7\x80") {  // 가, 지
                out.beon.push_back({num_span, num_start, num_end, "sino"});
                continue;
            }
        }

        if (!impl_->beon_session) {
            out.beon.push_back({num_span, num_start, num_end, "sino"});
            continue;
        }

        // Find token at num_start
        int ti = -1;
        size_t sc = static_cast<size_t>(m.position());
        for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
            auto [s, e] = offsets[i];
            if (e == 0) continue;
            if (s <= sc && sc < e) { ti = i; break; }
        }
        if (ti < 0) { out.beon.push_back({num_span, num_start, num_end, "sino"}); continue; }

        const float* feat = hidden + ti * hdim;
        auto res = run_session(*impl_->beon_session, feat, {1, (int64_t)hdim});
        int num_cls = static_cast<int>(res.data.size());
        auto probs = softmax(res.data.data(), num_cls);
        int pred = argmax(probs.data(), num_cls);
        auto lit = impl_->beon_labels.find(pred);
        std::string label = (lit != impl_->beon_labels.end()) ? lit->second : "sino";
        out.beon.push_back({num_span, num_start, num_end, label});
    }
}

// ═══════════════════════════════════════════════════════
// UTF-8 helpers (local to classifier.cpp)
// ═══════════════════════════════════════════════════════

static uint32_t utf8_decode_adv(const std::string& s, size_t& pos) {
    if (pos >= s.size()) return 0;
    uint8_t b0 = static_cast<uint8_t>(s[pos]);
    if (b0 < 0x80) { pos += 1; return b0; }
    if (b0 < 0xC0) { pos += 1; return 0xFFFD; }
    if (b0 < 0xE0) {
        uint32_t cp = (b0 & 0x1F);
        if (pos+1 < s.size()) cp = (cp<<6) | (static_cast<uint8_t>(s[pos+1]) & 0x3F);
        pos += 2; return cp;
    }
    if (b0 < 0xF0) {
        uint32_t cp = (b0 & 0x0F);
        if (pos+1 < s.size()) cp = (cp<<6) | (static_cast<uint8_t>(s[pos+1]) & 0x3F);
        if (pos+2 < s.size()) cp = (cp<<6) | (static_cast<uint8_t>(s[pos+2]) & 0x3F);
        pos += 3; return cp;
    }
    uint32_t cp = (b0 & 0x07);
    for (int i = 1; i <= 3; i++) {
        if (pos+i < s.size()) cp = (cp<<6) | (static_cast<uint8_t>(s[pos+i]) & 0x3F);
    }
    pos += 4; return cp;
}

static std::string utf8_encode_cp(uint32_t cp) {
    std::string s;
    if (cp < 0x80) { s += static_cast<char>(cp); }
    else if (cp < 0x800) {
        s += static_cast<char>(0xC0|(cp>>6));
        s += static_cast<char>(0x80|(cp&0x3F));
    } else if (cp < 0x10000) {
        s += static_cast<char>(0xE0|(cp>>12));
        s += static_cast<char>(0x80|((cp>>6)&0x3F));
        s += static_cast<char>(0x80|(cp&0x3F));
    } else {
        s += static_cast<char>(0xF0|(cp>>18));
        s += static_cast<char>(0x80|((cp>>12)&0x3F));
        s += static_cast<char>(0x80|((cp>>6)&0x3F));
        s += static_cast<char>(0x80|(cp&0x3F));
    }
    return s;
}

// ═══════════════════════════════════════════════════════
// run_morph — morpheme boundary + POS tagging
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_morph(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    if (!impl_->morph_session) return;

    static constexpr int MC = 200;   // max chars
    static constexpr int NP = 45;    // num POS tags
    static constexpr int MAX_WL = 20; // max word char length

    // ─── Step 1: Build ns (space-stripped chars) and ns_text ───────────────
    struct NsItem { size_t orig_char_idx; uint32_t cp; };
    std::vector<NsItem> ns;
    std::string ns_text;
    std::vector<size_t> ns_byte_starts;  // byte start of char ci in ns_text

    size_t bi = 0;
    size_t char_idx = 0;
    while (bi < text.size()) {
        size_t bi_before = bi;
        uint32_t cp = utf8_decode_adv(text, bi);
        if (cp == ' ') { char_idx++; continue; }
        ns_byte_starts.push_back(ns_text.size());
        ns_text += utf8_encode_cp(cp);
        ns.push_back({char_idx, cp});
        char_idx++;
    }
    ns_byte_starts.push_back(ns_text.size());  // sentinel

    int n = std::min((int)ns.size(), MC);
    if (n == 0) return;

    // ─── Step 2: Build orig char index → byte start mapping ────────────────
    // Used to map offsets (byte ranges) → char indices
    std::vector<size_t> orig_char_byte_starts;
    {
        size_t bp = 0;
        while (bp < text.size()) {
            orig_char_byte_starts.push_back(bp);
            utf8_decode_adv(text, bp);
        }
        orig_char_byte_starts.push_back(text.size());
    }
    int n_orig_chars = static_cast<int>(orig_char_byte_starts.size()) - 1;

    // ─── Step 3: char_idx → token_index (c2t) ──────────────────────────────
    std::vector<int> c2t(n_orig_chars, -1);
    for (int ti = 0; ti < seq_len && ti < (int)offsets.size(); ti++) {
        auto [s_byte, e_byte] = offsets[ti];
        if (e_byte == 0) continue;
        for (int ci = 0; ci < n_orig_chars; ci++) {
            size_t cs = orig_char_byte_starts[ci];
            if (cs >= s_byte && cs < e_byte)
                c2t[ci] = ti;
        }
    }

    // ─── Step 4: Build 6 feature tensors ───────────────────────────────────
    std::vector<int64_t> tok_indices_data(MC, 0);
    std::vector<int64_t> pos_in_tok_data(MC, 0);
    std::vector<int64_t> char_ids_data(MC, 0);
    std::vector<float>   dict_starts_data(MC * NP, 0.0f);
    std::vector<float>   dict_covers_data(MC * NP, 0.0f);

    for (int ci = 0; ci < n; ci++) {
        size_t oi = ns[ci].orig_char_idx;
        uint32_t cp = ns[ci].cp;
        int ti = (oi < (size_t)n_orig_chars) ? c2t[oi] : -1;
        if (ti >= 0 && ti < seq_len) {
            tok_indices_data[ci] = static_cast<int64_t>(ti);
            char_ids_data[ci] = std::min(static_cast<int64_t>(cp), (int64_t)11999);
            if (ci > 0 && tok_indices_data[ci-1] == ti)
                pos_in_tok_data[ci] = std::min(pos_in_tok_data[ci-1] + 1, (int64_t)15);
        }
    }

    struct ForceSpan {
        int length;
        int st;
        int ed;
        std::string pos;
    };
    std::vector<ForceSpan> force_spans;

    // Dict features: for each char position i, find all words starting at i
    for (int i = 0; i < n; i++) {
        size_t bstart = ns_byte_starts[i];
        for (int len = 1; len <= MAX_WL && i + len <= n; len++) {
            // 원본 텍스트 기준 공백 경계를 넘어서는 매칭(Hallucination) 완벽 차단
            if (len > 1 && (ns[i + len - 1].orig_char_idx - ns[i].orig_char_idx != len - 1)) {
                break;
            }
            
            size_t bend = ns_byte_starts[i + len];
            std::string sub = ns_text.substr(bstart, bend - bstart);
            auto it = impl_->morph_word_map.find(sub);
            if (it != impl_->morph_word_map.end()) {
                uint64_t mask = it->second.first;
                int32_t wcost = it->second.second;
                
                int pidx_count = 0;
                bool all_substantive = true;

                for (int pidx = 0; pidx < NP; pidx++) {
                    if (mask & (1ULL << pidx)) {
                        dict_starts_data[i * NP + pidx] = 1.0f;
                        for (int k = 0; k < len && i+k < n; k++)
                            dict_covers_data[(i+k) * NP + pidx] = 1.0f;
                        
                        pidx_count++;
                        // NNG(0), NNP(1), NNB(2), NP(4), NR(5)
                        if (pidx != 0 && pidx != 1 && pidx != 2 && pidx != 4 && pidx != 5) {
                            all_substantive = false;
                        }
                    }
                }
                
                if (len >= 2 && pidx_count > 0 && all_substantive && wcost <= 3500) {
                    std::string target_pos = (mask & (1ULL << 1)) ? "NNP" : 
                                             (mask & (1ULL << 0)) ? "NNG" :
                                             (mask & (1ULL << 2)) ? "NNB" :
                                             (mask & (1ULL << 4)) ? "NP" : "NR";
                    force_spans.push_back({len, i, i + len - 1, target_pos});
                }
            }
        }
    }

    // ─── Step 5: Run ONNX (6 inputs) ───────────────────────────────────────
    Ort::MemoryInfo mem_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<float>   bert_data(hidden, hidden + seq_len * hdim);
    std::array<int64_t,3> bert_shape  = {1, static_cast<int64_t>(seq_len), static_cast<int64_t>(hdim)};
    std::array<int64_t,2> int2_shape  = {1, MC};
    std::array<int64_t,3> dict3_shape = {1, MC, NP};

    std::vector<Ort::Value> inputs;
    inputs.reserve(6);

    auto mk_f32 = [&](std::vector<float>& d, auto& sh) {
        return Ort::Value::CreateTensor<float>(
            mem_info, d.data(), d.size(), sh.data(), sh.size());
    };
    auto mk_i64 = [&](std::vector<int64_t>& d, auto& sh) {
        return Ort::Value::CreateTensor<int64_t>(
            mem_info, d.data(), d.size(), sh.data(), sh.size());
    };

    inputs.push_back(mk_f32(bert_data,       bert_shape));
    inputs.push_back(mk_i64(tok_indices_data, int2_shape));
    inputs.push_back(mk_i64(pos_in_tok_data,  int2_shape));
    inputs.push_back(mk_i64(char_ids_data,    int2_shape));
    inputs.push_back(mk_f32(dict_starts_data, dict3_shape));
    inputs.push_back(mk_f32(dict_covers_data, dict3_shape));

    const char* in_names[]  = {"bert_hidden","tok_indices","pos_in_tok","char_ids","dict_starts","dict_covers"};
    const char* out_names[] = {"logits"};

    std::vector<Ort::Value> ort_out;
    try {
        ort_out = impl_->morph_session->Run(
            Ort::RunOptions{nullptr},
            in_names, inputs.data(), 6,
            out_names, 1);
    } catch (...) { return; }

    // ─── Step 6: BIO decode → morphemes ────────────────────────────────────
    const float* logits  = ort_out[0].GetTensorData<float>();
    int n_labels = static_cast<int>(impl_->morph_labels.size());

    std::vector<int> forced_pred(n, -1);
    std::sort(force_spans.begin(), force_spans.end(), [](const ForceSpan& a, const ForceSpan& b) {
        return a.length > b.length;
    });
    std::vector<bool> covered(n, false);
    for (const auto& fs : force_spans) {
        if (fs.ed >= MC) continue;
        bool overlap = false;
        for (int k = fs.st; k <= fs.ed; k++) {
            if (covered[k]) { overlap = true; break; }
        }
        if (overlap) continue;
        
        std::string b_label = "B-" + fs.pos;
        std::string i_label = "I-" + fs.pos;
        
        int b_idx = -1, i_idx = -1;
        for (const auto& kv : impl_->morph_labels) {
            if (kv.second == b_label) b_idx = kv.first;
            if (kv.second == i_label) i_idx = kv.first;
        }
        
        if (b_idx != -1 && i_idx != -1) {
            forced_pred[fs.st] = b_idx;
            for (int k = fs.st + 1; k <= fs.ed; k++) {
                forced_pred[k] = i_idx;
            }
            for (int k = fs.st; k <= fs.ed; k++) covered[k] = true;
        }
    }

    std::string curr_surface, curr_pos;
    int curr_start = -1;

    auto flush = [&](int end_orig_idx) {
        if (curr_surface.empty()) return;
        out.morphemes.push_back({curr_surface, curr_pos, curr_start, end_orig_idx});
        curr_surface.clear();
        curr_pos.clear();
        curr_start = -1;
    };

    for (int ci = 0; ci < n; ci++) {
        const float* row = logits + ci * n_labels;
        int pred = (forced_pred[ci] != -1) ? forced_pred[ci] : argmax(row, n_labels);
        auto lit = impl_->morph_labels.find(pred);
        if (lit == impl_->morph_labels.end()) {
            flush(ci > 0 ? (int)ns[ci - 1].orig_char_idx + 1 : 0);
            continue;
        }

        const std::string& tag = lit->second;
        auto dash = tag.find('-');
        if (dash == std::string::npos) {
            flush(ci > 0 ? (int)ns[ci - 1].orig_char_idx + 1 : 0);
            continue;
        }

        char bio = tag[0];
        std::string pos = tag.substr(dash + 1);

        if (bio == 'B') {
            flush(ci > 0 ? (int)ns[ci - 1].orig_char_idx + 1 : 0);
            curr_surface = ns_text.substr(ns_byte_starts[ci], ns_byte_starts[ci+1] - ns_byte_starts[ci]);
            curr_pos     = pos;
            curr_start   = static_cast<int>(ns[ci].orig_char_idx);
        } else if (bio == 'I') {
            if (!curr_surface.empty()) {
                // Normal I: append to current morpheme
                curr_surface += ns_text.substr(ns_byte_starts[ci], ns_byte_starts[ci+1] - ns_byte_starts[ci]);
            } else {
                // I without preceding B: treat as B (match Python _run_morph behavior)
                curr_surface = ns_text.substr(ns_byte_starts[ci], ns_byte_starts[ci+1] - ns_byte_starts[ci]);
                curr_pos     = pos;
                curr_start   = static_cast<int>(ns[ci].orig_char_idx);
            }
        }
    }
    if (!curr_surface.empty() && n > 0)
        flush(static_cast<int>(ns[n-1].orig_char_idx) + 1);
}

// ═══════════════════════════════════════════════════════
// Init
// ═══════════════════════════════════════════════════════

bool ContextClassifier::init(const std::string& weights_dir,
                             const std::string& lang) {
    impl_->language = lang;

    // 언어 접두사 변환 헬퍼 (ko -> KR, en -> EN, ja -> JA)
    auto get_lang_prefix = [](const std::string& l) -> std::string {
        if (l == "ko" || l == "KR") return "KR";
        if (l == "en" || l == "EN") return "EN";
        if (l == "ja" || l == "JP" || l == "JA") return "JA";
        return "KR";
    };
    std::string prefix = get_lang_prefix(lang);

    // SNAP_HOME 환경변수 기반 앵커 탐색 (SNAP_HOME -> SNAP_ITN_HOME -> SNAP_WEIGHTS -> weights_dir)
    std::string snap_home;
    const char* env_home = std::getenv("SNAP_HOME");
    if (!env_home) env_home = std::getenv("SNAP_ITN_HOME");
    if (!env_home) env_home = std::getenv("SNAP_WEIGHTS");
    if (env_home && strlen(env_home) > 0) {
        snap_home = std::string(env_home);
    } else if (!weights_dir.empty()) {
        snap_home = weights_dir;
    } else {
        snap_home = ".";
    }

    auto strip_sub = [](std::string s) -> std::string {
        while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
        if (s.size() >= 7 && (s.substr(s.size() - 7) == "/models" || s.substr(s.size() - 7) == "\\models")) {
            return s.substr(0, s.size() - 7);
        }
        if (s.size() >= 8 && (s.substr(s.size() - 8) == "/weights" || s.substr(s.size() - 8) == "\\weights")) {
            return s.substr(0, s.size() - 8);
        }
        return s;
    };
    snap_home = strip_sub(snap_home);

    // 탐색 순서: SNAP_HOME/models/{lang}/ -> SNAP_HOME/{lang}/ -> SNAP_HOME/ (lang 폴더 직접)
    std::string ld;
    std::string weights_root = snap_home;
    {
        std::string cand_models = snap_home + "/models/" + lang;
        std::string cand_weights = snap_home + "/" + lang;
        std::ifstream f_models(cand_models + "/snap_config.json", std::ios::binary);
        std::ifstream f_weights(cand_weights + "/snap_config.json", std::ios::binary);
        std::ifstream f_direct(snap_home + "/snap_config.json", std::ios::binary);

        if (f_models.good()) {
            ld = cand_models;
        } else if (f_weights.good()) {
            ld = cand_weights;
        } else if (f_direct.good()) {
            ld = snap_home;
            size_t sep = snap_home.find_last_of("/\\");
            weights_root = (sep != std::string::npos) ? snap_home.substr(0, sep) : snap_home;
        } else {
            ld = cand_models;
        }
    }

    // Load snap_config.json first (needed for bert_model path)
    auto cfg = parse_json_utf8(ld + "/snap_config.json");
    if (cfg.is_discarded() || !cfg.is_object()) {
        std::cerr << "[SNAP Strict Policy Error] snap_config.json을 찾을 수 없습니다!\n"
                  << "  - SNAP_HOME 앵커: " << snap_home << "\n"
                  << "  - 탐색 상대 경로: " << ld << "\n"
                  << "  - 올바른 예시: SNAP_HOME 환경변수 설정 또는 snap_create(\"path/to/snap_root\", \"" << lang << "\")\n" << std::endl;
        return false;
    }
    impl_->g2p_threshold = cfg.value("g2p_threshold", 0.0f);

    // Load BERT (Strict Exit Policy: snap_config.json에 명시된 지정 모델 로드)
    bool use_int8 = cfg.value("use_int8", false);
    int num_threads = cfg.value("num_threads", 0);
    std::string device = cfg.value("device", "cpu");
    std::string default_bert = prefix + "_model_bert_int8.onnx";
    std::string bert_model_name = cfg.value("bert_model", default_bert);
    std::string bert_onnx = ld + "/" + bert_model_name;

    if (!bert_.load(bert_onnx, ld + "/tokenizer.json", num_threads, device)) {
        std::cerr << "[SNAP Strict Policy Error] 필수 BERT ONNX 모델을 로드할 수 없습니다!\n"
                  << "  - 대상 경로: " << bert_onnx << "\n"
                  << "  - 올바른 모델 파일명(" << prefix << "_model_bert_int8.onnx)이 지정된 디렉터리에 존재하는지 확인하십시오.\n";
        return false;
    }

    // Heteronym config
    {
        auto hcfg = cfg.value("heteronym", nlohmann::json::object());
        impl_->heteronym_conf_threshold =
            hcfg.value("conf_threshold", 0.6f);
    }

    auto load_if_enabled = [&](const std::string& key,
                               std::unique_ptr<Ort::Session>& sess,
                               std::map<int,std::string>& labels) {
        auto sec = cfg.value(key, nlohmann::json::object());
        if (!sec.value("enabled", false)) return;
        std::string onnx = ld + "/" + sec.value("head_onnx", key + "_head.onnx");
        std::string lmap = ld + "/" + sec.value("label_map", key + "_label_map.json");
        try { sess = load_head(impl_->env, onnx); } catch (...) { return; }
        labels = load_label_map(lmap);
    };

    load_if_enabled("semiotic",       impl_->semiotic_session,     impl_->semiotic_labels);
    load_if_enabled("korean_context", impl_->korean_context_session, impl_->kc_labels);

    // Heteronym head (fixed filename, no config enable key)
    {
        std::string h_onnx = ld + "/model_heteronym.onnx";
        std::ifstream hf(h_onnx, std::ios::binary);
        if (hf.good()) {
            try { impl_->heteronym_session = load_head(impl_->env, h_onnx); }
            catch (...) {}
        }
        impl_->heteronym_labels = {{0, "NONE"}, {1, "TENS"}};
    }

    // Beon head (fixed filename)
    {
        std::string b_onnx = ld + "/model_counter.onnx";
        std::ifstream bf(b_onnx, std::ios::binary);
        if (bf.good()) {
            try { impl_->beon_session = load_head(impl_->env, b_onnx); }
            catch (...) {}
        }
        impl_->beon_labels = {{0, "native"}, {1, "sino"}};
    }

    // G2P (yomi) Head
    {
        std::string g2p_onnx = ld + "/" + cfg.value("head_onnx", "yomi_head.onnx");
        std::string g2p_lmap = ld + "/label_map.json";
        auto g2p_j = parse_json_utf8(g2p_lmap);
        if (!g2p_j.is_discarded() && g2p_j.is_object()) {
            for (auto& [key, val] : g2p_j.items()) {
                int id = val.get<int>();
                impl_->g2p_labels[id] = key;
                impl_->g2p_label_map[key] = id;
            }
        }
        try { impl_->g2p_session = load_head(impl_->env, g2p_onnx); } catch (...) {}
    }

    // Korean heteronym targets (phon_heteronym.json)
    if (lang == "ko") {
        std::string ht_path = ld + "/phon_heteronym.json";
        auto hj = parse_json_utf8(ht_path);
        if (!hj.is_discarded() && hj.is_object()) {
            if (hj.contains("targets") && hj["targets"].is_array()) {
                for (auto& t : hj["targets"]) {
                    HeteronymTarget ht;
                    ht.word          = t.value("word", "");
                    ht.default_label = t.value("default", "NONE");
                    ht.always_tens   = t.value("always_tens", false);
                    if (ht.word.empty()) continue;
                    impl_->heteronym_targets.push_back(ht);
                    impl_->heteronym_defaults[ht.word] = ht.default_label;
                    if (ht.always_tens)
                        impl_->heteronym_always_tens.insert(ht.word);
                }
            }
        }
    }

    // Morph head (Korean or Japanese)
    if (lang == "ko" || lang == "ja") {
        auto morph_cfg = cfg.value("morph", nlohmann::json::object());
        if (morph_cfg.value("enabled", false)) {
            std::string morph_onnx = ld + "/" + morph_cfg.value("head_onnx", "morph_head_trie.onnx");
            try { impl_->morph_session = load_head(impl_->env, morph_onnx); } catch (...) {
                std::cerr << "[ContextClassifier] Warning: Missing morph ONNX model: " << morph_onnx << std::endl;
            }

            std::string morph_lmap = ld + "/" + morph_cfg.value("label_map", (lang == "ja" ? "morph_label_map_ja.json" : "morph_label_map.json"));
            impl_->morph_labels = load_label_map(morph_lmap);

            // Load morph word dictionary from binary export
            std::string morph_words_path = ld + "/morph_words.bin";
            std::ifstream mwf(morph_words_path, std::ios::binary);
            if (mwf.is_open()) {
                uint32_t num_words = 0;
                mwf.read(reinterpret_cast<char*>(&num_words), 4);
                impl_->morph_word_map.reserve(num_words);
                for (uint32_t wi = 0; wi < num_words; wi++) {
                    uint16_t wlen = 0;
                    mwf.read(reinterpret_cast<char*>(&wlen), 2);
                    std::string word(wlen, '\0');
                    mwf.read(word.data(), wlen);
                    uint64_t mask = 0;
                    mwf.read(reinterpret_cast<char*>(&mask), 8);
                    int32_t wcost = 10000;
                    mwf.read(reinterpret_cast<char*>(&wcost), 4);
                    impl_->morph_word_map[word] = {mask, wcost};
                }
            }
        }
    }

    // G2P targets (targets.json — Japanese/English)

    {
        std::string targets_path = ld + "/targets.json";
        auto tj = parse_json_utf8(targets_path);
        if (!tj.is_discarded() && tj.is_object()) {
            if (tj.is_object() && !impl_->g2p_label_map.empty()) {
                for (auto& [word, readings] : tj.items()) {
                    if (!readings.is_object()) continue;
                    impl_->g2p_target_words.push_back(word);
                    std::vector<int> ids;
                    for (auto& [reading, atype] : readings.items()) {
                        if (atype.is_string()) {
                            impl_->targets_ipa_map[word][reading] = atype.get<std::string>();
                        }
                        auto it = impl_->g2p_label_map.find(reading);
                        if (it != impl_->g2p_label_map.end()) {
                            ids.push_back(it->second);
                        } else if (atype.is_number_integer()) {
                            std::string combined = reading + ":" +
                                std::to_string(atype.get<int>());
                            auto it2 = impl_->g2p_label_map.find(combined);
                            if (it2 != impl_->g2p_label_map.end())
                                ids.push_back(it2->second);
                        }
                    }
                    if (!ids.empty())
                        impl_->g2p_word_valid_ids[word] = ids;
                }
            }
        }
    }

    if (lang == "ko") {
        impl_->phonology_kr.init(weights_dir);

        // Vowel Length Dictionary Loading
        auto vl_cfg = cfg.value("vowel_length", nlohmann::json::object());
        if (vl_cfg.value("enabled", false)) {
            std::string vl_dict_path = ld + "/" + vl_cfg.value("long_dict", "phon_vowel_length.json");
            auto vlj = parse_json_utf8(vl_dict_path);
            if (!vlj.is_discarded() && vlj.is_object()) {
                try {
                    if (vlj.is_object()) {
                        for (auto& [ch, entry] : vlj.items()) {
                            impl_->vowel_length_targets.insert(ch);
                            auto words = entry.value("words", nlohmann::json::array());
                            for (auto& w : words) {
                                if (w.is_string()) {
                                     impl_->vowel_length_long_set.insert(w.get<std::string>());
                                }
                            }
                        }
                    }
                } catch (...) {}
            }
        }
    } else if (lang == "ja") {
        if (!impl_->phonology_ja.init(weights_dir)) {
            std::cerr << "[SNAP] Warning: PhonologyJa init failed." << std::endl;
        }
    }
    // Text normalization (Korean)
    if (lang == "ko") {
        if (!impl_->text_normalize_kr.init(weights_root)) {
            // 영어 사전 로딩 실패 시 에러 (스펙지 읽기 모드로 fallback하지 않음)
            std::cerr << "[SNAP] 영어 발음 사전(dict_eng_merged.json)를 로드할 수 없습니다.\n"
                      << "       weights 경로: " << weights_root << "/ko/dict_eng_merged.json\n"
                      << "       영어 단어가 스펙지 읽기 모드로 처리됩니다." << std::endl;
            return false;
        }
    }

    return true;
}

// ═══════════════════════════════════════════════════════
// classify_span — shared by semiotic & number heads
// ═══════════════════════════════════════════════════════

/// Classify a span using either token-level or sequence-level head.
/// Detects head type by querying ONNX input shape dimensions.
static std::string classify_span_impl(
        Ort::Session& sess,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        size_t span_start, size_t span_end,
        const std::map<int,std::string>& id_to_label,
        const std::string& fallback) {

    // Detect head type: 2D [batch, hdim] = token-level, 3D [batch, seq, hdim] = seq-level
    auto input_info = sess.GetInputTypeInfo(0);
    auto shape = input_info.GetTensorTypeAndShapeInfo().GetShape();
    bool is_seq_level = (shape.size() >= 3);

    if (is_seq_level) {
        // Sequence-level: run on full hidden [1, seq_len, hdim] → [1, seq_len, num_labels]
        auto out = run_session(sess, hidden, {1, (int64_t)seq_len, (int64_t)hdim});
        int num_labels = (out.shape.size() >= 3) ?
            static_cast<int>(out.shape[2]) :
            static_cast<int>(out.data.size() / seq_len);

        // Extract + average logits for overlapping tokens
        std::vector<const float*> span_logits;
        for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
            auto [s, e] = offsets[i];
            if (e == 0) continue;
            if (s < span_end && e > span_start) {
                span_logits.push_back(out.data.data() + i * num_labels);
            }
        }
        if (span_logits.empty()) return fallback;

        std::vector<float> avg(num_labels, 0.0f);
        for (auto* row : span_logits)
            for (int j = 0; j < num_labels; j++)
                avg[j] += row[j];
        for (auto& v : avg)
            v /= static_cast<float>(span_logits.size());

        int best = argmax(avg.data(), num_labels);
        auto it = id_to_label.find(best);
        return it != id_to_label.end() ? it->second : fallback;
    } else {
        // Token-level: average hidden vectors → [1, hdim], then run head
        std::vector<const float*> token_vecs;
        for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
            auto [s, e] = offsets[i];
            if (e == 0) continue;
            if (s < span_end && e > span_start) {
                token_vecs.push_back(hidden + i * hdim);
            }
        }
        if (token_vecs.empty()) return fallback;

        std::vector<float> avg(hdim, 0.0f);
        for (auto* vec : token_vecs)
            for (int j = 0; j < hdim; j++)
                avg[j] += vec[j];
        float inv = 1.0f / static_cast<float>(token_vecs.size());
        for (auto& v : avg) v *= inv;

        auto out = run_session(sess, avg.data(), {1, (int64_t)hdim});
        int num_labels = static_cast<int>(out.data.size());

        int best = argmax(out.data.data(), num_labels);
        auto it = id_to_label.find(best);
        return it != id_to_label.end() ? it->second : fallback;
    }
}

// ═══════════════════════════════════════════════════════
// run_semiotic
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_semiotic(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    if (!impl_->semiotic_session) return;

    std::vector<std::pair<size_t, size_t>> claimed;
    auto is_claimed = [&](size_t s, size_t e) {
        for (auto& c : claimed) {
            if (c.first < e && c.second > s) return true;
        }
        return false;
    };

    // ── Phase 1: Rule-based patterns (Phone, IP) ──
    static const std::regex phone_patterns[] = {
        std::regex(R"(\+\d{1,3}[\-\.\s]?\d{1,2}[\-\.\s]?\d{3,4}[\-\.\s]?\d{3,4})"),
        std::regex(R"(01[016789][\-\.\s]?\d{3,4}[\-\.\s]?\d{4})"),
        std::regex(R"(02[\-\.\s]?\d{3,4}[\-\.\s]?\d{4})"),
        std::regex(R"(0\d{2}[\-\.\s]?\d{3,4}[\-\.\s]?\d{4})"),
        std::regex(R"(1[0-9]{3}[\-\.\s]?\d{4})")
    };

    for (auto& pat : phone_patterns) {
        auto it = std::sregex_iterator(text.begin(), text.end(), pat);
        auto end_it = std::sregex_iterator();
        for (; it != end_it; ++it) {
            size_t s = it->position();
            size_t e = s + it->length();
            if (is_claimed(s, e)) continue;
            claimed.push_back({s, e});
            out.semiotic.push_back({it->str(), (int)s, "phone"});
        }
    }

    static const std::regex ip_pat(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
    {
        auto it = std::sregex_iterator(text.begin(), text.end(), ip_pat);
        auto end_it = std::sregex_iterator();
        for (; it != end_it; ++it) {
            size_t s = it->position();
            size_t e = s + it->length();
            if (is_claimed(s, e)) continue;

            // Validate octets (0~255)
            std::string ip_str = it->str();
            std::stringstream ss(ip_str);
            std::string item;
            bool valid = true;
            while (std::getline(ss, item, '.')) {
                try {
                    int val = std::stoi(item);
                    if (val < 0 || val > 255) { valid = false; break; }
                } catch (...) { valid = false; break; }
            }
            if (valid) {
                claimed.push_back({s, e});
                out.semiotic.push_back({ip_str, (int)s, "ip"});
            }
        }
    }

    // ── Phase 2: Neural classification patterns ──
    static const std::regex neural_patterns[] = {
        std::regex(R"(\d+:\d+)"),
        std::regex(R"(\d+/\d+)"),
        std::regex(R"(\d{4}-\d{1,2}-\d{1,2})")
    };

    for (auto& pat : neural_patterns) {
        auto it = std::sregex_iterator(text.begin(), text.end(), pat);
        auto end_it = std::sregex_iterator();
        for (; it != end_it; ++it) {
            size_t s = it->position();
            size_t e = s + it->length();
            if (is_claimed(s, e)) continue;
            claimed.push_back({s, e});

            std::string span = it->str();
            std::string label = classify_span_impl(
                *impl_->semiotic_session,
                hidden, seq_len, hdim, offsets,
                s, e,
                impl_->semiotic_labels, "unknown");

            out.semiotic.push_back({span, (int)s, label});
        }
    }
}

// ═══════════════════════════════════════════════════════
// run_number
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_number(
        const std::string& text,
        const float* /*hidden*/, int /*seq_len*/, int /*hdim*/,
        const std::vector<std::pair<size_t,size_t>>& /*offsets*/,
        SnapResult& out) {
    // Rule-based, aligned with Python classifier._run_number()
    // native_units (34 items), sorted by UTF-8 byte length descending for longest-match
    static const std::vector<std::string> native_units = {
        "\xEC\x88\x9F\xEA\xB0\x80\xEB\xAD\x89",  // 숟가락 (9)
        "\xEB\xA7\x88\xEB\xA6\xAC",              // 마리 (6)
        "\xEC\xBC\x80\xEB\xA0\x88",              // 켤레 (6)
        "\xEC\x9E\x90\xEB\xA3\xA8",              // 자루 (6)
        "\xED\x8F\xAC\xEA\xB8\xB0",              // 포기 (6)
        "\xEA\xB7\xB8\xEB\xA6\xAC",              // 그릇 (6)
        "\xEB\xB0\x94\xED\x80\xB4",              // 바퀴 (6)
        "\xEA\xB0\x80\xEC\xA7\x80",              // 가지 (6)
        "\xEB\xAA\xA8\xEA\xB8\x88",              // 모금 (6)
        "\xEC\x9B\x80\xEC\xBB\xB4",              // 움큼 (6)
        "\xEA\xB7\xB8\xEB\xA3\xA8",              // 그루 (6)
        "\xEC\x86\xA1\xEC\x9D\xB4",              // 송이 (6)
        "\xEB\x8B\xA4\xEB\xB0\x9C",              // 다발 (6)
        "\xEB\xB4\x89\xEC\xA7\x80",              // 봉지 (6)
        "\xEC\x83\x81\xEC\x9E\x90",              // 상자 (6)
        "\xEB\xAB\xB6\xEC\x9D\x8C",              // 묶음 (6)
        "\xEA\xB0\x9C",                          // 개 (3)
        "\xEB\xAA\x85",                          // 명 (3)
        "\xEC\x82\xB4",                          // 살 (3)
        "\xEC\x9E\x94",                          // 잔 (3)
        "\xEB\xB3\x91",                          // 병 (3)
        "\xEB\xB2\x8C",                          // 벌 (3)
        "\xEC\xB1\x84",                          // 채 (3)
        "\xEA\xB3\xA1",                          // 곡 (3)
        "\xEC\xA4\x84",                          // 줄 (3)
        "\xEB\xBC\x98",                          // 뼘 (3)
        "\xED\x86\xA8",                          // 톨 (3)
        "\xEC\x8C\x8D",                          // 쌍 (3)
        "\xED\x86\xB5",                          // 통 (3)
        "\xEC\x8B\x9C",                          // 시 (3)
        "\xEA\xB6\x8C",                          // 권 (3)
        "\xED\x8E\xB8",                          // 편 (3)
        "\xEC\x9E\xA5",                          // 장 (3)
        "\xEB\x8C\x80"                           // 대 (3)
    };

    static const std::regex pat(R"(\d{1,3}(?:,\d{3})+|\d+)");
    auto it = std::sregex_iterator(text.begin(), text.end(), pat);
    auto end_it = std::sregex_iterator();
    for (; it != end_it; ++it) {
        auto& m = *it;
        std::string raw_span = m.str();
        std::string span;
        for (char c : raw_span) {
            if (c != ',') span += c;
        }
        int start = 0;
        size_t byte_pos = 0;
        size_t target_pos = static_cast<size_t>(m.position());
        while (byte_pos < target_pos && byte_pos < text.size()) {
            utf8_decode_adv(text, byte_pos);
            start++;
        }
        std::string after = text.substr(static_cast<size_t>(m.position() + m.length()));

        // Rule: 제 prefix → sino (ordinal)
        std::string before = text.substr(0, static_cast<size_t>(m.position()));
        bool is_sino = true;

        if (!before.empty()) {
            // Check if ends with "제" (EC A0 9C)
            static const std::string je = "\xEC\xA0\x9C";
            if (before.size() >= 3 && before.compare(before.size()-3, 3, je) == 0) {
                is_sino = true;
                out.numbers.push_back({span, start, "sino"});
                continue;
            }
        }

        // Try 100+ → sino
        long long num_val = 0;
        try { num_val = std::stoll(span); } catch (...) {}
        if (num_val >= 100) {
            out.numbers.push_back({span, start, "sino"});
            continue;
        }

        // Longest-match against native_units
        std::string label = "sino";
        for (auto& unit : native_units) {
            if (after.size() >= unit.size() &&
                after.compare(0, unit.size(), unit) == 0) {
                // Exception: '개월', '개년' 등은 접두사에 '개'를 포함하지만 sino 단위이므로 native '개' 매칭에서 제외
                if (unit == "\xEA\xB0\x9C") {  // "개"
                    if (after.compare(0, 6, "\xEA\xB0\x9C\xEC\x9B\x94") == 0 || // "개월"
                        after.compare(0, 6, "\xEA\xB0\x9C\xEB\x85\x84") == 0) { // "개년"
                        continue;
                    }
                }
                label = "native";
                break;
            }
        }
        out.numbers.push_back({span, start, label});
    }
}


// ═══════════════════════════════════════════════════════
// run_vowel_length
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_vowel_length(
        const std::string& text,
        SnapResult& out) {
    if (impl_->vowel_length_targets.empty()) return;

    // 1. Build non-space index map
    auto text_cps = parse_utf8(text);
    std::unordered_map<size_t, int> ns_idx_map;
    int no_sp_idx = 0;
    for (auto& cp : text_cps) {
        if (cp.cp != ' ') {
            ns_idx_map[cp.byte_start] = no_sp_idx;
            no_sp_idx++;
        }
    }

    // 2. Scan and match bigrams
    for (size_t i = 0; i + 1 < text_cps.size(); ++i) {
        uint32_t cp1 = text_cps[i].cp;
        std::string ch1_utf8 = utf8_encode_cp(cp1);
        
        if (impl_->vowel_length_targets.count(ch1_utf8)) {
            uint32_t cp2 = text_cps[i+1].cp;
            std::string ch2_utf8 = utf8_encode_cp(cp2);
            std::string bigram = ch1_utf8 + ch2_utf8;

            if (impl_->vowel_length_long_set.count(bigram)) {
                auto it = ns_idx_map.find(text_cps[i].byte_start);
                if (it != ns_idx_map.end()) {
                    out.vowel_length.push_back(it->second);
                }
            }
        }
    }

    std::sort(out.vowel_length.begin(), out.vowel_length.end());
}

// ═══════════════════════════════════════════════════════
// run_ko_extensions (korean_context head)
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_ko_extensions(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    if (!impl_->korean_context_session) return;

    auto cps = parse_utf8(text);

    struct MatchInfo {
        std::string type;   // "tens", "josa", "liaison"
        size_t span_start;  // byte offset
        size_t span_end;
    };

    std::vector<MatchInfo> all_matches;
    std::vector<std::vector<float>> word_hiddens;

    // Helper: find token index for byte position
    auto find_token = [&](size_t byte_pos) -> int {
        for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
            auto [s, e] = offsets[i];
            if (e == 0) continue;
            if (s <= byte_pos && byte_pos < e) return i;
        }
        return -1;
    };

    // 1. Tensification targets
    for (auto& word : impl_->tensification_targets) {
        size_t pos = 0;
        while ((pos = text.find(word, pos)) != std::string::npos) {
            int ti = find_token(pos);
            if (ti >= 0) {
                word_hiddens.push_back(
                    std::vector<float>(hidden + ti * hdim,
                                       hidden + ti * hdim + hdim));
                all_matches.push_back({"tens", pos, pos + word.size()});
            }
            pos += word.size();
        }
    }

    // 2. Josa '의' (UTF-8: EC 9D 98)
    {
        std::string ui("\xEC\x9D\x98");
        size_t pos = 0;
        while ((pos = text.find(ui, pos)) != std::string::npos) {
            int ti = find_token(pos);
            if (ti >= 0) {
                word_hiddens.push_back(
                    std::vector<float>(hidden + ti * hdim,
                                       hidden + ti * hdim + hdim));
                all_matches.push_back({"josa", pos, pos + ui.size()});
            }
            pos += ui.size();
        }
    }

    // 3. Liaison boundary
    // Check consecutive Hangul syllables: c1 has jongseong, c2 starts with 'ㅇ'
    for (size_t i = 0; i + 1 < cps.size(); i++) {
        uint32_t c1 = cps[i].cp;
        size_t c2_idx = i + 1;
        if (c2_idx < cps.size() && cps[c2_idx].cp == ' ') c2_idx++;
        if (c2_idx >= cps.size()) continue;
        uint32_t c2 = cps[c2_idx].cp;

        if (c1 >= 0xAC00 && c1 <= 0xD7A3 && (c1 - 0xAC00) % 28 > 0) {
            if (c2 >= 0xAC00 && c2 <= 0xD7A3 && (c2 - 0xAC00) / 588 == 11) {
                int ti = find_token(cps[i].byte_start);
                if (ti >= 0) {
                    word_hiddens.push_back(
                        std::vector<float>(hidden + ti * hdim,
                                           hidden + ti * hdim + hdim));
                    all_matches.push_back(
                        {"liaison", cps[i + 1].byte_start, cps[c2_idx].byte_end});
                }
            }
        }
    }

    if (word_hiddens.empty()) return;

    // Batch inference
    int64_t N = static_cast<int64_t>(word_hiddens.size());
    std::vector<float> batch(N * hdim);
    for (int64_t i = 0; i < N; i++)
        std::copy(word_hiddens[i].begin(), word_hiddens[i].end(),
                  batch.begin() + i * hdim);

    auto result = run_session(*impl_->korean_context_session,
                              batch.data(), {N, (int64_t)hdim});
    int num_labels = static_cast<int>(result.data.size() / N);

    for (int64_t i = 0; i < N; i++) {
        int pred = argmax(result.data.data() + i * num_labels, num_labels);
        auto it = impl_->kc_labels.find(pred);
        std::string label = (it != impl_->kc_labels.end()) ? it->second : "NONE";

        auto& mi = all_matches[i];
        if (mi.type == "tens" && label == "TENS") {
            out.annotations.push_back({(int)mi.span_start, (int)mi.span_end, "TENS"});
        } else if (mi.type == "josa" && label == "JOSA") {
            out.annotations.push_back({(int)mi.span_start, (int)mi.span_end, "JOSA"});
        } else if (mi.type == "liaison" &&
                   (label == "SUBSTANTIVE" || label == "FORMAL")) {
            out.annotations.push_back({(int)mi.span_start, (int)mi.span_end, label});
        }
    }
}

// ═══════════════════════════════════════════════════════
// process — main entry point
// ═══════════════════════════════════════════════════════

SnapResult ContextClassifier::process(const std::string& text) {
    auto bert_out = bert_.get_hidden_states(text);
    const float* hidden = bert_out.hidden_states.data();
    int seq_len = bert_out.seq_len;
    int hdim = bert_out.hidden_dim;
    auto& offsets = bert_out.offsets;

    SnapResult result;
    std::vector<std::tuple<int, int, std::string>> insertions;

    if (impl_->language == "ko") {
        run_heteronym(text, hidden, seq_len, hdim, offsets, result);
        run_beon(text, hidden, seq_len, hdim, offsets, result);
        run_morph(text, hidden, seq_len, hdim, offsets, result);
        // run_ko_extensions is removed as it's not used in latest Python version

        // Collect TENS from heteronym results
        for (const auto& hr : result.heteronym) {
            std::string label = hr.label;
            float conf = hr.conf;
            if (conf < impl_->heteronym_conf_threshold) {
                auto it = impl_->heteronym_defaults.find(hr.span);
                label = (it != impl_->heteronym_defaults.end()) ? it->second : "NONE";
            }
            if (label == "TENS") {
                int tens_start = hr.start + 3;  // Skip first character (3 bytes in UTF-8)
                if (tens_start < hr.end) {
                    insertions.push_back({tens_start, hr.end, "TENS"});
                }
            }
        }
    } else if (impl_->language == "ja") {
        run_morph(text, hidden, seq_len, hdim, offsets, result);
    }

    // Collect G2P insertions using a temporary SnapResult to prevent direct annotations modification
    SnapResult g2p_tmp;
    run_g2p(text, hidden, seq_len, hdim, offsets, g2p_tmp);
    for (const auto& ann : g2p_tmp.annotations) {
        insertions.push_back(ann);
    }

    // Remove overlaps and sort annotations
    if (!insertions.empty()) {
        std::sort(insertions.begin(), insertions.end(),
                  [](const std::tuple<int, int, std::string>& a,
                     const std::tuple<int, int, std::string>& b) {
                      return std::get<0>(a) > std::get<0>(b);
                  });
        std::vector<std::pair<int, int>> claimed;
        for (const auto& ann : insertions) {
            int start = std::get<0>(ann);
            int end   = std::get<1>(ann);
            const std::string& label = std::get<2>(ann);

            bool overlaps = false;
            for (const auto& cl : claimed) {
                if (cl.first < end && cl.second > start) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) {
                claimed.push_back({start, end});
                result.annotations.push_back(ann);
            }
        }
    }

    run_semiotic(text, hidden, seq_len, hdim, offsets, result);
    run_number(text, hidden, seq_len, hdim, offsets, result);

    // Apply counter predictions on numbers for Korean
    if (impl_->language == "ko" && !result.beon.empty()) {
        std::unordered_map<int, std::string> counter_map;
        for (const auto& c : result.beon) {
            counter_map[c.start] = c.label;
        }
        for (auto& num_item : result.numbers) {
            auto it = counter_map.find(num_item.start);
            if (it != counter_map.end()) {
                num_item.label = it->second;
            }
        }
    }

    run_vowel_length(text, result);

    if (impl_->language == "ko") {
        // 1. Scan normalized spans from original text (equivalent to python scan)
        auto spans = impl_->text_normalize_kr.scan(text, result.numbers);
        // 2. Apply scanned spans to get normalized text (equivalent to python apply_spans)
        result.normalized_text = impl_->text_normalize_kr.apply_spans(text, spans);
        // 3. Apply phonological rules on normalized text (equivalent to python apply_rules)
        result.phonology = impl_->phonology_kr.apply_rules(result.normalized_text, result);
    } else if (impl_->language == "ja") {
        result.phonology = impl_->phonology_ja.apply_rules(text, result.annotations, result.morphemes, result.accent_overrides);
    } else if (impl_->language == "en") {
        result.phonology = impl_->phonology_en.apply_rules(text, result, impl_->targets_ipa_map);
    }

    return result;
}

std::string ContextClassifier::normalize_text(const std::string& text) const {
    if (impl_->language == "ko") {
        return impl_->text_normalize_kr.normalize(text);
    }
    return text;  // No normalization for other languages yet
}

// ═══════════════════════════════════════════════════════
// run_g2p — G2P (yomi) head for Japanese kanji homographs
// ═══════════════════════════════════════════════════════

void ContextClassifier::run_g2p(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        SnapResult& out) {
    if (!impl_->g2p_session || impl_->g2p_target_words.empty()) return;

    bool use_word_boundary = (impl_->language == "en");

    // Detect target words present in text
    std::string text_lower;
    if (use_word_boundary) {
        text_lower = text;
        for (auto& c : text_lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    struct WordMatch { std::string word; size_t start; size_t end; };
    std::vector<WordMatch> all_matches;

    // Sort target words by length descending (longer first)
    auto sorted_targets = impl_->g2p_target_words;
    std::sort(sorted_targets.begin(), sorted_targets.end(),
              [](const std::string& a, const std::string& b) {
                  return a.size() > b.size();
              });

    // Track claimed spans
    std::vector<std::pair<size_t, size_t>> claimed;

    for (auto& word : sorted_targets) {
        if (use_word_boundary) {
            // English: word-boundary + case-insensitive via regex
            try {
                std::regex pat("\\b" + word + "\\b",
                               std::regex::icase | std::regex::optimize);
                auto it = std::sregex_iterator(text.begin(), text.end(), pat);
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    size_t sc = static_cast<size_t>(it->position());
                    size_t ec = sc + it->length();
                    bool overlaps = false;
                    for (auto& [cs, ce] : claimed)
                        if (sc < ce && ec > cs) { overlaps = true; break; }
                    if (!overlaps)
                        all_matches.push_back({word, sc, ec});
                }
            } catch (...) {}
        } else {
            // Japanese/Korean: substring matching
            size_t pos = 0;
            while ((pos = text.find(word, pos)) != std::string::npos) {
                size_t sc = pos, ec = pos + word.size();
                bool overlaps = false;
                for (auto& [cs, ce] : claimed)
                    if (sc < ce && ec > cs) { overlaps = true; break; }
                if (!overlaps)
                    all_matches.push_back({word, sc, ec});
                pos = ec;
            }
        }
        // Claim spans from this word
        for (auto& m : all_matches)
            if (m.word == word)
                claimed.push_back({m.start, m.end});
    }

    if (all_matches.empty()) return;

    // Run G2P head once: [1, seq_len, hdim] → [1, seq_len, num_labels]
    auto head_out = run_session(*impl_->g2p_session, hidden,
                                {1, (int64_t)seq_len, (int64_t)hdim});
    int num_labels = (head_out.shape.size() >= 3) ?
        static_cast<int>(head_out.shape[2]) :
        static_cast<int>(head_out.data.size() / seq_len);
    const float* logits = head_out.data.data();

    for (auto& m : all_matches) {
        // Find first token overlapping the word span
        int target_idx = -1;
        for (int i = 0; i < seq_len && i < (int)offsets.size(); i++) {
            auto [s, e] = offsets[i];
            if (s == 0 && e == 0) continue;
            if ((m.start <= s && s < m.end) || (m.start < e && e <= m.end)) { target_idx = i; break; }
        }
        if (target_idx < 0) continue;

        const float* token_logits = logits + target_idx * num_labels;

        // Restricted argmax over valid readings
        auto vit = impl_->g2p_word_valid_ids.find(m.word);
        if (vit == impl_->g2p_word_valid_ids.end() || vit->second.empty()) {
            continue; // Exclude from neural prediction if valid_ids is empty
        }

        int label_id = 0;
        auto& valid = vit->second;
        float best_val = -1e30f;
        for (int vid : valid) {
            if (vid < num_labels && token_logits[vid] > best_val) {
                best_val = token_logits[vid];
                label_id = vid;
            }
        }

        auto lit = impl_->g2p_labels.find(label_id);
        std::string label = (lit != impl_->g2p_labels.end()) ? lit->second : "O";

        if (label != "O" && label != "[PAD]") {
            auto probs = softmax(token_logits, num_labels);
            float confidence = probs[label_id];
            if (confidence >= impl_->g2p_threshold) {
                out.annotations.push_back(
                    {static_cast<int>(m.start), static_cast<int>(m.end), label});
            }
        }
    }
}

// ═══════════════════════════════════════════════════════
// SnapResult::to_json
// ═══════════════════════════════════════════════════════

std::string SnapResult::to_json() const {
    nlohmann::json j;

    // annotations
    nlohmann::json anns = nlohmann::json::array();
    for (auto& [s, e, l] : annotations)
        anns.push_back({s, e, l});
    j["annotations"] = anns;

    // semiotic
    nlohmann::json sem = nlohmann::json::array();
    for (auto& item : semiotic)
        sem.push_back({{"span", item.span}, {"start", item.start}, {"label", item.label}});
    j["semiotic"] = sem;

    // numbers
    nlohmann::json nums = nlohmann::json::array();
    for (auto& item : numbers)
        nums.push_back({{"span", item.span}, {"start", item.start}, {"label", item.label}});
    j["numbers"] = nums;

    // vowel_length
    j["vowel_length"] = vowel_length;

    // heteronym
    nlohmann::json het = nlohmann::json::array();
    for (auto& item : heteronym)
        het.push_back({{"span", item.span}, {"start", item.start}, {"end", item.end},
                       {"label", item.label}, {"conf", item.conf}});
    j["heteronym"] = het;

    // beon
    nlohmann::json ben = nlohmann::json::array();
    for (auto& item : beon)
        ben.push_back({{"span", item.span}, {"start", item.start}, {"end", item.end},
                       {"label", item.label}});
    j["beon"] = ben;

    // morphemes
    nlohmann::json morphs = nlohmann::json::array();
    for (auto& item : morphemes)
        morphs.push_back({{"surface", item.surface}, {"pos", item.pos},
                          {"start", item.start}, {"end", item.end}});
    j["morphemes"] = morphs;

    j["phonology"] = phonology;
    if (!normalized_text.empty())
        j["normalized_text"] = normalized_text;

    // accent_overrides
    nlohmann::json acc_ov = nlohmann::json::object();
    for (auto& [k, v] : accent_overrides) {
        acc_ov[k] = v;
    }
    j["accent_overrides"] = acc_ov;

    return j.dump();
}

}  // namespace snap
