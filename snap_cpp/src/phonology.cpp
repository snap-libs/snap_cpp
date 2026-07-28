#include "snap/phonology.h"
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace snap {

// UTF-8 Helper Functions
static std::vector<uint32_t> utf8_to_codepoints(const std::string& str) {
    std::vector<uint32_t> res;
    res.reserve(str.size());
    for (size_t i = 0; i < str.size(); ) {
        uint8_t c = str[i];
        uint32_t cp = 0;
        size_t len = 0;
        if (c < 0x80) {
            cp = c;
            len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            len = 4;
        } else {
            i++;
            continue;
        }
        if (i + len > str.size()) {
            break;
        }
        for (size_t j = 1; j < len; j++) {
            cp = (cp << 6) | (str[i + j] & 0x3F);
        }
        res.push_back(cp);
        i += len;
    }
    return res;
}

static std::string codepoint_to_utf8(uint32_t cp) {
    std::string res;
    if (cp < 0x80) {
        res.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        res.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        res.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x110000) {
        res.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return res;
}

static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

// Korean Hangul Components
static const std::vector<std::string> CHOSUNGS = {
    "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
};
static const std::vector<std::string> JUNGSUNGS = {
    "ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ", "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ"
};
static const std::vector<std::string> JONGSUNGS = {
    "", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ", "ㅁ", "ㅂ", "ㅄ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
};

static int get_cho_idx(const std::string& cho) {
    for (size_t i = 0; i < CHOSUNGS.size(); ++i) {
        if (CHOSUNGS[i] == cho) return static_cast<int>(i);
    }
    return -1;
}
static int get_jung_idx(const std::string& jung) {
    for (size_t i = 0; i < JUNGSUNGS.size(); ++i) {
        if (JUNGSUNGS[i] == jung) return static_cast<int>(i);
    }
    return -1;
}
static int get_jong_idx(const std::string& jong) {
    for (size_t i = 0; i < JONGSUNGS.size(); ++i) {
        if (JONGSUNGS[i] == jong) return static_cast<int>(i);
    }
    return -1;
}

// Article 16 JAMO_NAMES
static const std::unordered_map<uint32_t, std::string> JAMO_NAMES = {
    {0x3131, "기역"}, {0x3132, "쌍기역"}, {0x3134, "니은"}, {0x3137, "디귿"}, {0x3138, "쌍디귿"},
    {0x3139, "리을"}, {0x3141, "미음"}, {0x3142, "비읍"}, {0x3143, "쌍비읍"}, {0x3145, "시옷"},
    {0x3146, "쌍시옷"}, {0x3147, "이응"}, {0x3148, "지읒"}, {0x3149, "쌍지읒"}, {0x314A, "치읓"},
    {0x314B, "키읔"}, {0x314C, "티읕"}, {0x314D, "피읖"}, {0x314E, "히읗"}
};

// Article 17 Aspiration
static const std::unordered_map<std::string, std::string> PALATAL_JUNG_MAP = {
    {"ㅣ", "ㅣ"}, {"ㅑ", "ㅏ"}, {"ㅕ", "ㅓ"}, {"ㅛ", "ㅗ"}, {"ㅠ", "ㅜ"}, {"ㅖ", "ㅔ"}, {"ㅒ", "ㅐ"}
};

// Tensification map
static const std::unordered_map<std::string, std::string> TENSE_MAP = {
    {"ㄱ", "ㄲ"}, {"ㄷ", "ㄸ"}, {"ㅂ", "ㅃ"}, {"ㅅ", "ㅆ"}, {"ㅈ", "ㅉ"}
};

static const std::unordered_set<std::string> TENSE_TRIGGER_JONGS = {
    "ㄱ", "ㄲ", "ㅋ", "ㄳ", "ㄺ",
    "ㄷ", "ㅅ", "ㅆ", "ㅈ", "ㅊ", "ㅌ",
    "ㅂ", "ㅍ", "ㄼ", "ㄿ", "ㅄ",
    "ㄽ", "ㄾ"
};

// Substantive morpheme identification POS set
// VCP(이다 서술격조사), VCN(형용사화접미사)은 앞 형태소와 연음되므로 제외
static const std::unordered_set<std::string> SUBSTANTIVE_POS = {
    "NNG", "NNP", "NNB", "NP", "NR",
    "VV", "VA", "VX",          // VCN, VCP 제외 — 서술격조사는 연음 적용
    "MM", "MAG", "MAJ",
    "XR", "IC"
};

// 오분류 보완용: 앞 형태소에 이 POS가 있으면 다음 NNG는 어미로 간주하여 연음 허용
// 예: 같(VA)+은(NNG오분류) → '가튜', 같(ETM)+은(NNG오분류) → '가튜'
static const std::unordered_set<std::string> VERBAL_POS = {
    "VA", "VV", "VX", "VCP", "VCN",
    "ETM", "EC", "EF", "EP",
    "XSV", "XSA"
};

// Double batchim liaison
static const std::unordered_map<std::string, std::pair<std::string, std::string>> DOUBLE_JONG_LIAISON = {
    {"ㄳ", {"ㄱ", "ㅆ"}},
    {"ㄵ", {"ㄴ", "ㅈ"}},
    {"ㄺ", {"ㄹ", "ㄱ"}},
    {"ㄻ", {"ㄹ", "ㅁ"}},
    {"ㄼ", {"ㄹ", "ㅂ"}},
    {"ㄽ", {"ㄹ", "ㅆ"}},
    {"ㄾ", {"ㄹ", "ㅌ"}},
    {"ㄿ", {"ㄹ", "ㅍ"}},
    {"ㅄ", {"ㅂ", "ㅆ"}}
};

// Double batchim simplification
static const std::unordered_map<std::string, std::string> SIMPL_MAP = {
    {"ㄳ", "ㄱ"}, {"ㄵ", "ㄴ"}, {"ㄶ", "ㄴ"}, {"ㄺ", "ㄱ"}, {"ㄻ", "ㅁ"},
    {"ㄼ", "ㄹ"}, {"ㄽ", "ㄹ"}, {"ㄾ", "ㄹ"}, {"ㄿ", "ㅂ"},
    {"ㅀ", "ㄹ"}, {"ㅄ", "ㅂ"}
};

// Neutralization representation
static const std::unordered_map<std::string, std::string> NEUT_MAP = {
    {"ㄱ", "ㄱ"}, {"ㄲ", "ㄱ"}, {"ㅋ", "ㄱ"},
    {"ㄷ", "ㄷ"}, {"ㅅ", "ㄷ"}, {"ㅆ", "ㄷ"}, {"ㅈ", "ㄷ"}, {"ㅊ", "ㄷ"}, {"ㅌ", "ㄷ"}, {"ㅎ", "ㄷ"},
    {"ㅂ", "ㅂ"}, {"ㅍ", "ㅂ"}
};

PhonologyKr::PhonologyKr() {}
PhonologyKr::~PhonologyKr() {}

bool PhonologyKr::init(const std::string& weights_dir) {
    idiom_exceptions_ = {
        {"\uACBD\uAE30\uB3C4\uC758\uD68C", "\uACBD\uAE30\uB3C4\uC5D0\uD68C"},
        {"\uC758\uACAC\uB780", "\uC758\uACAC\uB09C"},
        {"\uC784\uC9C4\uB780", "\uC784\uC9C4\uB09C"},
        {"\uC0DD\uC0B0\uB7C9", "\uC0DD\uC0B0\uB0E5"},
        {"\uACB0\uB2E8\uB825", "\uACB0\uB534\uB141"},
        {"\uACF5\uAD8C\uB825", "\uACF5\uAFD8\uB141"},
        {"\uB3D9\uC6D0\uB839", "\uB3D9\uC6D0\uB155"},
        {"\uC0C1\uACAC\uB840", "\uC0C1\uACAC\uB15C"},
        {"\uD6A1\uB2E8\uB85C", "\uD6A1\uB2E8\uB178"},
        {"\uC774\uC6D0\uB860", "\uC774\uC6D0\uB17C"},
        {"\uC785\uC6D0\uB8CC", "\uC774\uBDA0\uB1E8"},
        {"\uAD6C\uADFC\uB958", "\uAD6C\uADFC\uB274"},
        {"\uAC08\uB4F1", "\uAC08\uB73D"},
        {"\uBC1C\uB3D9", "\uBC1C\uB625"},
        {"\uC808\uB3C4", "\uC808\uB610"},
        {"\uB9D0\uC0B4", "\uB9D0\uC300"},
        {"\uBD88\uC18C", "\uBD88\uC3D8"},
        {"\uC77C\uC2DC", "\uC77C\uC528"},
        {"\uAC08\uC99D", "\uAC08\uCBE9"},
        {"\uBB3C\uC9C8", "\uBB3C\uCC14"},
        {"\uBC1C\uC804", "\uBC1C\uCA50"},
        {"\uBAB0\uC0C1\uC2DD", "\uBAB0\uC30D\uC2DD"},
        {"\uBD88\uC138\uCD9C", "\uBD88\uC384\uCD9C"},
        {"\uBB38\uACE0\uB9AC", "\uBB38\uAF2C\uB9AC"},
        {"\uB208\uB3D9\uC790", "\uB208\uB625\uC790"},
        {"\uC2E0\uBC14\uB78C", "\uC2E0\uBE60\uB78C"},
        {"\uC0B0\uC0C8", "\uC0B0\uC314"},
        {"\uC190\uC7AC\uC8FC", "\uC190\uC9F8\uC8FC"},
        {"\uAE38\uAC00", "\uAE38\uAE4C"},
        {"\uBB3C\uB3D9\uC774", "\uBB3C\uB625\uC774"},
        {"\uBC1C\uBC14\uB2E5", "\uBC1C\uBE60\uB2E5"},
        {"\uAD74\uC18D", "\uAD74\uC3D9"},
        {"\uC220\uC794", "\uC220\uC9E0"},
        {"\uBC1C\uB78C\uACB0", "\uBC1C\uB78C\uAEFC"},
        {"\uADF8\uBBD0\uB2EC", "\uADF8\uBBD0\uB538"},
        {"\uC544\uCE68\uBC25", "\uC544\uCE68\uBE71"},
        {"\uC7A0\uC790\uB9AC", "\uC7A0\uC9DC\uB9AC"},
        {"\uAC15\uAC00", "\uAC15\uAE4C"},
        {"\uCD08\uC2B9\uB2EC", "\uCD08\uC2B9\uB538"},
        {"\uB4F1\uBD88", "\uB4F1\uBFD4"},
        {"\uCC3D\uC0B4", "\uCC3D\uC300"},
        {"\uAC15\uC904\uAE30", "\uAC15\uCB50\uAE30"},
        {"\uC19C\uC774\uBD88", "\uC19C\uB2C8\uBD88"},
        {"\uD651\uC774\uBD88", "\uD63C\uB2C8\uBD88"},
        {"\uB9C9\uC77C", "\uB9DD\uB2D0"},
        {"\uC0AF\uC77C", "\uC0C1\uB2D0"},
        {"\uB9E8\uC785", "\uB9E8\uB2D9"},
        {"\uAF43\uC78E", "\uAF30\uB2D9"},
        {"\uB0B4\uBCF5\uC57D", "\uB0B4\uBD09\uB0D1"},
        {"\uC0C9\uC5F0\uD544", "\uC0DD\uB144\uD544"},
        {"\uC9C1\uD589\uC5F4\uCC28", "\uC9C0\uCEA5\uB148\uCC28"},
        {"\uB291\uB9C9\uC5FC", "\uB2A5\uB9DD\uB150"},
        {"\uCF69\uC5FF", "\uCF69\uB147"},
        {"\uB2F4\uC694", "\uB2F4\uB1E8"},
        {"\uB208\uC694\uAE30", "\uB208\uB1E8\uAE30"},
        {"\uC601\uC5C5\uC6A9", "\uC601\uC5C4\uB1FD"},
        {"\uC2DD\uC6A9\uC720", "\uC2DC\uAD65\uB274"},
        {"\uAD6D\uBBFC\uC724\uB9AC", "\uAD81\uBBFC\uB27C\uB9AC"},
        {"\uBC24\uC733", "\uBC24\uB27B"},
        {"\uC774\uC8FD\uC774\uC8FD", "\uC774\uC911\uB2C8\uC8FD"},
        {"\uC57C\uAE08\uC57C\uAE08", "\uC57C\uAE08\uB0D0\uAE08"},
        {"\uAC80\uC5F4", "\uAC80\uB148"},
        {"\uC69C\uB791\uC69C\uB791", "\uC69C\uB791\uB1F0\uB791"},
        {"\uAE08\uC735", "\uAE08\uB289"},
        {"\uB4E4\uC77C", "\uB4E4\uB9B4"},
        {"\uC194\uC78E", "\uC194\uB9BD"},
        {"\uC124\uC775\uB2E4", "\uC124\uB9AD\uB530"},
        {"\uBB3C\uC57D", "\uBB3C\uB7B5"},
        {"\uBD88\uC5EC\uC6B0", "\uBD88\uB824\uC6B0"},
        {"\uC11C\uC6B8\uC5ED", "\uC11C\uC6B8\uB825"},
        {"\uBB3C\uC5FF", "\uBB3C\uB82B"},
        {"\uD718\uBC1C\uC720", "\uD718\uBC1C\uB958"},
        {"\uC720\uB4E4\uC720\uB4E4", "\uC720\uB4E4\uB958\uB4E4"},
        {"\uD55C\uC77C", "\uD55C\uB2D0"},
        {"\uC637\uC785\uB2E4", "\uC628\uB2D9\uB530"},
        {"\uC11C\uB978\uC5EC\uC12F", "\uC11C\uB978\uB140\uC123"},
        {"\uBA39\uC740\uC5FF", "\uBA38\uADFC\uB147"},
        {"\uD560\uC77C", "\uD560\uB9B4"},
        {"\uC798\uC785\uB2E4", "\uC798\uB9BD\uB530"},
        {"\uC2A4\uBB3C\uC5EC\uC12F", "\uC2A4\uBB3C\uB824\uC123"},
        {"\uBA39\uC744\uC5FF", "\uBA38\uAE00\uB82B"},
        {"\uC1A1\uBCC4\uC5F0", "\uC1A1\uBCBC\uB828"},
        {"\uB0C7\uAC00", "\uB0B4\uAE4C"},
        {"\uC0DB\uAE38", "\uC0C8\uB084"},
        {"\uBE68\uB7AB\uB3CC", "\uBE68\uB798\uB618"},
        {"\uCF67\uB4F1", "\uCF54\uB73D"},
        {"\uAE43\uBC1C", "\uAE30\uBE68"},
        {"\uB300\uD33B\uBC25", "\uB300\uD328\uBE71"},
        {"\uD587\uC0B4", "\uD574\uC300"},
        {"\uBC43\uC18D", "\uBC30\uC3D9"},
        {"\uBC43\uC804", "\uBC30\uCA50"},
        {"\uACE0\uAC2F\uC9D3", "\uACE0\uAC1C\uCC13"},
        {"\uCF67\uB0A0", "\uCF58\uB0A0"},
        {"\uC544\uB7AB\uB2C8", "\uC544\uB79C\uB2C8"},
        {"\uD207\uB9C8\uB8E8", "\uD1F8\uB9C8\uB8E8"},
        {"\uBC43\uBA38\uB9AC", "\uBC34\uBA38\uB9AC"},
        {"\uBCA0\uAC2F\uC787", "\uBCA0\uAC20\uB2CF"},
        {"\uAE7B\uC78E", "\uAE6C\uB2D9"},
        {"\uB098\uBB47\uC78E", "\uB098\uBB38\uB2D9"},
        {"\uB3C4\uB9AC\uAE7B\uC5F4", "\uB3C4\uB9AC\uAE6C\uB148"},
        {"\uB4B7\uC737", "\uB4A8\uB27B"},
        {"\uD560\uAC78", "\uD560\uAEC4"},
        {"\uD560\uBC16\uC5D0", "\uD560\uBE60\uAED8"},
        {"\uD560\uC138\uB77C", "\uD560\uC384\uB77C"},
        {"\uD560\uC218\uB85D", "\uD560\uC464\uB85D"},
        {"\uD560\uC9C0\uB77C\uB3C4", "\uD560\uCC0C\uB77C\uB3C4"},
        {"\uD560\uC9C0\uC5B8\uC915", "\uD560\uCC0C\uC5B8\uC915"},
        {"\uD560\uC9C4\uB300", "\uD560\uCC10\uB300"}
    };
    sorted_idioms_.assign(idiom_exceptions_.begin(), idiom_exceptions_.end());
    std::sort(sorted_idioms_.begin(), sorted_idioms_.end(), 
              [](const std::pair<std::string, std::string>& a, 
                 const std::pair<std::string, std::string>& b) {
                    return a.first.size() > b.first.size();
                });

    // Load dict_loanwords.json
    {
        std::ifstream ifs(weights_dir + "/ko/dict_loanwords.json", std::ios::binary);
        if (ifs.is_open()) {
            try {
                nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
                if (!j.is_discarded() && j.is_object()) {
                    for (auto& [key, val] : j.items()) {
                        loanwords_set_.insert(key);
                    }
                }
            } catch (...) {}
        }
    }

    return true;
}



bool PhonologyKr::is_loanword_span(const std::vector<HangulTuple>& tuples, int start_idx, int nxt_idx) {
    if (loanwords_set_.empty()) {
        return false;
    }
    
    int start = start_idx;
    while (start > 0 && tuples[start - 1].type == 'H') {
        start--;
    }
    
    int end = nxt_idx;
    while (end < static_cast<int>(tuples.size()) - 1 && tuples[end + 1].type == 'H') {
        end++;
    }
    
    std::string word;
    for (int k = start; k <= end; ++k) {
        const auto& item = tuples[k];
        if (item.type == 'H') {
            int cho_idx = get_cho_idx(item.cho);
            int jung_idx = get_jung_idx(item.jung);
            int jong_idx = get_jong_idx(item.jong);
            if (cho_idx != -1 && jung_idx != -1 && jong_idx != -1) {
                uint32_t cp = 0xAC00 + cho_idx * 588 + jung_idx * 28 + jong_idx;
                word += codepoint_to_utf8(cp);
            }
        }
    }
    
    return loanwords_set_.find(word) != loanwords_set_.end();
}

std::string PhonologyKr::apply_jamo_names(const std::string& text) {
    auto cps = utf8_to_codepoints(text);
    std::string res;
    for (auto cp : cps) {
        if (cp >= 0x3131 && cp <= 0x314E) {
            auto it = JAMO_NAMES.find(cp);
            if (it != JAMO_NAMES.end()) {
                res += it->second;
                continue;
            }
        }
        res += codepoint_to_utf8(cp);
    }
    return res;
}

std::string PhonologyKr::apply_idiom_exceptions(const std::string& text) {
    std::string res = text;
    for (const auto& [src, dst] : sorted_idioms_) {
        res = replace_all(res, src, dst);
    }
    return res;
}

std::vector<CharMeta> PhonologyKr::build_char_meta(
    const std::string& text, 
    const std::vector<uint32_t>& codepoints,
    const std::vector<std::pair<size_t, size_t>>& cp_byte_offsets,
    const SnapResult& result) 
{
    std::vector<CharMeta> meta(codepoints.size());
    
    // Process annotations
    for (const auto& ann : result.annotations) {
        int start = std::get<0>(ann);
        int end = std::get<1>(ann);
        std::string label = std::get<2>(ann);
        if (label != "TENS") continue;
        
        for (size_t i = 0; i < codepoints.size(); ++i) {
            size_t c_start = cp_byte_offsets[i].first;
            size_t c_end = cp_byte_offsets[i].second;
            if (c_start >= (size_t)start && c_end <= (size_t)end) {
                meta[i].tens = true;
            }
        }
    }
    
    // Process morphemes
    for (const auto& m : result.morphemes) {
        int start = m.start;
        int end = m.end;
        std::string pos = m.pos;
        
        if (start >= 0 && end <= (int)codepoints.size()) {
            for (int i = start; i < end; ++i) {
                meta[i].pos = pos;
            }
            if (start < (int)codepoints.size()) {
                meta[start].morph_boundary = true;
            }
        }
    }
    
    return meta;
}

std::vector<HangulTuple> PhonologyKr::text_to_tuples(
    const std::string& text, 
    const std::vector<uint32_t>& codepoints,
    const std::vector<std::pair<size_t, size_t>>& cp_byte_offsets,
    const std::vector<CharMeta>& char_meta) 
{
    std::vector<HangulTuple> tuples;
    tuples.reserve(codepoints.size());
    
    for (size_t i = 0; i < codepoints.size(); ++i) {
        uint32_t cp = codepoints[i];
        HangulTuple t;
        t.original_cp = cp;
        t.byte_start = cp_byte_offsets[i].first;
        t.byte_end = cp_byte_offsets[i].second;
        if (i < char_meta.size()) {
            t.meta = char_meta[i];
        }
        
        if (cp >= 0xAC00 && cp <= 0xD7A3) {
            t.type = 'H';
            uint32_t base = cp - 0xAC00;
            t.cho = CHOSUNGS[base / 588];
            t.jung = JUNGSUNGS[(base % 588) / 28];
            t.jong = JONGSUNGS[base % 28];
        } else {
            t.type = 'O';
            t.cho = codepoint_to_utf8(cp);
            t.jung = "";
            t.jong = "";
        }
        tuples.push_back(t);
    }
    return tuples;
}

void PhonologyKr::apply_josa_ui(std::vector<HangulTuple>& tuples) {
    for (size_t i = 0; i < tuples.size(); ++i) {
        auto& item = tuples[i];
        if (item.type != 'H') continue;
        if (!(item.cho == "ㅇ" && item.jung == "ㅢ" && item.jong == "" && item.meta.pos == "JKG")) {
            continue;
        }
        
        std::string prev_pos = "";
        for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
            if (tuples[j].type == 'H') {
                prev_pos = tuples[j].meta.pos;
                break;
            } else if (tuples[j].type == 'O' && tuples[j].cho != " ") {
                break;
            }
        }
        
        if (prev_pos == "JKB") continue;
        item.jung = "ㅔ";
    }
}

void PhonologyKr::apply_vowel_simplification(std::vector<HangulTuple>& tuples) {
    bool is_first_syllable = true;
    for (auto& item : tuples) {
        if (item.type != 'H') {
            if (item.type == 'O') {
                if (item.cho == " ") {
                    is_first_syllable = true;
                } else {
                    is_first_syllable = false;
                }
            }
            continue;
        }
        
        if ((item.cho == "ㅈ" || item.cho == "ㅉ" || item.cho == "ㅊ") && item.jung == "ㅕ") {
            item.jung = "ㅓ";
        } else if (!is_first_syllable && item.cho != "ㅇ" && item.jung == "ㅢ") {
            item.jung = "ㅣ";
        } else if (!is_first_syllable && item.cho == "ㅇ" && item.jung == "ㅢ" && item.jong == "") {
            item.jung = "ㅣ";
        }
        is_first_syllable = false;
    }
}

void PhonologyKr::apply_n_addition(std::vector<HangulTuple>& tuples) {
    // Standard rule 29: Handled in idiom exceptions.
    // Kept for structural alignment.
}

void PhonologyKr::apply_aspiration_and_h_drop(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        std::string jung = nxt.jung;
        
        // ㄷ + 히 -> 치
        if (jong == "ㄷ" && cho == "ㅎ" && PALATAL_JUNG_MAP.find(jung) != PALATAL_JUNG_MAP.end()) {
            bool is_hyeong = (jung == "ㅕ" && nxt.jong == "ㅇ");
            if (!is_hyeong) {
                curr.jong = "";
                nxt.cho = "ㅊ";
                nxt.jung = PALATAL_JUNG_MAP.at(jung);
                continue;
            }
        }
        
        // A: 받침 + ㅎ -> 격음화
        if (cho == "ㅎ") {
            if (jong == "ㄱ" || jong == "ㄺ") {
                curr.jong = (jong == "ㄺ") ? "ㄹ" : "";
                nxt.cho = "ㅋ";
            } else if (jong == "ㄷ" || jong == "ㅅ" || jong == "ㅆ" || jong == "ㅊ" || jong == "ㅌ") {
                curr.jong = "";
                nxt.cho = "ㅌ";
            } else if (jong == "ㅂ" || jong == "ㄼ") {
                curr.jong = (jong == "ㄼ") ? "ㄹ" : "";
                nxt.cho = "ㅍ";
            } else if (jong == "ㅈ" || jong == "ㄵ") {
                curr.jong = (jong == "ㄵ") ? "ㄴ" : "";
                nxt.cho = "ㅊ";
            }
        }
        // B: ㅎ 계열 받침 + 평음 -> 격음화
        else if ((jong == "ㅎ" || jong == "ㄶ" || jong == "ㅀ") && (cho == "ㄱ" || cho == "ㄷ" || cho == "ㅈ")) {
            std::string rem = jong;
            if (jong == "ㅎ") rem = "";
            else if (jong == "ㄶ") rem = "ㄴ";
            else if (jong == "ㅀ") rem = "ㄹ";
            
            std::string target_cho = cho;
            if (cho == "ㄱ") target_cho = "ㅋ";
            else if (cho == "ㄷ") target_cho = "ㅌ";
            else if (cho == "ㅈ") target_cho = "ㅊ";
            
            curr.jong = rem;
            nxt.cho = target_cho;
        }
        // C: ㅎ 계열 받침 + ㅇ -> ㅎ탈락
        else if ((jong == "ㅎ" || jong == "ㄶ" || jong == "ㅀ") && cho == "ㅇ") {
            std::string rem = jong;
            if (jong == "ㅎ") rem = "";
            else if (jong == "ㄶ") rem = "ㄴ";
            else if (jong == "ㅀ") rem = "ㄹ";
            curr.jong = rem;
        }
        // D: ㅎ 계열 받침 + ㄴ
        else if ((jong == "ㅎ" || jong == "ㄶ" || jong == "ㅀ") && cho == "ㄴ") {
            std::string rem = jong;
            if (jong == "ㅎ") rem = "ㄴ";
            else if (jong == "ㄶ") rem = "ㄴ";
            else if (jong == "ㅀ") rem = "ㄹ";
            curr.jong = rem;
        }
        // E: ㅎ + ㅅ -> ㅆ
        else if (jong == "ㅎ" && cho == "ㅅ") {
            curr.jong = "";
            nxt.cho = "ㅆ";
        }
    }
}

