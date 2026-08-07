/**
 * SNAP C++ E2E Test Runner (Batch & Interactive CLI)
 * ==================================================
 * Usage:
 *   1) Batch Mode:
 *      ./test_e2e <weights_dir> <lang> "<text>"
 *   2) Interactive Mode:
 *      ./test_e2e <weights_dir> <lang>
 */

#include "snap/snap_api.h"
#include "snap/tokenizer.h"
#include <iostream>
#include <fstream>
#include <string>

void print_usage(const char* prog) {
    std::cout << "Usage:\n";
    std::cout << "  1) Batch Mode:\n";
    std::cout << "     " << prog << " <weights_dir> <lang> \"<text_to_process>\"\n\n";
    std::cout << "  2) Interactive CLI Mode:\n";
    std::cout << "     " << prog << " <weights_dir> <lang>\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog << " ../snap/weights ko \"안과에 갔다.\"\n";
    std::cout << "  " << prog << " ../snap/weights ja\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string weights_dir = argv[1];
    const std::string lang = argv[2];

    std::cout << "[SNAP] Initializing engine for language '" << lang << "'...\n";
    std::cout << "[SNAP] Weights directory: " << weights_dir << "\n";

    // 1. Standard mode: Automatically reads manifest.json and loads active versions
    void* handle = snap_create(weights_dir.c_str(), lang.c_str());

    // Note: To pin a specific dictionary or model version explicitly, you can also use:
    // void* handle = snap_create_with_version(weights_dir.c_str(), lang.c_str(), nullptr, "v1.0.0", "v1.0.0");

    if (!handle) {
        std::cerr << "[SNAP] ERROR: snap_create failed to load models and dictionaries from '" 
                  << weights_dir << "'!\n";
        return -1;
    }
    std::cout << "[SNAP] Engine initialized successfully.\n\n";

    if (argc >= 4) {
        // Batch mode: Process single sentence
        const std::string text = argv[3];
        std::cout << "--- Input ---\n" << text << "\n\n";

        // Dump Tokenizer info
        snap::BertTokenizer tok;
        std::string tok_path = weights_dir + "/models/" + lang + "/tokenizer.json";
        std::ifstream tf(tok_path);
        if (!tf.good()) tok_path = weights_dir + "/" + lang + "/tokenizer.json";
        if (tok.load(tok_path)) {
            auto enc = tok.encode(text);
            std::cout << "--- C++ Tokenizer Dump ---\n";
            std::cout << "Input IDs: ";
            for (auto id : enc.input_ids) std::cout << id << " ";
            std::cout << "\nOffsets: ";
            for (auto& off : enc.offsets) std::cout << "(" << off.first << ", " << off.second << ") ";
            std::cout << "\n--------------------------\n\n";
        }

        std::cout << "--- G2P Result ---\n";
        const char* result = snap_process(handle, text.c_str());
        if (result) {
            std::cout << result << "\n";
            snap_free(result);
        } else {
            std::cerr << "[SNAP] ERROR: Inference failed.\n";
        }
        std::cout << "-------------\n";
    } else {
        // Interactive mode
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
