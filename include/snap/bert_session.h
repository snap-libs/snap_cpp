#pragma once
#include "tokenizer.h"
#include <memory>
#include <string>
#include <vector>

namespace snap {

/// BERT ONNX session — loads model.onnx and runs inference
class BertSession {
public:
    BertSession();
    ~BertSession();
    BertSession(BertSession&&) noexcept;
    BertSession& operator=(BertSession&&) noexcept;

    bool load(const std::string& model_onnx_path,
              const std::string& tokenizer_json_path,
              int num_threads = 0,
              const std::string& device = "cpu");

    struct Output {
        std::vector<float> hidden_states;  // flattened [1, seq_len, hidden_dim]
        int seq_len = 0;
        int hidden_dim = 768;
        std::vector<std::pair<size_t, size_t>> offsets;  // byte offsets per token
    };

    Output get_hidden_states(const std::string& text);

    const BertTokenizer& tokenizer() const { return tokenizer_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    BertTokenizer tokenizer_;
};

}  // namespace snap