void PhonologyKr::apply_palatalization(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        std::string jung = nxt.jung;
        
        if ((jong == "ㄷ" || jong == "ㅌ") && cho == "ㅇ" && PALATAL_JUNG_MAP.find(jung) != PALATAL_JUNG_MAP.end()) {
            std::string target_cho = (jong == "ㄷ") ? "ㅈ" : "ㅊ";
            curr.jong = "";
            nxt.cho = target_cho;
            nxt.jung = PALATAL_JUNG_MAP.at(jung);
        }
    }
}

void PhonologyKr::apply_tensification(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        bool has_space = false;
        for (int k = i + 1; k < nxt_idx; ++k) {
            if (tuples[k].type == 'O' && tuples[k].cho == " ") {
                has_space = true;
                break;
            }
        }
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        
        // 23항: 폐쇄음 받침 + 평음 -> 경음화
        if (TENSE_TRIGGER_JONGS.find(jong) != TENSE_TRIGGER_JONGS.end() && TENSE_MAP.find(cho) != TENSE_MAP.end()) {
            if (!has_space) {
                nxt.cho = TENSE_MAP.at(cho);
            }
        }
        // 비음 뒤 경음화 확장
        else if ((jong == "ㄴ" || jong == "ㅁ") && TENSE_MAP.find(cho) != TENSE_MAP.end()) {
            if (curr.meta.tens || nxt.meta.tens) {
                nxt.cho = TENSE_MAP.at(cho);
            }
        }
        // ㄹ 받침 + 평음 경음화
        else if (jong == "ㄹ" && TENSE_MAP.find(cho) != TENSE_MAP.end()) {
            if ((cho == "ㄷ" || cho == "ㅅ" || cho == "ㅈ") && !has_space) {
                bool is_verb = (curr.meta.pos == "VV" || curr.meta.pos == "VA" || curr.meta.pos == "VX");
                if (curr.meta.pos.empty()) {
                    if (nxt.type == 'H') {
                        int cho_idx = get_cho_idx(nxt.cho);
                        int jung_idx = get_jung_idx(nxt.jung);
                        int jong_idx = get_jong_idx(nxt.jong);
                        if (cho_idx != -1 && jung_idx != -1 && jong_idx != -1) {
                            uint32_t cp = 0xAC00 + cho_idx * 588 + jung_idx * 28 + jong_idx;
                            std::string _nxt_char = codepoint_to_utf8(cp);
                            static const std::unordered_set<std::string> verb_endings = {
                                "다", "고", "지", "면", "며", "게", "아", "어", "은", "을", "는", "니", "자", "서"
                            };
                            if (verb_endings.find(_nxt_char) != verb_endings.end()) {
                                is_verb = true;
                            }
                        }
                    }
                }
                bool is_particle_or_ending = !nxt.meta.pos.empty() && (nxt.meta.pos[0] == 'J' || nxt.meta.pos[0] == 'E');
                bool is_foreign = (curr.meta.pos == "SL" || nxt.meta.pos == "SL" || is_loanword_span(tuples, i, nxt_idx));
                if (!is_verb && !is_foreign && !is_particle_or_ending) {
                    nxt.cho = TENSE_MAP.at(cho);
                }
            } else if (curr.meta.pos == "ETM") {
                nxt.cho = TENSE_MAP.at(cho);
            }
        }
        // 25항: 용언 ㄼ,ㄾ + 평음 -> 경음화
        else if ((jong == "ㄼ" || jong == "ㄾ") && TENSE_MAP.find(cho) != TENSE_MAP.end()) {
            if (curr.meta.pos == "VV" || curr.meta.pos == "VA" || curr.meta.pos == "VX") {
                nxt.cho = TENSE_MAP.at(cho);
            }
        }
        
        // annotation 기반 직접 경음화
        if (nxt.meta.tens && TENSE_MAP.find(cho) != TENSE_MAP.end()) {
            std::string t_cho = TENSE_MAP.at(cho);
            if (nxt.cho != t_cho) {
                nxt.cho = t_cho;
            }
        }
    }
}

