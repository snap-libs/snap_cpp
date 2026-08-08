// snap_cpp/src/config.cpp — Implementation of SnapGlobalConfig for C++ core
#include "snap/config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>

namespace snap {

// Helper basic JSON parser for models/snap_config.json
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\"");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\"");
    return str.substr(first, (last - first + 1));
}

SnapGlobalConfig SnapGlobalConfig::LoadOrDefault(const std::string& config_path) {
    SnapGlobalConfig cfg; // Starts with defaults

    std::string target_path = config_path;
    if (target_path.empty()) {
        const char* env_home = std::getenv("SNAP_HOME");
        if (!env_home) env_home = std::getenv("SNAP_ITN_HOME");
        if (!env_home) env_home = std::getenv("SNAP_WEIGHTS");
        std::string root = (env_home && strlen(env_home) > 0) ? env_home : ".";
        while (!root.empty() && (root.back() == '/' || root.back() == '\\')) root.pop_back();
        target_path = root + "/models/snap_config.json";
    }

    std::ifstream file(target_path);
    if (!file.is_open()) {
        return cfg;
    }


    std::string line;
    std::string current_lang = "";
    while (std::getline(file, line)) {
        if (line.find("\"ko\"") != std::string::npos) current_lang = "ko";
        else if (line.find("\"ja\"") != std::string::npos) current_lang = "ja";
        else if (line.find("\"en\"") != std::string::npos) current_lang = "en";

        if (current_lang == "ko") {
            if (line.find("\"vowel_length\"") != std::string::npos) {
                if (line.find("true") != std::string::npos) cfg.ko.vowel_length = true;
                else if (line.find("false") != std::string::npos) cfg.ko.vowel_length = false;
            }
            if (line.find("\"to_ipa\"") != std::string::npos) {
                if (line.find("true") != std::string::npos) cfg.ko.to_ipa = true;
                else if (line.find("false") != std::string::npos) cfg.ko.to_ipa = false;
            }
        }
        else if (current_lang == "ja") {
            if (line.find("\"script\"") != std::string::npos) {
                if (line.find("hiragana") != std::string::npos) cfg.ja.script = "hiragana";
                else if (line.find("romaji") != std::string::npos) cfg.ja.script = "romaji";
                else if (line.find("katakana") != std::string::npos) cfg.ja.script = "katakana";
            }
            if (line.find("\"pitch_accent\"") != std::string::npos) {
                if (line.find("true") != std::string::npos) cfg.ja.pitch_accent = true;
                else if (line.find("false") != std::string::npos) cfg.ja.pitch_accent = false;
            }
            if (line.find("\"to_ipa\"") != std::string::npos) {
                if (line.find("true") != std::string::npos) cfg.ja.to_ipa = true;
                else if (line.find("false") != std::string::npos) cfg.ja.to_ipa = false;
            }
        }
        else if (current_lang == "en") {
            if (line.find("\"to_ipa\"") != std::string::npos) {
                if (line.find("true") != std::string::npos) cfg.en.to_ipa = true;
                else if (line.find("false") != std::string::npos) cfg.en.to_ipa = false;
            }
        }
    }

    return cfg;
}

} // namespace snap
