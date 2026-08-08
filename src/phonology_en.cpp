#include "snap/phonology_en.h"
#include <algorithm>
#include <iostream>

namespace snap {

struct Replacement {
    size_t start;
    size_t end;
    std::string tag;
};

static std::string trim_quotes(const std::string& str) {
    if (str.empty()) return str;
    size_t first = 0;
    size_t last = str.size() - 1;
    while (first <= last && (str[first] == '\'' || str[first] == ' ')) first++;
    while (last >= first && (str[last] == '\'' || str[last] == ' ')) last--;
    if (first > last) return "";
    return str.substr(first, last - first + 1);
}

static std::string to_lower_utf8(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower;
}

PhonologyEn::PhonologyEn() {}
PhonologyEn::~PhonologyEn() {}

bool PhonologyEn::init(const std::string& /*weights_dir*/) {
    return true;
}

std::string PhonologyEn::apply_rules(
    const std::string& text,
    const SnapResult& result,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& targets_ipa_map,
    bool to_ipa) {
    
    std::vector<Replacement> replacements;

    // 1. Process Heteronyms from result.annotations
    for (const auto& ann : result.annotations) {
        int start = std::get<0>(ann);
        int end   = std::get<1>(ann);
        std::string label = std::get<2>(ann);

        if (start < 0 || end > (int)text.size() || start >= end) {
            continue;
        }

        std::string span = text.substr(start, end - start);
        std::string word_key = to_lower_utf8(span);

        // Find IPA reading
        std::string ipa = "";
        auto it_word = targets_ipa_map.find(word_key);
        if (it_word != targets_ipa_map.end()) {
            auto it_lbl = it_word->second.find(label);
            if (it_lbl != it_word->second.end()) {
                ipa = it_lbl->second;
            }
        }

        if (!ipa.empty()) {
            std::string clean_ipa = trim_quotes(ipa);
            std::string tag = to_ipa ? ("[" + clean_ipa + "]") :
                              ("<say-as interpret-as=\"pronunciation\" detail=\"/" + clean_ipa + "/\">" + span + "</say-as>");
            replacements.push_back({ (size_t)start, (size_t)end, tag });
        }
    }


    // 2. Process Semiotics from result.semiotic
    for (const auto& sem : result.semiotic) {
        if (sem.start < 0 || sem.span.empty() || sem.start + (int)sem.span.size() > (int)text.size()) {
            continue;
        }
        size_t start = (size_t)sem.start;
        size_t end = start + sem.span.size();
        std::string tag = "<say-as interpret-as=\"" + sem.label + "\">" + sem.span + "</say-as>";
        replacements.push_back({ start, end, tag });
    }

    if (replacements.empty()) {
        return text;
    }

    // Sort replacements by start index descending to avoid index shifts
    std::sort(replacements.begin(), replacements.end(), [](const Replacement& a, const Replacement& b) {
        return a.start > b.start;
    });

    // Apply replacements from right to left while avoiding overlaps
    std::vector<std::pair<size_t, size_t>> claimed;
    std::string out_text = text;
    for (const auto& r : replacements) {
        bool overlap = false;
        for (const auto& c : claimed) {
            if (c.first < r.end && c.second > r.start) {
                overlap = true;
                break;
            }
        }
        if (overlap) {
            continue;
        }
        claimed.push_back({ r.start, r.end });
        out_text.replace(r.start, r.end - r.start, r.tag);
    }

    return out_text;
}

} // namespace snap