void PhonologyKr::apply_liaison(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        
        if (jong != "" && cho == "ㅇ") {
            if (jong == "ㅇ") continue;
            
            bool has_space = false;
            for (int k = i + 1; k < nxt_idx; ++k) {
                if (tuples[k].type == 'O' && tuples[k].cho == " ") {
                    has_space = true;
                    break;
                }
            }
            if (has_space) continue;
            
            bool is_substantive = nxt.meta.morph_boundary && (SUBSTANTIVE_POS.find(nxt.meta.pos) != SUBSTANTIVE_POS.end());

            // 서술격조사 이다 heuristic: NNG/NNB로 오분류된 '이(ㅣ)' 계열 처리
            // 예: 것(NNG)+입니다(NNG) → 실제로는 것(NNB)+이다(VCP) 구조
            // '이' 음절(초성ㅇ+중성ㅣ)이 NNG/NNP/NNB로 분류됐웈을 때 연음 허용
            if (is_substantive) {
                const std::string& nxt_pos = nxt.meta.pos;
                if ((nxt_pos == "NNG" || nxt_pos == "NNP" || nxt_pos == "NNB") && nxt.jung == "ㅣ") {
                    is_substantive = false;
                }
            }

            // 용언+어미 오분류 heuristic: 앞 형태소가 용언/어미류인데 뒤가 NNG로 오분류된 경우
            // 예: 같(VA)+은(NNG) → 실제 같(VA)+은(ETM) 구조 → '가튜'이 되어야 함
            // 앞 형태소 POS가 용언/어미 계열이면 뒤 NNG도 어미류로 간주하여 연음 허용
            if (is_substantive) {
                const std::string& nxt_pos = nxt.meta.pos;
                if (nxt_pos == "NNG" || nxt_pos == "NNP" || nxt_pos == "NNB") {
                    if (VERBAL_POS.find(curr.meta.pos) != VERBAL_POS.end()) {
                        is_substantive = false;
                    }
                }
            }

            if (is_substantive) {
                std::string temp_jong = jong;
                if (SIMPL_MAP.find(jong) != SIMPL_MAP.end()) temp_jong = SIMPL_MAP.at(jong);
                std::string neut_jong = temp_jong;
                if (NEUT_MAP.find(temp_jong) != NEUT_MAP.end()) neut_jong = NEUT_MAP.at(temp_jong);
                if (neut_jong != "") {
                    curr.jong = "";
                    nxt.cho = neut_jong;
                }
                continue;
            }
            
            if (DOUBLE_JONG_LIAISON.find(jong) != DOUBLE_JONG_LIAISON.end()) {
                auto p = DOUBLE_JONG_LIAISON.at(jong);
                curr.jong = p.first;
                nxt.cho = p.second;
            } else {
                curr.jong = "";
                nxt.cho = jong;
            }
        }
    }
}

