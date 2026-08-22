/**
 * SNAP C++ E2E Test Runner (Batch & Interactive CLI)
 * ==================================================
 * Usage:
 *   1) Default Mode (Smart Auto-Path Fallback):
 *      ./test_e2e
 *   2) Explicit Mode:
 *      ./test_e2e <weights_dir> [lang] ["<text>"]
 */

#include "snap/snap_api.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#if __has_include("snap/tokenizer.h")
#include "snap/tokenizer.h"
#define HAS_SNAP_TOKENIZER 1
#else
#define HAS_SNAP_TOKENIZER 0
#endif

void print_usage(const char* prog) {
    std::cout << "Usage:\n";
    std::cout << "  1) Quick Mode (Smart Auto-Path):\n";
    std::cout << "     " << prog << "\n\n";
    std::cout << "  2) Batch Mode:\n";
    std::cout << "     " << prog << " <weights_dir> <lang> \"<text_to_process>\"\n\n";
    std::cout << "  3) Interactive CLI Mode:\n";
    std::cout << "     " << prog << " <weights_dir> <lang>\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog << " . ko \"2024년 5월 28일 오후 3시 만남\"\n";
    std::cout << "  " << prog << " ./models ja\n";
}

int main(int argc, char* argv[]) {
    std::string weights_dir = ".";
    std::string lang = "ko";
    std::string text = "2024년 5월 28일 오후 3시 45분에 만납시다.";

    if (argc >= 2) weights_dir = argv[1];
    if (argc >= 3) lang = argv[2];
    if (argc >= 4) text = argv[3];

    // Smart path fallback candidates if direct initialization fails
    std::vector<std::string> path_candidates = {
        weights_dir,
        ".",
        "./models",
        "..",
        "../models",
        "../..",
        "/mnt/c/work/snap",
        "c:/work/snap"
    };

    auto check_path_valid = [](const std::string& p, const std::string& l) -> bool {
        if (p.empty()) return false;
        std::string prefix = (l == "ko" ? "KO" : (l == "ja" ? "JA" : "EN"));
        std::vector<std::string> test_files = {
            p + "/models/" + l + "/" + prefix + "_model_index.json",
            p + "/" + l + "/" + prefix + "_model_index.json"
        };
        for (const auto& tf : test_files) {
            std::ifstream f(tf);
            if (f.good()) return true;
        }
        return false;
    };

    void* handle = nullptr;
    std::string resolved_path = weights_dir;

    for (const auto& path : path_candidates) {
        if (check_path_valid(path, lang)) {
            handle = snap_create(path.c_str(), lang.c_str());
            if (handle) {
                resolved_path = path;
                break;
            }
        }
    }

    if (!handle) {
        std::cerr << "[SNAP] ERROR: snap_create failed to load models and dictionaries for language '"
                  << lang << "'!\n";
        std::cerr << "[SNAP] Searched candidate paths: ";
        for (const auto& p : path_candidates) std::cerr << "'" << p << "' ";
        std::cerr << "\n";
        print_usage(argv[0]);
        return -1;
    }

    std::cout << "[SNAP] Initialized engine for language '" << lang << "'...\n";
    std::cout << "[SNAP] Resolved asset path: " << resolved_path << "\n\n";

    if (argc >= 4 || argc == 1) {
        std::cout << "--- Input ---\n" << text << "\n\n";

#if HAS_SNAP_TOKENIZER
        snap::BertTokenizer tok;
        std::string tok_path = resolved_path + "/models/" + lang + "/tokenizer.json";
        std::ifstream tf(tok_path);
        if (!tf.good()) tok_path = resolved_path + "/" + lang + "/tokenizer.json";
        if (tok.load(tok_path)) {
            auto enc = tok.encode(text);
            std::cout << "--- C++ Tokenizer Dump ---\n";
            std::cout << "Input IDs: ";
            for (auto id : enc.input_ids) std::cout << id << " ";
            std::cout << "\nOffsets: ";
            for (auto& off : enc.offsets) std::cout << "(" << off.first << ", " << off.second << ") ";
            std::cout << "\n--------------------------\n\n";
        }
#endif

        std::cout << "--- G2P Result ---\n";
        const char* result = snap_process(handle, text.c_str());
        if (result) {
            std::cout << result << "\n";
            snap_free(result);
        } else {
            std::cerr << "[SNAP] ERROR: Inference failed.\n";
        }
        std::cout << "-------------\n\n";

        std::cout << "--- Raw BERT Features C-API Test ---\n";
        int seq_len = 0, hidden_dim = 0, w2ph_len = 0;
        int* w2ph = nullptr;
        float* bert_features = snap_get_bert_features(handle, text.c_str(), &seq_len, &hidden_dim, &w2ph, &w2ph_len);
        if (bert_features) {
            std::cout << "  ✅ Successfully exported BERT hidden states via C-API!\n";
            std::cout << "  - seq_len:    " << seq_len << "\n";
            std::cout << "  - hidden_dim: " << hidden_dim << "\n";
            std::cout << "  - word2ph len:" << w2ph_len << "\n";
            std::cout << "  - First 5 float features: ";
            for (int i = 0; i < (seq_len * hidden_dim > 5 ? 5 : seq_len * hidden_dim); ++i) {
                std::cout << bert_features[i] << " ";
            }
            std::cout << "\n";
            snap_free_tensor(bert_features);
            if (w2ph) snap_free_tensor(w2ph);
        } else {
            std::cerr << "[SNAP] ERROR: snap_get_bert_features failed.\n";
        }
        std::cout << "------------------------------------\n";
    } else {
        std::cout << "=== Interactive CLI Mode ===\n";
        std::cout << "Type a sentence and press Enter. Type 'exit' or 'quit' to stop.\n\n";

        std::string input_line;
        while (true) {
            std::cout << "[snap_cpp:" << lang << "]> ";
            if (!std::getline(std::cin, input_line)) {
                break;
            }
            if (input_line == "exit" || input_line == "quit") {
                break;
            }
            if (input_line.empty()) {
                continue;
            }

            const char* result = snap_process(handle, input_line.c_str());
            if (result) {
                std::cout << result << "\n\n";
                snap_free(result);
            } else {
                std::cerr << "[SNAP] ERROR: Inference failed.\n\n";
            }
        }
    }

    std::cout << "[SNAP] Shutting down engine...\n";
    snap_destroy(handle);
    std::cout << "[SNAP] Shutdown complete.\n";

    return 0;
}
