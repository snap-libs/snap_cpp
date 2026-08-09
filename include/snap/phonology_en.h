#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "classifier.h"

namespace snap {

class PhonologyEn {
public:
    PhonologyEn();
    ~PhonologyEn();

    bool init(const std::string& weights_dir);

    // Apply English phonology rules to output SSML tagged string.
    // Map: Word -> (Label -> IPA pronunciation)
    std::string apply_rules(
        const std::string& text,
        const SnapResult& result,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& targets_ipa_map,
        bool to_ipa = false);

};

} // namespace snap