void PhonologyKr::apply_neutralization(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        std::string jong = curr.jong;
        if (jong == "") continue;
        
        // 11.1항: ㄺ 발음
        if (jong == "ㄺ") {
            int nxt_idx = get_next_hangul_idx(tuples, i);
            bool is_followed_by_g = false;
            if (nxt_idx != -1 && tuples[nxt_idx].type == 'H' && (tuples[nxt_idx].cho == "ㄱ" || tuples[nxt_idx].cho == "ㄲ")) {
                is_followed_by_g = true;
            }
            
            bool is_verb = (curr.meta.pos == "VV" || curr.meta.pos == "VA" || curr.meta.pos == "VX");
            if (curr.meta.pos.empty()) {
                bool is_noun_exception = (curr.cho == "ㄷ" && curr.jung == "ㅏ") || (curr.cho == "ㅎ" && curr.jung == "ㅡ");
                is_verb = !is_noun_exception;
            }
            
            if (is_followed_by_g && is_verb) {
                curr.jong = "ㄹ";
                continue;
            } else {
                curr.jong = "ㄱ";
                continue;
            }
        }
        
        // 밟- 용언 휴리스틱
        if (jong == "ㄼ" && (curr.cho == "ㅂ" || curr.cho == "ㅃ") && curr.jung == "ㅏ") {
            int nxt_idx = get_next_hangul_idx(tuples, i);
            if (nxt_idx != -1 && tuples[nxt_idx].type == 'H' && tuples[nxt_idx].cho != "ㅇ") {
                curr.jong = "ㅂ";
                continue;
            }
        }
        
        // 넓- 용언 예외 휴리스틱
        if (jong == "ㄼ" && curr.cho == "ㄴ" && curr.jung == "ㅓ") {
            int nxt_idx = get_next_hangul_idx(tuples, i);
            if (nxt_idx != -1 && tuples[nxt_idx].type == 'H') {
                const auto& nxt = tuples[nxt_idx];
                bool is_neop_exception = false;
                if ((nxt.cho == "ㅈ" || nxt.cho == "ㅉ") && nxt.jung == "ㅜ") {
                    is_neop_exception = true;
                } else if ((nxt.cho == "ㅈ" || nxt.cho == "ㅉ") && nxt.jung == "ㅓ") {
                    is_neop_exception = true;
                } else if ((nxt.cho == "ㄷ" || nxt.cho == "ㄸ") && nxt.jung == "ㅜ") {
                    is_neop_exception = true;
                }
                
                if (is_neop_exception) {
                    curr.jong = "ㅂ";
                    continue;
                }
            }
        }
        
        std::string temp_jong = jong;
        if (SIMPL_MAP.find(jong) != SIMPL_MAP.end()) temp_jong = SIMPL_MAP.at(jong);
        std::string final_jong = temp_jong;
        if (NEUT_MAP.find(temp_jong) != NEUT_MAP.end()) final_jong = NEUT_MAP.at(temp_jong);
        curr.jong = final_jong;
    }
}

