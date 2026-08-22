#pragma once
#include "bert_session.h"
#include "text_normalize_kr.h"
#include "config.h"

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <unordered_set>

namespace snap {

struct SemioticItem {
    std::string span;
    int start;
    std::string label;
};

struct NumberItem {
    std::string span;
    int start;
    std::string label;
};

struct HeteronymItem {
    std::string span;
    int start, end;
    std::string label;   // "TENS" or "NONE"
    float conf = 0.0f;
};

struct CounterItem {
    std::string span;
    int start, end;
    std::string label;   // "native" or "sino"
};

struct MorphItem {
    std::string surface;
    std::string pos;
    int start, end;
};

struct PauseInfo {
    int word_idx = 0;
    std::string word = "";
    std::string pause = "NONE"; // "NONE", "P1", "P2", "P3"
};

/// target entry from heteronym_targets.json
struct HeteronymTarget {
    std::string word;
    std::string default_label;  // "NONE" or "TENS"
    bool always_tens = false;
};

struct SnapResult {
    std::vector<std::tuple<int, int, std::string>> annotations;  // (start, end, label)
    std::vector<SemioticItem>   semiotic;
    std::vector<NumberItem>     numbers;
    std::vector<int>            vowel_length;
    std::vector<HeteronymItem>  heteronym;
    std::vector<CounterItem>       counter;
    std::vector<MorphItem>      morphemes;
    std::vector<PauseInfo>      pauses;
    std::string                 phonology;
    std::string                 normalized_text;  // text after normalization
    std::unordered_map<std::string, int> accent_overrides;

    /// Serialize to JSON string
    std::string to_json() const;
};

/// SNAP ContextClassifier — runs all heads in a single BERT pass
class ContextClassifier {
public:
    ContextClassifier();
    ~ContextClassifier();
    ContextClassifier(ContextClassifier&&) noexcept;
    ContextClassifier& operator=(ContextClassifier&&) noexcept;

    /// Load BERT + all head ONNX sessions + configs from weights_dir/lang/
    bool init(const std::string& weights_dir, const std::string& lang, const std::string& device = "");

    /// Run all enabled heads. BERT is called once.
    /// Text normalization is applied automatically for Korean.
    SnapResult process(const std::string& text, const std::string& config_file = "");

    /// Run all enabled heads in batch mode. BERT is called once for multiple sentences.
    std::vector<SnapResult> process_batch(const std::vector<std::string>& texts, const std::string& config_file = "");

    /// Normalize text only (no BERT inference)
    std::string normalize_text(const std::string& text) const;

    /// Access underlying BERT session
    BertSession& get_bert_session() { return bert_; }
    const BertSession& get_bert_session() const { return bert_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    BertSession bert_;

    void process_single_with_hidden(
        const std::string& text,
        const float* hidden, int seq_len, int hdim,
        const std::vector<std::pair<size_t,size_t>>& offsets,
        const SnapGlobalConfig& global_cfg,
        SnapResult& result);

    // Head runners
    void run_semiotic(const std::string& text,
                      const float* hidden, int seq_len, int hdim,
                      const std::vector<std::pair<size_t,size_t>>& offsets,
                      SnapResult& out);

    void run_number(const std::string& text,
                    const float* hidden, int seq_len, int hdim,
                    const std::vector<std::pair<size_t,size_t>>& offsets,
                    SnapResult& out);

    void run_vowel_length(const std::string& text,
                          SnapResult& out);

    void run_ko_extensions(const std::string& text,
                           const float* hidden, int seq_len, int hdim,
                           const std::vector<std::pair<size_t,size_t>>& offsets,
                           SnapResult& out);

    void run_g2p(const std::string& text,
                 const float* hidden, int seq_len, int hdim,
                 const std::vector<std::pair<size_t,size_t>>& offsets,
                 SnapResult& out);

    void run_heteronym(const std::string& text,
                       const float* hidden, int seq_len, int hdim,
                       const std::vector<std::pair<size_t,size_t>>& offsets,
                       SnapResult& out);

    void run_counter(const std::string& text,
                  const float* hidden, int seq_len, int hdim,
                  const std::vector<std::pair<size_t,size_t>>& offsets,
                  SnapResult& out);

    void run_morph(const std::string& text,
                   const float* hidden, int seq_len, int hdim,
                   const std::vector<std::pair<size_t,size_t>>& offsets,
                   SnapResult& out);

    void run_morph_batch(const std::vector<std::string>& texts,
                         const float* hidden_batch,
                         const std::vector<int>& seq_lens,
                         int max_seq_len, int hdim,
                         const std::vector<std::vector<std::pair<size_t,size_t>>>& offsets_batch,
                         std::vector<SnapResult>& results);
};

}  // namespace snap