void PhonologyKr::apply_assimilation(std::vector<HangulTuple>& tuples) {
    int n = static_cast<int>(tuples.size());
    
    // 1. 초성 ㄹ -> ㄴ 변환 (종성이 ㄱ, ㄷ, ㅂ, ㅁ, ㅇ 일 때)
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        
        if ((jong == "ㄱ" || jong == "ㄷ" || jong == "ㅂ" || jong == "ㅁ" || jong == "ㅇ") && cho == "ㄹ") {
            nxt.cho = "ㄴ";
        }
    }
    
    // 2. 비음화 (종성 ㄱ, ㄷ, ㅂ + 초성 ㄴ, ㅁ)
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        
        if (jong == "ㄱ" && (cho == "ㄴ" || cho == "ㅁ")) {
            curr.jong = "ㅇ";
        } else if (jong == "ㄷ" && (cho == "ㄴ" || cho == "ㅁ")) {
            curr.jong = "ㄴ";
        } else if (jong == "ㅂ" && (cho == "ㄴ" || cho == "ㅁ")) {
            curr.jong = "ㅁ";
        }
    }
    
    // 3. 유음화
    for (int i = 0; i < n; ++i) {
        auto& curr = tuples[i];
        if (curr.type != 'H') continue;
        
        int nxt_idx = get_next_hangul_idx(tuples, i);
        if (nxt_idx == -1) continue;
        auto& nxt = tuples[nxt_idx];
        
        std::string jong = curr.jong;
        std::string cho = nxt.cho;
        
        if (jong == "ㄴ" && cho == "ㄹ") {
            curr.jong = "ㄹ";
        } else if (jong == "ㄹ" && cho == "ㄴ") {
            nxt.cho = "ㄹ";
        }
    }
}

std::string PhonologyKr::tuples_to_text(const std::vector<HangulTuple>& tuples) {
    std::string result = "";
    for (const auto& item : tuples) {
        if (item.type == 'H') {
            int cho_idx = get_cho_idx(item.cho);
            int jung_idx = get_jung_idx(item.jung);
            int jong_idx = get_jong_idx(item.jong);
            if (cho_idx == -1 || jung_idx == -1 || jong_idx == -1) {
                result += codepoint_to_utf8(item.original_cp);
                continue;
            }
            uint32_t cp = 0xAC00 + cho_idx * 588 + jung_idx * 28 + jong_idx;
            result += codepoint_to_utf8(cp);
        } else {
            result += item.cho;
        }
    }
    return result;
}

int PhonologyKr::get_next_hangul_idx(const std::vector<HangulTuple>& tuples, int start_idx) {
    for (size_t j = start_idx + 1; j < tuples.size(); ++j) {
        if (tuples[j].type == 'H') {
            return static_cast<int>(j);
        } else if (tuples[j].type == 'O' && tuples[j].cho == " ") {
            continue;
        } else {
            break;
        }
    }
    return -1;
}

std::string PhonologyKr::apply_rules(const std::string& text, const SnapResult& result) {
    try {
        std::string processed_text = apply_jamo_names(text);
        processed_text = apply_idiom_exceptions(processed_text);
        
        std::vector<uint32_t> codepoints = utf8_to_codepoints(processed_text);
        std::vector<std::pair<size_t, size_t>> cp_byte_offsets;
        cp_byte_offsets.reserve(codepoints.size());
        size_t current_byte = 0;
        for (auto cp : codepoints) {
            size_t len = codepoint_to_utf8(cp).size();
            cp_byte_offsets.push_back({current_byte, current_byte + len});
            current_byte += len;
        }
        
        std::vector<CharMeta> char_meta = build_char_meta(processed_text, codepoints, cp_byte_offsets, result);
        std::vector<HangulTuple> tuples = text_to_tuples(processed_text, codepoints, cp_byte_offsets, char_meta);
        
        apply_josa_ui(tuples);
        apply_vowel_simplification(tuples);
        apply_n_addition(tuples);
        apply_aspiration_and_h_drop(tuples);
        apply_palatalization(tuples);
        apply_tensification(tuples);
        apply_liaison(tuples);
        apply_neutralization(tuples);
        apply_assimilation(tuples);
        
        return tuples_to_text(tuples);
    } catch (...) {
        return text;
    }
}

} // namespace snap
