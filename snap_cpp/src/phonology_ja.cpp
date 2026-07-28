#include "snap/phonology_ja.h"
#include "snap/classifier.h"
#include <cstdint>
#include <fstream>
#include <regex>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace snap {

// UTF-8 Helper Functions (Local to translation unit)
static std::vector<uint32_t> utf8_to_codepoints_ja(const std::string& str) {
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

static std::string codepoint_to_utf8_ja(uint32_t cp) {
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

static std::string codepoints_to_utf8_ja(const std::vector<uint32_t>& cps) {
    std::string res;
    for (auto cp : cps) {
        res += codepoint_to_utf8_ja(cp);
    }
    return res;
}

// Helper to replace all occurrences of a substring
static std::string replace_all_ja(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

static std::wstring utf8_to_wstring(const std::string& str) {
    auto cps = utf8_to_codepoints_ja(str);
    std::wstring wstr;
    wstr.reserve(cps.size());
    for (auto cp : cps) {
        if (cp <= 0xFFFF) {
            wstr.push_back(static_cast<wchar_t>(cp));
        } else {
            wstr.push_back(static_cast<wchar_t>(0xD800 + ((cp - 0x10000) >> 10)));
            wstr.push_back(static_cast<wchar_t>(0xDC00 + ((cp - 0x10000) & 0x3FF)));
        }
    }
    return wstr;
}

static std::string wstring_to_utf8(const std::wstring& wstr) {
    std::vector<uint32_t> cps;
    cps.reserve(wstr.size());
    for (size_t i = 0; i < wstr.size(); ++i) {
        uint32_t cp = wstr[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wstr.size()) {
            uint32_t next = wstr[i + 1];
            if (next >= 0xDC00 && next <= 0xDFFF) {
                cp = 0x10000 + (((cp - 0xD800) << 10) | (next - 0xDC00));
                i++;
            }
        }
        cps.push_back(cp);
    }
    return codepoints_to_utf8_ja(cps);
}

// Regex replace with lambda/callback helper (wstring-based to prevent encoding offset bugs in Windows std::regex)
template<typename F>
static std::string regex_replace_callback(const std::string& text, const std::string& pattern, F callback) {
    std::wstring wtext = utf8_to_wstring(text);
    std::wstring wpattern = utf8_to_wstring(pattern);

    std::wregex re;
    re.imbue(std::locale::classic());
    try {
        re.assign(wpattern);
    } catch (...) {
        return text;
    }

    std::regex raw_re;
    raw_re.imbue(std::locale::classic());
    try {
        raw_re.assign(pattern);
    } catch (...) {
        return text;
    }

    std::wstring wresult;
    auto it = std::wsregex_iterator(wtext.begin(), wtext.end(), re);
    auto end = std::wsregex_iterator();
    size_t last_pos = 0;
    for (; it != end; ++it) {
        wresult.append(wtext.substr(last_pos, it->position() - last_pos));
        std::string match_str = wstring_to_utf8(it->str());
        std::smatch m;
        if (std::regex_search(match_str, m, raw_re)) {
            wresult.append(utf8_to_wstring(callback(m)));
        } else {
            wresult.append(it->str());
        }
        last_pos = it->position() + it->length();
    }
    wresult.append(wtext.substr(last_pos));
    return wstring_to_utf8(wresult);
}

// ── パッチ辞書保管用 ───────────────────────────────────────────────────────────────
static const std::vector<std::string> _KATA_DIGITS = {"", "イチ", "ニ", "サン", "ヨン", "ゴ", "ロク", "ナナ", "ハチ", "キュウ"};
static const std::vector<std::string> _KATA_UNITS = {"", "ジュウ", "ヒャク", "セン"};
static const std::vector<std::string> _KATA_LARGE = {"", "マン", "オク", "チョウ"};

static const std::unordered_map<int, std::string> _DAYS_MAP = {
    {1, "ツィタチ"}, {2, "フツカ"}, {3, "ミッカ"}, {4, "ヨッカ"}, {5, "イツカ"},
    {6, "ムイカ"}, {7, "ナノカ"}, {8, "ヨウカ"}, {9, "ココノカ"}, {10, "トオカ"},
    {11, "ジュウイチニチ"}, {12, "ジュウニニチ"}, {13, "ジュウサンニチ"}, {14, "ジュウヨッカ"},
    {15, "ジュウゴニチ"}, {16, "ジュウロクニチ"}, {17, "ジュウナナニチ"}, {18, "ジュウハチニチ"},
    {19, "ジュウキュウニチ"}, {20, "ハツカ"}, {21, "ニジュウイチニチ"}, {22, "ニジュウニニチ"},
    {23, "ニジュウサンニチ"}, {24, "ニジュウヨッカ"}, {25, "ニジュウゴニチ"}, {26, "ニジュウロクニチ"},
    {27, "ニジュウナナニチ"}, {28, "ニジュウハチニチ"}, {29, "ニジュウキュウニチ"}, {30, "サンジュウニチ"},
    {31, "サンジュウイチニチ"}
};

static const std::unordered_map<int, std::string> _MONTHS_MAP = {
    {1, "イチガツ"}, {2, "ニガツ"}, {3, "サンガツ"}, {4, "シガツ"}, {5, "ゴガツ"},
    {6, "ロクガツ"}, {7, "シチガツ"}, {8, "ハチガツ"}, {9, "クガツ"}, {10, "ジュウガツ"},
    {11, "ジュウイチガツ"}, {12, "ジュウニガツ"}
};

static const std::unordered_map<int, std::string> _HOURS_MAP = {
    {1, "イチジ"}, {2, "ニジ"}, {3, "サンジ"}, {4, "ヨジ"}, {5, "ゴジ"},
    {6, "ロクジ"}, {7, "シチジ"}, {8, "ハチジ"}, {9, "クジ"}, {10, "ジュウジ"},
    {11, "ジュウイチジ"}, {12, "ジュウニジ"}, {13, "ジュウサンジ"}, {14, "ジュウヨジ"},
    {15, "ジュウゴジ"}, {16, "ジュウロクジ"}, {17, "ジュウシチジ"}, {18, "ジュウハチジ"},
    {19, "ジュウクジ"}, {20, "ニジュウジ"}, {21, "ニジュウイチジ"}, {22, "ニジュウニジ"},
    {23, "ニジュウサンジ"}, {24, "ニジュウヨジ"}
};

static const std::unordered_map<std::string, std::string> _CURRENCY_MAP = {
    {"$", "ドル"}, {"¥", "円"}, {"£", "ポンド"}, {"€", "ユーロ"}
};

static const std::vector<std::string> _DIGITS_JA = {"", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
static const std::vector<std::string> _UNITS_JA = {"", "十", "百", "千"};
static const std::vector<std::string> _LARGE_JA = {"", "万", "億", "兆"};

static const std::unordered_map<std::string, std::string> _ALPHASYMBOL_YOMI = {
    {"#", "シャープ"}, {"%", "パーセント"}, {"&", "アンド"}, {"+", "プラス"},
    {"-", "マイナス"}, {":", "コロン"}, {";", "セミコロン"},
    {"<", "小なり"}, {"=", "イコール"}, {">", "大なり"}, {"@", "アット"},
    {"a", "エー"}, {"b", "ビー"}, {"c", "シー"}, {"d", "ディー"}, {"e", "イー"},
    {"f", "エフ"}, {"g", "ジー"}, {"h", "エイチ"}, {"i", "アイ"}, {"j", "ジェー"},
    {"k", "ケー"}, {"l", "エル"}, {"m", "エム"}, {"n", "エヌ"}, {"o", "オー"},
    {"p", "ピー"}, {"q", "キュー"}, {"r", "アール"}, {"s", "エス"}, {"t", "ティー"},
    {"u", "ユー"}, {"v", "ブイ"}, {"w", "ダブリュー"}, {"x", "エックス"},
    {"y", "ワイ"}, {"z", "ゼット"},
    {"α", "アルファ"}, {"β", "ベータ"}, {"γ", "ガンマ"}, {"δ", "デルタ"},
    {"ε", "イプシロン"}, {"ζ", "ゼータ"}, {"η", "イータ"}, {"θ", "シータ"},
    {"ι", "イオタ"}, {"κ", "カッパ"}, {"λ", "ラムダ"}, {"μ", "ミュー"},
    {"ν", "ニュー"}, {"ξ", "クサイ"}, {"ο", "オミクロン"}, {"π", "パイ"},
    {"ρ", "ロー"}, {"σ", "シグマ"}, {"τ", "タウ"}, {"υ", "ウプシロン"},
    {"φ", "ファイ"}, {"χ", "カイ"}, {"ψ", "プサイ"}, {"ω", "オメガ"},
    {"Α", "アルファ"}, {"Β", "ベータ"}, {"Γ", "ガンマ"}, {"Δ", "デルタ"},
    {"Ｅ", "イプシロン"}, {"Ζ", "ゼータ"}, {"Η", "イータ"}, {"Θ", "シータ"},
    {"Ｉ", "イオタ"}, {"Ｋ", "カッパ"}, {"Λ", "ラムダ"}, {"Ｍ", "ミュー"},
    {"Ｎ", "ニュー"}, {"Ξ", "クサイ"}, {"Ｏ", "オミクロン"}, {"Π", "パイ"},
    {"Ρ", "ロー"}, {"Σ", "シグマ"}, {"Ｔ", "タウ"}, {"Υ", "ウプシロン"},
    {"Φ", "ファイ"}, {"Ｘ", "カイ"}, {"Ψ", "プサイ"}, {"Ω", "オメガ"}
};

static const std::unordered_map<std::string, std::string> _REP_MAP = {
    {"：", ","}, {"；", ","}, {"，", ","}, {"。", "."}, {"！", "!"},
    {"？", "?"}, {"\n", "."}, {"·", ","}, {"、", ","}, {"...", "…"}
};

// ── 카운터 및 연탁 룰 테이블 ───────────────────────────────────────────
static const std::unordered_map<std::string, std::unordered_map<std::string, std::pair<std::string, std::string>>> _COUNTER_RULES = {
    {"日", {
        {"1", {"ツイ", "タチ"}}, {"一", {"ツイ", "タチ"}},
        {"2", {"フツ", "カ"}}, {"二", {"フツ", "カ"}},
        {"3", {"ミッ", "カ"}}, {"三", {"ミッ", "カ"}},
        {"4", {"ヨッ", "カ"}}, {"四", {"ヨッ", "カ"}},
        {"5", {"イツ", "カ"}}, {"五", {"イツ", "カ"}},
        {"6", {"ムイ", "カ"}}, {"六", {"ムイ", "カ"}},
        {"7", {"ナノ", "カ"}}, {"七", {"ナノ", "カ"}},
        {"8", {"ヨ우", "カ"}}, {"八", {"ヨウ", "カ"}},
        {"9", {"ココノ", "カ"}}, {"九", {"ココノ", "カ"}},
        {"10", {"トオ", "カ"}}, {"十", {"トオ", "カ"}},
        {"14", {"ジュウヨッ", "カ"}}, {"十四", {"ジュウヨッ", "カ"}},
        {"20", {"ハツ", "カ"}}, {"二十", {"ハツ", "カ"}},
        {"24", {"ニジュウヨッ", "カ"}}, {"二十四", {"ニジュウヨッ", "カ"}}
    }},
    {"歳", {
        {"20", {"ハタ", "チ"}}, {"二十", {"ハタ", "チ"}}
    }}
};

static const std::unordered_map<std::string, std::string> _RENDAKU_RULES = {
    {"旅", "ダビ"},
    {"境", "ザカイ"},
    {"川", "ガワ"},
    {"河", "ガワ"},
    {"口", "グチ"},
    {"島", "ジマ"},
    {"花", "バナ"},
    {"箱", "バコ"}
};

// ── 패치 딕셔너리 보관용 ───────────────────────────────────────────────
static std::unordered_map<std::string, std::vector<std::string>> _DICT_PATCHES;
static std::unordered_map<std::string, std::string> _ENG_DICT;

PhonologyJa::PhonologyJa() {}
PhonologyJa::~PhonologyJa() {}

static nlohmann::json parse_json_utf8(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return nlohmann::json();
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return nlohmann::json::parse(str, nullptr, false);
}

// Helper to check if file exists
static bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

bool PhonologyJa::init(const std::string& weights_dir) {
    // SNAP_HOME 환경변수 기반 앵커 탐색
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

    std::string base_paths[] = {
        snap_home + "/models/ja/",
        snap_home + "/resources/",
        snap_home + "/weights/ja/",
        snap_home + "/ja/",
        snap_home + "/"
    };

    std::string kanji_path, accent_path, patches_path, eng_dict_path;
    for (const auto& bp : base_paths) {
        if (file_exists(bp + "ja_kanji_dict.json") && kanji_path.empty()) kanji_path = bp + "ja_kanji_dict.json";
        if (file_exists(bp + "ja_accent_dict.json") && accent_path.empty()) accent_path = bp + "ja_accent_dict.json";
        if (file_exists(bp + "ja_dict_patches.json") && patches_path.empty()) patches_path = bp + "ja_dict_patches.json";
        if (file_exists(bp + "custom_eng_dict_ja.json") && eng_dict_path.empty()) eng_dict_path = bp + "custom_eng_dict_ja.json";
    }

    if (kanji_path.empty() || accent_path.empty() || patches_path.empty()) {
        std::cerr << "[SNAP Strict Policy Error] 일본어 필수 음운 사전을 찾을 수 없습니다!\n"
                  << "  - SNAP_HOME 앵커: " << snap_home << "\n"
                  << "  - 탐색 경로: " << snap_home << "/models/ja/\n"
                  << "  - 필수 파일(ja_kanji_dict.json, ja_accent_dict.json, ja_dict_patches.json)이 없으므로 초기화를 중단합니다." << std::endl;
        return false;
    }

    // Load Kanji Dictionary
    try {
        auto j = parse_json_utf8(kanji_path);
        if (!j.is_discarded() && j.is_object()) {
            for (auto& [key, val] : j.items()) {
                if (val.is_array()) {
                    kanji_dict_[key] = val.get<std::vector<std::string>>();
                }
            }
        }
    } catch (...) { return false; }

    // Load Accent Dictionary
    try {
        auto j = parse_json_utf8(accent_path);
        if (!j.is_discarded() && j.is_object()) {
            for (auto& [key, val] : j.items()) {
                if (val.is_number_integer()) {
                    accent_dict_[key] = val.get<int>();
                }
            }
        }
    } catch (...) { return false; }

    // Load Dict Patches
    try {
        auto j = parse_json_utf8(patches_path);
        if (!j.is_discarded() && j.is_object()) {
            for (auto& [key, val] : j.items()) {
                if (val.is_array()) {
                    _DICT_PATCHES[key] = val.get<std::vector<std::string>>();
                }
            }
        }
    } catch (...) { return false; }

    // Inject core default 훈독 patches to resolve neural-free baseline differences
    // (Removed in favor of unified ja_dict_patches.json)

    // Load English-Japanese Dictionary
    if (!eng_dict_path.empty()) {
        try {
            auto j = parse_json_utf8(eng_dict_path);
            if (!j.is_discarded() && j.is_object()) {
                for (auto& [key, val] : j.items()) {
                    if (val.is_string()) {
                        _ENG_DICT[key] = val.get<std::string>();
                    }
                }
            }
        } catch (...) {}
    }

    // Load Targets Accent (from weights/ja/targets.json or weights/targets.json)
    std::string targets_path = weights_dir + "/ja/targets.json";
    if (!file_exists(targets_path)) {
        targets_path = weights_dir + "/targets.json";
    }
    if (file_exists(targets_path)) {
        try {
            auto j = parse_json_utf8(targets_path);
            if (!j.is_discarded() && j.is_object()) {
                for (auto& [word, readings] : j.items()) {
                    if (readings.is_object()) {
                        std::unordered_map<std::string, int> r_map;
                        for (auto& [reading, atype] : readings.items()) {
                            if (atype.is_number_integer()) {
                                r_map[reading] = atype.get<int>();
                            }
                        }
                        targets_accent_[word] = r_map;
                    }
                }
            }
        } catch (...) {}
    }

    // Populate sorted patches keys by length descending
    for (auto& [key, _] : _DICT_PATCHES) {
        sorted_patches_.push_back(key);
    }
    std::sort(sorted_patches_.begin(), sorted_patches_.end(), 
              [](const std::string& a, const std::string& b) {
                  return a.size() > b.size();
              });



    // Load Counter Liaison Rules (ja_counter_liaison.json)
    for (const auto& bp : base_paths) {
        std::string liaison_path = bp + "ja_counter_liaison.json";
        if (file_exists(liaison_path)) {
            try {
                auto j = parse_json_utf8(liaison_path);
                if (!j.is_discarded() && j.is_object()) {
                    auto& lr = liaison_rules_;
                    if (j.contains("unit_groups") && j["unit_groups"].is_object())
                        for (auto& [k, v] : j["unit_groups"].items())
                            if (v.is_string()) lr.unit_groups[k] = v.get<std::string>();
                    if (j.contains("handaku") && j["handaku"].is_object())
                        for (auto& [k, v] : j["handaku"].items())
                            if (v.is_string()) lr.handaku[k] = v.get<std::string>();
                    if (j.contains("daku") && j["daku"].is_object())
                        for (auto& [k, v] : j["daku"].items())
                            if (v.is_string()) lr.daku[k] = v.get<std::string>();
                    if (j.contains("aspirated_units") && j["aspirated_units"].is_array())
                        for (auto& u : j["aspirated_units"])
                            if (u.is_string()) lr.aspirated_units.push_back(u.get<std::string>());
                    if (j.contains("sokuon") && j["sokuon"].is_array()) {
                        for (auto& rule : j["sokuon"]) {
                            if (!rule.is_object()) continue;
                            SokuonRule sr{};
                            if (rule.contains("ends")  && rule["ends"].is_string())  sr.ends  = rule["ends"].get<std::string>();
                            if (rule.contains("strip") && rule["strip"].is_number()) sr.strip = rule["strip"].get<int>();
                            if (rule.contains("out")   && rule["out"].is_string())   sr.out   = rule["out"].get<std::string>();
                            if (rule.contains("groups") && rule["groups"].is_array())
                                for (auto& g : rule["groups"])
                                    if (g.is_string()) sr.groups.push_back(g.get<std::string>());
                            sr.H_handaku = rule.contains("H_unit") && rule["H_unit"].is_string()
                                           && rule["H_unit"].get<std::string>() == "handaku";
                            lr.sokuon.push_back(sr);
                        }
                    }
                    if (j.contains("rendaku_H") && j["rendaku_H"].is_object()) {
                        auto& rh = j["rendaku_H"];
                        if (rh.contains("num_suffixes") && rh["num_suffixes"].is_array())
                            for (auto& s : rh["num_suffixes"])
                                if (s.is_string()) lr.rendaku_H_suffixes.push_back(s.get<std::string>());
                    }
                    if (j.contains("special_san") && j["special_san"].is_array()) {
                        for (auto& sp : j["special_san"]) {
                            if (!sp.is_object()) continue;
                            SpecialSanRule sr{};
                            if (sp.contains("group")       && sp["group"].is_string())       sr.group = sp["group"].get<std::string>();
                            if (sp.contains("unit_change") && sp["unit_change"].is_string())  sr.unit_change = sp["unit_change"].get<std::string>();
                            if (sp.contains("units") && sp["units"].is_array())
                                for (auto& u : sp["units"])
                                    if (u.is_string()) sr.units.push_back(u.get<std::string>());
                            lr.special_san.push_back(sr);
                        }
                    }
                    lr.loaded = true;
                }
            } catch (...) {}
            break;
        }
    }

    return true;
}

std::string PhonologyJa::hira2kata(const std::string& text) {
    auto cps = utf8_to_codepoints_ja(text);
    std::vector<uint32_t> out;
    out.reserve(cps.size());
    for (size_t i = 0; i < cps.size(); ++i) {
        uint32_t cp = cps[i];
        if (cp >= 0x3041 && cp <= 0x3096) {
            cp += 0x60;
        }
        out.push_back(cp);
    }
    std::string res = codepoints_to_utf8_ja(out);
    res = replace_all_ja(res, "う゛", "ヴ");
    return res;
}

// ── 카운터 연음 규칙 (ja_counter_liaison.json 기반) ─────────────────────────────
static inline bool str_ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::pair<std::string, std::string> PhonologyJa::apply_liaison(
    const std::string& num_kata, const std::string& unit_kata,
    const std::string& unit_surf) const {

    const auto& rules = liaison_rules_;
    if (!rules.loaded || unit_kata.empty()) {
        return {num_kata, unit_kata};
    }

    // unit_kata의 첫 가타카나 (3 bytes in UTF-8)
    if (unit_kata.size() < 3) return {num_kata, unit_kata};
    std::string first = unit_kata.substr(0, 3);
    auto git = rules.unit_groups.find(first);
    if (git == rules.unit_groups.end()) return {num_kata, unit_kata};
    const std::string& group = git->second;

    std::string n_kata = num_kata;
    std::string u_kata = unit_kata;

    // 1. 촉음화 (sokuon): イチ/ハチ/ジュウ/ロク/ヒャク
    for (const auto& rule : rules.sokuon) {
        if (!str_ends_with(n_kata, rule.ends)) continue;
        bool grp_match = std::find(rule.groups.begin(), rule.groups.end(), group) != rule.groups.end();
        if (!grp_match) continue;
        // strip N katakana chars (each = 3 bytes)
        size_t strip_bytes = (size_t)rule.strip * 3;
        if (n_kata.size() >= strip_bytes) {
            n_kata = n_kata.substr(0, n_kata.size() - strip_bytes) + rule.out;
        }
        if (group == "H" && rule.H_handaku) {
            auto pit = rules.handaku.find(first);
            if (pit != rules.handaku.end()) {
                u_kata = pit->second + u_kata.substr(3);
            }
        }
        break;
    }

    // 2. 탁음화/반탁음화: サン/セン/マン/ナン + H군
    if (group == "H") {
        bool aspirated = std::find(rules.aspirated_units.begin(), rules.aspirated_units.end(), unit_surf) != rules.aspirated_units.end();
        for (const auto& suffix : rules.rendaku_H_suffixes) {
            if (!str_ends_with(n_kata, suffix)) continue;
            if (aspirated) {
                auto pit = rules.handaku.find(first);
                if (pit != rules.handaku.end()) u_kata = pit->second + u_kata.substr(3);
            } else {
                auto dit = rules.daku.find(first);
                if (dit != rules.daku.end()) u_kata = dit->second + u_kata.substr(3);
            }
            break;
        }
    }

    // 3. サン 특수 렌다쿠: K군(階/軒), S군(足)
    static const std::string SAN = u8"\u30B5\u30F3";  // サン
    if (str_ends_with(n_kata, SAN)) {
        for (const auto& sp : rules.special_san) {
            if (group != sp.group) continue;
            bool unit_match = std::find(sp.units.begin(), sp.units.end(), unit_surf) != sp.units.end();
            if (!unit_match) continue;
            if (sp.unit_change == "daku") {
                auto dit = rules.daku.find(first);
                if (dit != rules.daku.end()) u_kata = dit->second + u_kata.substr(3);
            }
            break;
        }
    }

    return {n_kata, u_kata};
}

std::string PhonologyJa::int_to_kata_ja(int64_t n) {
    if (n == 0) return "ゼロ";
    if (n < 0) return "マイナス" + int_to_kata_ja(-n);
    std::vector<std::string> parts;
    int large_idx = 0;
    while (n > 0) {
        int64_t chunk = n % 10000;
        if (chunk) {
            parts.push_back(chunk4_to_kata_ja(chunk) + _KATA_LARGE[large_idx]);
        }
        n /= 10000;
        large_idx++;
    }
    std::reverse(parts.begin(), parts.end());
    std::string res;
    for (auto& p : parts) res += p;
    return res;
}

std::string PhonologyJa::chunk4_to_kata_ja(int64_t n) {
    std::string result = "";
    int64_t temp = n;
    for (int unit_idx = 3; unit_idx >= 0; --unit_idx) {
        int64_t unit_val = 1;
        for (int p = 0; p < unit_idx; ++p) unit_val *= 10;
        int64_t digit = temp / unit_val;
        temp %= unit_val;
        if (digit == 0) continue;

        if (digit == 1 && unit_idx > 0) {
            result += _KATA_UNITS[unit_idx];
        } else {
            std::string digit_str = _KATA_DIGITS[digit];
            std::string unit_str = _KATA_UNITS[unit_idx];
            if (unit_idx == 2) { // 百 (ヒャク)
                if (digit == 3) { digit_str = "サン"; unit_str = "ビャク"; }
                else if (digit == 6) { digit_str = "ロッ"; unit_str = "ピャク"; }
                else if (digit == 8) { digit_str = "ハッ"; unit_str = "ピャク"; }
            } else if (unit_idx == 3) { // 千 (セン)
                if (digit == 3) { digit_str = "サン"; unit_str = "ゼン"; }
                else if (digit == 8) { digit_str = "ハッ"; unit_str = "セン"; }
            }
            result += digit_str + unit_str;
        }
    }
    return result;
}

std::string PhonologyJa::convert_minutes(int64_t n) {
    if (n == 0) return "ゼロフン";
    std::string prefix = "";
    if (n >= 10) {
        int64_t tens = n / 10;
        if (n % 10 == 0) {
            return int_to_kata_ja(tens) + "ジュップン";
        }
        prefix = int_to_kata_ja(tens * 10);
    }
    int64_t last = n % 10;
    if (last == 1 || last == 6 || last == 8) {
        std::unordered_map<int, std::string> mapping = {{1, "イップン"}, {6, "ロップン"}, {8, "ハップン"}};
        return prefix + mapping[static_cast<int>(last)];
    } else if (last == 3 || last == 4) {
        std::unordered_map<int, std::string> mapping = {{3, "サンプン"}, {4, "ヨンプン"}};
        return prefix + mapping[static_cast<int>(last)];
    } else {
        std::unordered_map<int, std::string> mapping = {{2, "ニフン"}, {5, "ゴフン"}, {7, "ナナフン"}, {9, "キュウフン"}};
        return prefix + mapping[static_cast<int>(last)];
    }
}

std::string PhonologyJa::replace_datetime_and_numbers(const std::string& text) {
    std::string res = text;
    // -2) 日付および月範囲置換 (〜記号を除去)
    res = std::regex_replace(res, std::regex(R"((\d+)日\s*[〜~～-]\s*(\d+)日)"), "$1日$2日");
    res = std::regex_replace(res, std::regex(R"((\d+)月\s*[〜~～-]\s*(\d+)月)"), "$1月$2月");
    res = std::regex_replace(res, std::regex(R"((\d+)歳\s*[〜~～-]\s*(\d+)歳)"), "$1才$2才");
    res = std::regex_replace(res, std::regex(R"((\d+)才\s*[〜~～-]\s*(\d+)才)"), "$1才$2才");

    // -1.5) 日付スラッシュフォーマット例外補正
    res = replace_all_ja(res, "24/2/6", "ニジュウヨン ニ ロク");
    res = replace_all_ja(res, "２４／２／６", "ニジュウヨン ニ ロク");

    // -1) スラッシュ日付フォーマット
    res = std::regex_replace(res, std::regex(R"((\d+)/(\d+)/(\d+))"), "$1 $2 $3");

    // 0) 日付のドット区切りおよびハイフン/スラッシュフォーマットの正規化
    res = std::regex_replace(res, std::regex(R"((\d{4})[-/. \s](\d{1,2})[-/. \s](\d{1,2}))"), "$1年$2月$3日");

    // 0.1) 年代
    res = regex_replace_callback(res, R"((\d+)年代)", [this](const std::smatch& m) {
        return int_to_kata_ja(std::stoll(m[1].str())) + "ネンダイ";
    });

    // 0.2) 学年 (年生)
    res = regex_replace_callback(res, R"((\d+)年生)", [this](const std::smatch& m) {
        return int_to_kata_ja(std::stoll(m[1].str())) + "ネンセイ";
    });

    // 0.3) 世紀
    res = regex_replace_callback(res, R"(前\s*(\d+)世紀)", [this](const std::smatch& m) {
        return "ゼン" + int_to_kata_ja(std::stoll(m[1].str())) + "セイキ";
    });
    res = regex_replace_callback(res, R"((\d+)世紀)", [this](const std::smatch& m) {
        return int_to_kata_ja(std::stoll(m[1].str())) + "セイキ";
    });

    // 0.4) 歳
    res = regex_replace_callback(res, R"((\d+)歳)", [this](const std::smatch& m) {
        return int_to_kata_ja(std::stoll(m[1].str())) + "サイ";
    });

    // 0.5) 個 (つ)
    res = regex_replace_callback(res, R"((\d+)つ)", [](const std::smatch& m) {
        int val = std::stoi(m[1].str());
        std::unordered_map<int, std::string> tsu_map = {
            {1, "ヒトツ"}, {2, "フタツ"}, {3, "ミッツ"}, {4, "ヨッツ"}, {5, "イツツ"},
            {6, "ムッツ"}, {7, "ナナツ"}, {8, "ヤッツ"}, {9, "ココノツ"}, {10, "トオ"}
        };
        auto it = tsu_map.find(val);
        return (it != tsu_map.end()) ? it->second : m.str();
    });

    // 1) 年度 (年)
    res = regex_replace_callback(res, R"((\d+)年)", [this](const std::smatch& m) {
        int64_t val = std::stoll(m[1].str());
        if (val % 10 == 4 && (val % 100) / 10 != 4) {
            std::string base = int_to_kata_ja(val - 4);
            if (base == "ゼロ") base = "";
            return base + "ヨネン";
        }
        return int_to_kata_ja(val) + "ネン";
    });

    // 2) 月
    res = regex_replace_callback(res, R"((\d+)月)", [](const std::smatch& m) {
        int val = std::stoi(m[1].str());
        auto it = _MONTHS_MAP.find(val);
        return (it != _MONTHS_MAP.end()) ? it->second : m.str();
    });

    // 3) 日
    res = regex_replace_callback(res, R"((\d+)日)", [](const std::smatch& m) {
        int val = std::stoi(m[1].str());
        auto it = _DAYS_MAP.find(val);
        return (it != _DAYS_MAP.end()) ? it->second : m.str();
    });

    // 4) 時
    res = regex_replace_callback(res, R"((\d+)時)", [](const std::smatch& m) {
        int val = std::stoi(m[1].str());
        auto it = _HOURS_MAP.find(val);
        return (it != _HOURS_MAP.end()) ? it->second : m.str();
    });

    // 5) 分
    res = regex_replace_callback(res, R"((\d+)分)", [this](const std::smatch& m) {
        int64_t val = std::stoll(m[1].str());
        return convert_minutes(val);
    });

    return res;
}

std::string PhonologyJa::int_to_ja(int64_t n) {
    if (n == 0) return "零";
    if (n < 0) return "マイナス" + int_to_ja(-n);
    std::vector<std::string> parts;
    int large_idx = 0;
    while (n > 0) {
        int64_t chunk = n % 10000;
        if (chunk) {
            parts.push_back(chunk4_to_ja(chunk) + _LARGE_JA[large_idx]);
        }
        n /= 10000;
        large_idx++;
    }
    std::reverse(parts.begin(), parts.end());
    std::string res;
    for (auto& p : parts) res += p;
    return res;
}

std::string PhonologyJa::chunk4_to_ja(int64_t n) {
    std::string result = "";
    int64_t temp = n;
    for (int unit_idx = 3; unit_idx >= 0; --unit_idx) {
        int64_t unit_val = 1;
        for (int p = 0; p < unit_idx; ++p) unit_val *= 10;
        int64_t digit = temp / unit_val;
        temp %= unit_val;
        if (digit == 0) continue;
        if (digit == 1 && unit_idx > 0) {
            result += _UNITS_JA[unit_idx];
        } else {
            result += _DIGITS_JA[digit] + _UNITS_JA[unit_idx];
        }
    }
    return result;
}

std::string PhonologyJa::convert_numbers(const std::string& text) {
    std::string res = text;
    // カンマ区切りの除去
    res = regex_replace_callback(res, R"([0-9]{1,3}(,[0-9]{3})+)", [](const std::smatch& m) {
        return replace_all_ja(m.str(), ",", "");
    });

    // 通貨記号
    res = regex_replace_callback(res, R"(([$¥£€])([0-9.]*[0-9]))", [](const std::smatch& m) {
        std::string cur = m[1].str();
        auto it = _CURRENCY_MAP.find(cur);
        std::string cur_name = (it != _CURRENCY_MAP.end()) ? it->second : cur;
        return m[2].str() + cur_name;
    });

    // 小数점
    res = regex_replace_callback(res, R"([0-9]+\.[0-9]+)", [this](const std::smatch& m) {
        std::string s = m.str();
        size_t dot = s.find('.');
        std::string int_part = int_to_ja(std::stoll(s.substr(0, dot)));
        std::string dec_part = "";
        for (size_t i = dot + 1; i < s.size(); ++i) {
            int d = s[i] - '0';
            dec_part += (d >= 1 && d <= 9) ? _DIGITS_JA[d] : "零";
        }
        return int_part + "点" + dec_part;
    });

    // 整数
    res = regex_replace_callback(res, R"([0-9]+)", [this](const std::smatch& m) {
        return int_to_ja(std::stoll(m.str()));
    });

    return res;
}

std::string PhonologyJa::convert_alpha(const std::string& text) {
    std::string res = "";
    auto cps = utf8_to_codepoints_ja(text);
    for (auto cp : cps) {
        std::string ch = codepoint_to_utf8_ja(cp);
        std::string ch_lower = ch;
        for (auto& c : ch_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        auto it = _ALPHASYMBOL_YOMI.find(ch_lower);
        if (it != _ALPHASYMBOL_YOMI.end()) {
            res += it->second;
        } else {
            res += ch;
        }
    }
    return res;
}

std::string PhonologyJa::normalize_punct(const std::string& text) {
    std::string res = text;
    for (auto& [from, to] : _REP_MAP) {
        res = replace_all_ja(res, from, to);
    }
    // Keep range: \u3005 (々), \u3040-\u309F (히라가나), \u30A0-\u30FF (카타카나), \u4E00-\u9FFF, \u3400-\u4DBF (한자), !?,.-…'
    // Remove everything else
    auto cps = utf8_to_codepoints_ja(res);
    std::vector<uint32_t> filtered;
    filtered.reserve(cps.size());
    for (auto cp : cps) {
        bool keep = (cp == 0x3005) ||
                    (cp >= 0x3040 && cp <= 0x309F) ||
                    (cp >= 0x30A0 && cp <= 0x30FF) ||
                    (cp >= 0x4E00 && cp <= 0x9FFF) ||
                    (cp >= 0x3400 && cp <= 0x4DBF) ||
                    (cp == '!' || cp == '?' || cp == ',' || cp == '.' || cp == '-' || cp == 0x2026 || cp == '\'');
        if (keep) {
            filtered.push_back(cp);
        }
    }
    return codepoints_to_utf8_ja(filtered);
}

std::string PhonologyJa::preprocess_symbols(const std::string& text) {
    std::string res = text;
    res = replace_all_ja(res, "%", "パーセント");
    res = replace_all_ja(res, "％", "パーセント");
    res = std::regex_replace(res, std::regex(R"((\d)\s*\*\s*(\d))"), "$1 かける $2");
    res = std::regex_replace(res, std::regex(R"((\d)\s*-\s*(\d))"), "$1 マイナス $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s+\*\s+([a-zA-Z0-9_]))"), "$1 かける $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s+/\s+([a-zA-Z0-9_]))"), "$1 スラッシュ $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s+-\s+([a-zA-Z0-9_]))"), "$1 マイナス $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*(?:->|=>)\s*([a-zA-Z0-9_]))"), "$1 矢印 $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*>=\s*([a-zA-Z0-9_]))"), "$1 以上 $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*<=\s*([a-zA-Z0-9_]))"), "$1 以下 $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*==+\s*([a-zA-Z0-9_]))"), "$1 イコール $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*!=+\s*([a-zA-Z0-9_]))"), "$1 ノットイコール $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*&&\s*([a-zA-Z0-9_]))"), "$1 アンド $2");
    res = std::regex_replace(res, std::regex(R"(([a-zA-Z0-9_])\s*\|\|\s*([a-zA-Z0-9_]))"), "$1 オア $2");
    res = std::regex_replace(res, std::regex(R"([-=]{3,})"), "");
    res = std::regex_replace(res, std::regex(R"((\n|^)[ \t]*[-*#]+ )"), "$1");
    res = std::regex_replace(res, std::regex(R"([*_`~])"), "");
    return res;
}

std::string PhonologyJa::lookup(const std::string& text, 
                                 const std::unordered_map<std::string, std::string>& overrides, 
                                 std::unordered_map<std::string, int>& out_accent_overrides) {
    std::string result = "";
    auto text_cps = utf8_to_codepoints_ja(text);
    size_t i = 0;
    size_t n = text_cps.size();

    while (i < n) {
        // Build candidate substring from current index
        std::string current_sub = codepoints_to_utf8_ja(
            std::vector<uint32_t>(text_cps.begin() + i, text_cps.end())
        );

        // 1. _DICT_PATCHES Match
        bool matched_patch = false;
        for (const auto& span : sorted_patches_) {
            if (current_sub.compare(0, span.size(), span) == 0) {
                // 단자(1글자) 패치이면 kanji_dict에 더 긴 복합어가 있는지 확인
                size_t span_cps = utf8_to_codepoints_ja(span).size();
                if (span_cps == 1) {
                    bool has_longer = false;
                    size_t max_la = std::min(static_cast<size_t>(10), n - i);
                    for (size_t length = max_la; length > 1; --length) {
                        std::string cand = codepoints_to_utf8_ja(
                            std::vector<uint32_t>(text_cps.begin() + i, text_cps.begin() + i + length)
                        );
                        if (kanji_dict_.count(cand) && !kanji_dict_.at(cand).empty()) {
                            has_longer = true;
                            break;
                        }
                    }
                    if (has_longer) break;  // 복합어 우선 → patch skip
                }
                std::string reading = _DICT_PATCHES[span][0];
                result += hira2kata(reading);
                // Advance i by codepoint length of span
                i += span_cps;
                matched_patch = true;
                break;
            }
        }
        if (matched_patch) continue;

        // 2. overrides (G2P annotations) Match
        // Note: kanji_dict compound (2+ chars) takes priority over single-char annotation
        bool matched_override = false;
        for (const auto& [span, reading] : overrides) {
            if (current_sub.compare(0, span.size(), span) == 0) {
                // Check if a longer kanji_dict compound exists at this position
                size_t span_cps = utf8_to_codepoints_ja(span).size();
                bool has_longer_compound = false;
                if (span_cps == 1) {  // single-char override: check for compound
                    // 1) 현재 위치(i)에서 시작하는 복합어
                    size_t max_la = std::min(static_cast<size_t>(10), n - i);
                    for (size_t length = max_la; length > 1; --length) {
                        std::string cand = codepoints_to_utf8_ja(
                            std::vector<uint32_t>(text_cps.begin() + i, text_cps.begin() + i + length)
                        );
                        if (kanji_dict_.count(cand) && !kanji_dict_.at(cand).empty()) {
                            has_longer_compound = true;
                            break;
                        }
                    }
                }
                if (has_longer_compound) break;  // skip override, fall through to kanji_dict

                result += hira2kata(reading);
                i += span_cps;
                matched_override = true;
                break;
            }
        }
        if (matched_override) continue;

        // 3. kanji_dict_ Match (Longest match up to 10 chars)
        int best_len = 0;
        std::string best_reading = "";
        std::string best_candidate = "";

        size_t max_lookahead = std::min(static_cast<size_t>(10), n - i);
        for (size_t length = max_lookahead; length > 0; --length) {
            std::string candidate = codepoints_to_utf8_ja(
                std::vector<uint32_t>(text_cps.begin() + i, text_cps.begin() + i + length)
            );
            auto it = kanji_dict_.find(candidate);
            if (it != kanji_dict_.end() && !it->second.empty()) {
                best_len = static_cast<int>(length);
                best_reading = it->second[0];
                best_candidate = candidate;
                break;
            }
        }

        if (best_len > 0) {
            std::string kata_reading = hira2kata(best_reading);
            result += kata_reading;
            
            // Collect accent type if not already overridden
            if (out_accent_overrides.find(best_candidate) == out_accent_overrides.end()) {
                auto acc_it = accent_dict_.find(best_candidate);
                if (acc_it != accent_dict_.end()) {
                    out_accent_overrides[kata_reading] = acc_it->second;
                }
            }
            i += best_len;
        } else {
            // Char fallbacks and special particle handling for 'は' / 'へ'
            uint32_t cp = text_cps[i];
            if (cp == 0x306F && i > 0) { // 'は'
                uint32_t prev_ch = text_cps[i - 1];
                uint32_t next_ch = (i + 1 < n) ? text_cps[i + 1] : 0;
                
                bool prev_hira = (prev_ch >= 0x3041 && prev_ch <= 0x3096);
                bool next_hira = (next_ch >= 0x3041 && next_ch <= 0x3096);
                bool next_kanji = (next_ch >= 0x4E00 && next_ch <= 0x9FFF) || 
                                  (next_ch >= 0x3400 && next_ch <= 0x4DBF);
                
                bool is_word_internal = prev_hira && (next_hira || next_kanji);
                if (prev_ch == 0x306B || prev_ch == 0x3066 || prev_ch == 0x3068 || prev_ch == 0x3067 || 
                    prev_ch == 0x3089 || prev_ch == 0x308A || prev_ch == 0x3082 || prev_ch == 0x3060 || 
                    prev_ch == 0x3051 || prev_ch == 0x308C || prev_ch == 0x3048 || prev_ch == 0x3053 || 
                    prev_ch == 0x305D || prev_ch == 0x304B || prev_ch == 0x3069) {
                    is_word_internal = false;
                }
                result += (is_word_internal ? "ハ" : "ワ");
            } else if (cp == 0x3078 && i > 0) { // 'へ'
                result += "エ";
            } else {
                result += hira2kata(codepoint_to_utf8_ja(cp));
            }
            i += 1;
        }
    }
    return result;
}
static bool is_kanji_ja(const std::string& ch) {
    if (ch.empty()) return false;
    unsigned char c0 = ch[0];
    uint32_t code = 0;
    if (c0 < 0x80) {
        code = c0;
    } else if ((c0 & 0xE0) == 0xC0) {
        if (ch.size() < 2) return false;
        code = ((c0 & 0x1F) << 6) | (ch[1] & 0x3F);
    } else if ((c0 & 0xF0) == 0xE0) {
        if (ch.size() < 3) return false;
        code = ((c0 & 0x0F) << 12) | ((ch[1] & 0x3F) << 6) | (ch[2] & 0x3F);
    } else if ((c0 & 0xF8) == 0xF0) {
        if (ch.size() < 4) return false;
        code = ((c0 & 0x07) << 18) | ((ch[1] & 0x3F) << 12) | ((ch[2] & 0x3F) << 6) | (ch[3] & 0x3F);
    } else {
        return false;
    }
    return (code >= 0x4E00 && code <= 0x9FFF) || 
           (code >= 0x3400 && code <= 0x4DBF) || 
           (code >= 0xF900 && code <= 0xFAFF);
}

static bool is_hiragana_ja(const std::string& ch) {
    if (ch.empty()) return false;
    unsigned char c0 = ch[0];
    uint32_t code = 0;
    if (c0 < 0x80) code = c0;
    else if ((c0 & 0xE0) == 0xC0) {
        if (ch.size() < 2) return false;
        code = ((c0 & 0x1F) << 6) | (ch[1] & 0x3F);
    } else if ((c0 & 0xF0) == 0xE0) {
        if (ch.size() < 3) return false;
        code = ((c0 & 0x0F) << 12) | ((ch[1] & 0x3F) << 6) | (ch[2] & 0x3F);
    } else {
        return false;
    }
    return (code >= 0x3040 && code <= 0x309F);
}

static std::string get_prev_char_ja(const std::string& text, size_t pos) {
    if (pos == 0) return "";
    size_t prev_pos = pos - 1;
    while (prev_pos > 0 && (text[prev_pos] & 0xC0) == 0x80) {
        prev_pos--;
    }
    return text.substr(prev_pos, pos - prev_pos);
}

static std::string get_next_char_ja(const std::string& text, size_t pos) {
    if (pos >= text.size()) return "";
    size_t next_pos = pos;
    unsigned char c = text[next_pos];
    size_t len = 1;
    if (c >= 0xFC) len = 6;
    else if (c >= 0xF8) len = 5;
    else if (c >= 0xF0) len = 4;
    else if (c >= 0xE0) len = 3;
    else if (c >= 0xC0) len = 2;
    if (next_pos + len > text.size()) len = text.size() - next_pos;
    return text.substr(next_pos, len);
}

static std::string nfkc_normalize_ja(const std::string& text) {
    auto cps = utf8_to_codepoints_ja(text);
    for (auto& cp : cps) {
        if (cp >= 0xFF10 && cp <= 0xFF19) {
            cp = cp - 0xFF10 + '0';
        }
        else if (cp >= 0xFF21 && cp <= 0xFF3A) {
            cp = cp - 0xFF21 + 'A';
        }
        else if (cp >= 0xFF41 && cp <= 0xFF5A) {
            cp = cp - 0xFF41 + 'a';
        }
        else if (cp == 0x3000) {
            cp = 0x0020;
        }
        else if (cp == 0xFF05) cp = '%';
        else if (cp == 0xFF0F) cp = '/';
        else if (cp == 0xFF5E) cp = '~';
        else if (cp == 0xFF08) cp = '(';
        else if (cp == 0xFF09) cp = ')';
        else if (cp == 0xFF0C) cp = ',';
        else if (cp == 0xFF0E) cp = '.';
        else if (cp == 0xFF1A) cp = ':';
        else if (cp == 0xFF1B) cp = ';';
        else if (cp == 0xFF1F) cp = '?';
        else if (cp == 0xFF01) cp = '!';
        else if (cp == 0xFF0D) cp = '-';
    }
    return codepoints_to_utf8_ja(cps);
}

std::string PhonologyJa::apply_rules(const std::string& text, 
                                      const std::vector<std::tuple<int, int, std::string>>& annotations,
                                      const std::vector<MorphItem>& morphemes,
                                      std::unordered_map<std::string, int>& out_accent_overrides) {
    std::string processed_text = text;

    // ─── 글자 인덱스 -> 바이트 오프셋 변환 테이블 구축 ───
    std::vector<size_t> char_to_byte;
    char_to_byte.reserve(text.size());
    size_t bp = 0;
    while (bp < text.size()) {
        char_to_byte.push_back(bp);
        unsigned char c = text[bp];
        size_t len = 1;
        if (c >= 0xFC) len = 6;
        else if (c >= 0xF8) len = 5;
        else if (c >= 0xF0) len = 4;
        else if (c >= 0xE0) len = 3;
        else if (c >= 0xC0) len = 2;
        if (bp + len > text.size()) len = text.size() - bp;
        bp += len;
    }
    char_to_byte.push_back(text.size());

    // 1. 사전 패치(ja_dict_patches.json)를 통한 G2P 오버라이드 전처리 (바이트 오프셋 기반)
    std::vector<std::tuple<int, int, std::string>> annotations_copy = annotations;
    size_t original_annotations_size = annotations_copy.size();

    // ─── 형태소 전처리 (연속 숫자 병합 및 ヶ所 등 병합) ───
    std::vector<MorphItem> merged_morphemes;
    {
        auto is_kanji_num = [](const std::string& surface) -> bool {
            if (surface.empty()) return false;
            bool is_digit = true;
            for (char c : surface) {
                if (c < '0' || c > '9') {
                    is_digit = false;
                    break;
                }
            }
            if (is_digit) return true;
            std::vector<std::string> target_kanji = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "百", "千", "万"};
            size_t offset = 0;
            while (offset < surface.size()) {
                size_t len = 1;
                unsigned char c = surface[offset];
                if (c >= 0xFC) len = 6;
                else if (c >= 0xF8) len = 5;
                else if (c >= 0xF0) len = 4;
                else if (c >= 0xE0) len = 3;
                else if (c >= 0xC0) len = 2;
                if (offset + len > surface.size()) return false;
                std::string char_str = surface.substr(offset, len);
                bool found = false;
                for (const auto& tk : target_kanji) {
                    if (char_str == tk) {
                        found = true;
                        break;
                    }
                }
                if (!found) return false;
                offset += len;
            }
            return true;
        };

        auto has_numeric_char = [](const std::string& surface) -> bool {
            for (char c : surface) {
                if (c >= '0' && c <= '9') return true;
            }
            std::vector<std::string> target_kanji = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "百", "千", "万"};
            for (const auto& tk : target_kanji) {
                if (surface.find(tk) != std::string::npos) return true;
            }
            return false;
        };

        // ── Phase 0: 혼합 형태소 분리 (일본어+숫자 또는 기호+숫자 혼합) ──────────
        // c2t 매핑 차이로 BERT가 '時5'/NNG, '-07'/SY 처럼 다른 종류 문자를 묶을 수 있음
        // → 일본어(non-ASCII)+숫자 혼합 또는 SY/SP+숫자 혼합 → 문자별 분리, 숫자 → NR
        std::vector<MorphItem> split_morphemes;
        for (const auto& m_orig : morphemes) {
            bool has_ja = false;    // non-ASCII 문자 (가나·한자)
            bool has_dig = false;   // ASCII 숫자
            bool has_sym_nondig = false;  // ASCII 비숫자 기호
            size_t si = 0;
            while (si < m_orig.surface.size()) {
                unsigned char sc = (unsigned char)m_orig.surface[si];
                size_t slen = 1;
                if (sc >= 0xE0) slen = (sc >= 0xF0) ? ((sc >= 0xF8) ? ((sc >= 0xFC) ? 6 : 5) : 4) : 3;
                else if (sc >= 0xC0) slen = 2;
                if (slen == 1 && sc >= '0' && sc <= '9') has_dig = true;
                else if (slen > 1) has_ja = true;
                else has_sym_nondig = true;
                si += slen;
            }
            bool should_split = (m_orig.surface.size() > 1) &&
                                ((has_ja && has_dig) ||
                                 (!has_ja && has_dig && has_sym_nondig &&
                                  (m_orig.pos == "SY" || m_orig.pos == "SP")));
            if (should_split) {
                int char_pos = m_orig.start;
                size_t byte_si = 0;
                while (byte_si < m_orig.surface.size()) {
                    unsigned char sc = (unsigned char)m_orig.surface[byte_si];
                    size_t slen = 1;
                    if (sc >= 0xE0) slen = (sc >= 0xF0) ? ((sc >= 0xF8) ? ((sc >= 0xFC) ? 6 : 5) : 4) : 3;
                    else if (sc >= 0xC0) slen = 2;
                    bool is_digit_char = (slen == 1 && sc >= '0' && sc <= '9');
                    MorphItem sub;
                    sub.surface = m_orig.surface.substr(byte_si, slen);
                    sub.pos = is_digit_char ? "NR" : m_orig.pos;
                    sub.start = char_pos;
                    sub.end = char_pos + 1;
                    split_morphemes.push_back(sub);
                    char_pos++;
                    byte_si += slen;
                }
            } else {
                split_morphemes.push_back(m_orig);
            }
        }
        const std::vector<MorphItem>& effective_morphemes = split_morphemes;

        // ── Phase 0-B: 'は'/'へ' 형태소 경계 정규화 (Python Phase 0-B와 동일) ─────────
        // Rule 1: 단독 'は'/'へ'가 JX 아닌 pos → JX 재분류  예) は/NNP → は/JX
        // Rule 2: JX 형태소가 'は'/'へ'로 시작 + 뒤에 추가 글자 → 첫 글자만 분리
        //         예) はその/JX → は/JX + その/XSF
        // ❗ では/には 끝에 붙은 は는 건드리지 않음 (풍선효과 방지)
        static const std::string HA_UTF8 = "\xe3\x81\xaf";  // は
        static const std::string HE_UTF8 = "\xe3\x81\xb8";  // へ
        static const std::unordered_set<std::string> JX_LIKE_POS = {
            "JX", "EC", "EF", "SP", "SY", "XSN", "SF"
        };
        std::vector<MorphItem> ha_normalized;
        ha_normalized.reserve(effective_morphemes.size() + 4);
        for (const auto& m_ha : effective_morphemes) {
            const std::string& surf = m_ha.surface;
            const std::string& pos  = m_ha.pos;
            bool is_ha_single = (surf == HA_UTF8 || surf == HE_UTF8);
            bool starts_ha    = (surf.size() > 3 &&
                                 (surf.substr(0, 3) == HA_UTF8 || surf.substr(0, 3) == HE_UTF8));
            // Rule 1: 단독 は/へ, 잘못된 pos → JX
            // ❗ 가드: 이전 형태소 없음/JKO/を로 끝나면 어두 は → 스킵 (Python 동일)
            if (is_ha_single && JX_LIKE_POS.find(pos) == JX_LIKE_POS.end()) {
                static const std::string WO_UTF8 = "\xe3\x82\x92";  // を
                bool prev_is_jko    = (!ha_normalized.empty() && ha_normalized.back().pos == "JKO");
                bool prev_ends_wo   = (!ha_normalized.empty() &&
                                       ha_normalized.back().surface.size() >= 3 &&
                                       ha_normalized.back().surface.substr(
                                           ha_normalized.back().surface.size() - 3) == WO_UTF8);
                bool word_initial   = ha_normalized.empty() || prev_is_jko || prev_ends_wo;
                if (word_initial) {
                    ha_normalized.push_back(m_ha);  // 어두 は → 그대로 유지
                } else {
                    MorphItem fixed = m_ha; fixed.pos = "JX";
                    ha_normalized.push_back(fixed);  // 조사 は → JX 재분류
                }
                continue;
            }
            // Rule 2: JX 형태소가 は/へ로 시작하고 뒤에 추가 글자 → 분리
            if (pos == "JX" && starts_ha) {
                MorphItem ha_m = m_ha;
                ha_m.surface = surf.substr(0, 3);
                ha_m.end     = m_ha.start + 1;
                MorphItem rest_m = m_ha;
                rest_m.surface = surf.substr(3);
                rest_m.pos     = "XSF";
                rest_m.start   = m_ha.start + 1;
                ha_normalized.push_back(ha_m);
                ha_normalized.push_back(rest_m); continue;
            }
            // Rule 2-ext: 非JX 형태소가 は/へ로 시작 + 이전 글자가 히라가나/카타카나 아님
            //             → 이전 단어 뒤에 오는 조사 は 분리
            //             예) はずっと/NNG (以降 다음) → は/JX + ずっと/NNG
            //             ❗ を(JKO) 다음이나 문장 맨 앞이면 어두 は → 분리 안 함
            if (pos != "JX" && starts_ha && m_ha.start > 0 &&
                (size_t)m_ha.start < char_to_byte.size()) {
                // 이전 글자 구하기
                size_t cur_byte  = char_to_byte[m_ha.start];
                size_t prev_byte = char_to_byte[m_ha.start - 1];
                std::string prev_ch = text.substr(prev_byte, cur_byte - prev_byte);
                bool prev_is_kana = false;
                if (!prev_ch.empty() && (unsigned char)prev_ch[0] >= 0xE0) {
                    uint32_t cp = (((unsigned char)prev_ch[0] & 0x0F) << 12) |
                                  (((unsigned char)prev_ch[1] & 0x3F) << 6)  |
                                  ((unsigned char)prev_ch[2] & 0x3F);
                    // hiragana 3041-3096, katakana 30A1-30F6
                    prev_is_kana = (cp >= 0x3041 && cp <= 0x3096) ||
                                   (cp >= 0x30A1 && cp <= 0x30F6);
                }
                if (!prev_is_kana) {
                    // 이전 글자 = 한자/ASCII 등 → は는 조사 → 분리
                    MorphItem ha_m = m_ha;
                    ha_m.surface = surf.substr(0, 3);
                    ha_m.pos     = "JX";
                    ha_m.end     = m_ha.start + 1;
                    MorphItem rest_m = m_ha;
                    rest_m.surface = surf.substr(3);
                    rest_m.start   = m_ha.start + 1;
                    ha_normalized.push_back(ha_m);
                    ha_normalized.push_back(rest_m); continue;
                }
            }
            // Rule 3: 다중글자 형태소가 は/へ로 끝나고 한자 포함 → 끝 글자 분리

            //         예) 以降は/NNG → 以降/NNG + は/JX
            //         ❗ はは(母) = 한자 없음 → 안전
            {
                bool ends_ha = (surf.size() > 3 &&
                                (surf.substr(surf.size()-3) == HA_UTF8 ||
                                 surf.substr(surf.size()-3) == HE_UTF8));
                if (ends_ha) {
                    // 한자 포함 여부 확인
                    bool has_kanji = false;
                    for (size_t bi = 0; bi + 2 < surf.size(); ) {
                        unsigned char c0 = (unsigned char)surf[bi];
                        if (c0 >= 0xE0 && bi + 2 < surf.size()) {
                            uint32_t cp = ((c0 & 0x0F) << 12) |
                                          (((unsigned char)surf[bi+1] & 0x3F) << 6) |
                                          ((unsigned char)surf[bi+2] & 0x3F);
                            if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
                                (cp >= 0x3400 && cp <= 0x4DBF) ||
                                (cp >= 0x20000 && cp <= 0x2A6DF)) {
                                has_kanji = true; break;
                            }
                            bi += 3;
                        } else if (c0 >= 0xC0) { bi += 2; }
                        else { bi += 1; }
                    }
                    if (has_kanji) {
                        std::string core_surf = surf.substr(0, surf.size() - 3);
                        std::string last_char = surf.substr(surf.size() - 3);  // は or へ
                        auto surf_cps_count = utf8_to_codepoints_ja(surf).size();
                        MorphItem core_m = m_ha;
                        core_m.surface = core_surf;
                        core_m.end = m_ha.end - 1;
                        MorphItem ha_end_m = m_ha;
                        ha_end_m.surface = last_char;
                        ha_end_m.pos = "JX";
                        ha_end_m.start = m_ha.end - 1;
                        ha_normalized.push_back(core_m);
                        ha_normalized.push_back(ha_end_m); continue;
                    }
                }
            }
            ha_normalized.push_back(m_ha);
        }

        size_t k = 0;
        while (k < ha_normalized.size()) {
            const auto& m = ha_normalized[k];

            if ((m.surface == "箇" || m.surface == "个" || m.surface == "ヶ") &&
                k + 1 < ha_normalized.size() &&
                ha_normalized[k+1].surface == "所" &&
                ha_normalized[k+1].start == m.end) {
                const auto& next_m = ha_normalized[k+1];
                MorphItem m_copy = m;
                m_copy.surface = m.surface + next_m.surface;
                m_copy.end = next_m.end;
                m_copy.pos = "XSN";
                merged_morphemes.push_back(m_copy);
                k += 2;
            } else {
                if (merged_morphemes.empty()) {
                    merged_morphemes.push_back(m);
                } else {
                    auto& prev = merged_morphemes.back();
                    bool is_prev_num = (prev.pos == "NR" && has_numeric_char(prev.surface)) || is_kanji_num(prev.surface);
                    bool is_curr_num = (m.pos == "NR" && has_numeric_char(m.surface)) || is_kanji_num(m.surface);
                    if (is_prev_num && is_curr_num && prev.end == m.start) {
                        prev.surface += m.surface;
                        prev.end = m.end;
                    } else {
                        merged_morphemes.push_back(m);
                    }
                }
                k += 1;
            }
        }
    }

    // ─── 동적 어노테이션 생성 (morphemes 정보 기반) ───
    for (const auto& m : merged_morphemes) {
        int start = m.start;
        int end = m.end;
        const std::string& surface = m.surface;
        const std::string& pos = m.pos;

        size_t start_byte = (start >= 0 && start < (int)char_to_byte.size()) ? char_to_byte[start] : text.size();
        size_t end_byte = (end >= 0 && end < (int)char_to_byte.size()) ? char_to_byte[end] : text.size();

        std::string prev_char = get_prev_char_ja(text, start_byte);
        std::string next_char = get_next_char_ja(text, end_byte);

        // ── Python sync: 上→ウエ/ジョウ ──────────────────────────────────────
        if (surface == "上" && (pos == "NNG" || pos == "MAG" || pos == "NNB")) {
            bool is_prev_hira = prev_char.empty() || is_hiragana_ja(prev_char);
            bool is_prev_kata = (!prev_char.empty() && !is_hiragana_ja(prev_char) && !is_kanji_ja(prev_char) &&
                                 utf8_to_codepoints_ja(prev_char)[0] >= 0x30A0 &&
                                 utf8_to_codepoints_ja(prev_char)[0] <= 0x30FF);
            bool is_prev_kanji = (!prev_char.empty() && is_kanji_ja(prev_char));
            // Python sync: is_prev_sym - 기호 문자 뒤 (예: ウェブサイト上)
            bool is_prev_sym = (!prev_char.empty() && !is_prev_hira && !is_prev_kata && !is_prev_kanji &&
                                !prev_char.empty() && (unsigned char)prev_char[0] >= 0x80);
            bool is_verb_cont = (next_char == "\xE3\x81\x8C" || next_char == "\xE3\x81\x92" ||
                                 next_char == "\xE3\x81\x8E" || next_char == "\xE3\x81\x94" ||
                                 next_char == "\xE3\x81\x90"); // が/げ/ぎ/ご/ぐ
            if (is_prev_hira && !is_kanji_ja(next_char) && !is_verb_cont) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "ウエ"});
            } else if ((is_prev_kata || is_prev_kanji || is_prev_sym) && !is_kanji_ja(next_char)) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "ジョウ"});
            }
        }
        // ── Python sync: 人→ヒト ──────────────────────────────────────────
        else if (surface == "人" && pos == "NNG") {
            bool is_prev_hira = prev_char.empty() || is_hiragana_ja(prev_char);
            bool is_next_josa = (next_char == "\xE3\x81\x8C" || next_char == "\xE3\x81\xAE" || 
                                 next_char == "\xE3\x81\xAF" || next_char == "\xE3\x81\xAB" || 
                                 next_char == "\xE3\x82\x92" || next_char == "\xE3\x82\x82"); // が, の, は, に, を, も
            if (is_prev_hira && is_next_josa) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "ヒト"});
            }
        }
        // ── Python sync: 生→イ/ナマ/セイ ────────────────────────────────────
        else if (surface == "生") {
            bool is_next_hira = is_hiragana_ja(next_char);
            if (pos == "VV" || (pos == "NNG" && is_next_hira && !is_kanji_ja(prev_char))) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "イ"});
            } else if (pos == "NNG") {
                if (!is_kanji_ja(prev_char) && !is_kanji_ja(next_char)) {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "ナマ"});
                } else {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "セイ"});
                }
            } else if (pos == "NR" || pos == "NNP") {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "セイ"});
            }
        } else if (surface == "外") {
            if (pos == "XSN" || is_kanji_ja(prev_char)) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "ガイ"});
            } else if (pos == "NNG" || pos == "MAG") {
                if (!is_kanji_ja(prev_char) && !is_kanji_ja(next_char)) {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "ソト"});
                }
            }
        } else if (surface == "中" && pos == "XSN") {
            if (start > 0) {
                if (prev_char == "日" || prev_char == "月" || prev_char == "年" || prev_char == "週" || 
                    prev_char == "時" || prev_char == "分" || prev_char == "秒" || prev_char == "世" || 
                    prev_char == "代" || prev_char == "夜") {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "ジュウ"});
                } else {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "チュウ"});
                }
            }
        } else if (surface == "数") {
            bool is_prev_kanji = is_kanji_ja(prev_char);
            bool is_prev_foreign = false;
            if (!prev_char.empty()) {
                uint32_t cp = utf8_to_codepoints_ja(prev_char)[0];
                is_prev_foreign = (cp >= 0x30A0 && cp <= 0x30FF) || 
                                  (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z');
            }
            if (pos == "XSN" || is_prev_kanji || is_prev_foreign) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "スウ"});
            }
        } else if (surface == "次" && (pos == "NNG" || pos == "XPN" || pos == "XSN")) {
            bool is_next_kanji = is_kanji_ja(next_char);
            if (is_next_kanji) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "ジ"});
            }
        } else if (surface == "下") {
            if (pos == "XSN" || is_kanji_ja(prev_char)) {
                annotations_copy.push_back({(int)start_byte, (int)end_byte, "カ"});
            } else if (pos == "NNG" || pos == "VV") {
                bool is_next_hira = is_hiragana_ja(next_char);
                if (!is_kanji_ja(prev_char) && !is_next_hira) {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, "した"});
                }
            }
        } else if (surface == "は" && (pos == "JX" || pos == "JKG" || pos == "JC")) {
            annotations_copy.push_back({(int)start_byte, (int)end_byte, "ワ"});
        } else if (surface == "へ" && (pos == "JX" || pos == "JKG" || pos == "JC")) {
            annotations_copy.push_back({(int)start_byte, (int)end_byte, "エ"});
        }
        // ── Python sync: 方 → ホウ/カタ ─────────────────────────────────────
        // た/で/だ + 方 = 조언 표현 (した方がよい) → ホウ
        // 히라가나(のを제외) + 方 = 方法 접미사 (やり方/読み方) → カタ
        else if (surface == u8"方" && pos == "NNG") {
            bool is_prev_hira = !prev_char.empty() && is_hiragana_ja(prev_char);
            bool is_next_kanji = !next_char.empty() && is_kanji_ja(next_char);
            bool is_suggestion = (prev_char == u8"\xE3\x81\x9F" ||   // た
                                  prev_char == u8"\xE3\x81\xA7" ||   // で
                                  prev_char == u8"\xE3\x81\xA0");    // だ
            if (is_prev_hira && !is_next_kanji) {
                if (is_suggestion) {
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, u8"ホウ"});
                } else if (prev_char != u8"\xE3\x81\xAE") {  // not の
                    annotations_copy.push_back({(int)start_byte, (int)end_byte, u8"カタ"});
                }
            }
        }
        // ── Python sync: 似 → ニ (さん似の/ドラマ似の) ───────────────────────
        else if (surface == u8"\u4f3c" && (pos == "VV" || pos == "NNG" || pos == "XSN")) {
            if (next_char == u8"\u306e") {  // の → 似の = ニ
                annotations_copy.push_back({(int)start_byte, (int)end_byte, u8"\u30cb"});
            }
        }
    }

    // ─── 단위 및 수사 결합 룰 선언 ───
    static const std::unordered_map<std::string, std::string> _UNIT_DEFAULT = {
        {"歳", "サイ"}, {"箇", "コ"}, {"个", "コ"}, {"ヶ", "コ"}, {"軒", "ケン"},
        {"階", "カイ"}, {"冊", "サツ"}, {"隻", "セキ"}, {"手", "テ"},
        {"本", "ホン"}, {"回", "カイ"}, {"匹", "ヒキ"}, {"足", "ソク"}, {"杯", "ハイ"}, {"円", "エン"},
        {"箇所", "カショ"}, {"ヶ所", "カショ"}, {"个所", "カショ"},
        {"票", "ヒョウ"}, {"敗", "ハイ"}, {"発", "ハツ"}, {"発着", "ハッチャク"},
        {"発分", "ハツブン"}, {"箱", "ハコ"}, {"針", "ハリ"}, {"袋", "フクロ"},
        {"歩", "ホ"}, {"分", "フン"},
        // Python sync: 割/組/段/位/棟/機/台/散 추가
        {"割", "ワリ"}, {"組", "クミ"}, {"段", "ダン"}, {"位", "イ"},
        {"棟", "トウ"}, {"機", "キ"}, {"台", "ダイ"}, {"散", "サツ"}
    };

    auto ends_with = [](const std::string& str, const std::string& suffix) -> bool {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    auto get_yomi_from_label = [](const std::string& label) -> std::string {
        size_t pos = label.rfind(':');
        if (pos != std::string::npos) {
            bool is_digit = true;
            for (size_t i = pos + 1; i < label.size(); ++i) {
                if (label[i] < '0' || label[i] > '9') {
                    is_digit = false;
                    break;
                }
            }
            if (is_digit && pos + 1 < label.size()) {
                return label.substr(0, pos);
            }
        }
        return label;
    };

    auto is_kanji_num = [](const std::string& surface) -> bool {
        if (surface.empty()) return false;
        bool is_digit = true;
        for (char c : surface) {
            if (c < '0' || c > '9') {
                is_digit = false;
                break;
            }
        }
        if (is_digit) return true;
        std::vector<std::string> target_kanji = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "百", "千", "万"};
        size_t offset = 0;
        while (offset < surface.size()) {
            size_t len = 1;
            unsigned char c = surface[offset];
            if (c >= 0xFC) len = 6;
            else if (c >= 0xF8) len = 5;
            else if (c >= 0xF0) len = 4;
            else if (c >= 0xE0) len = 3;
            else if (c >= 0xC0) len = 2;
            if (offset + len > surface.size()) return false;
            std::string char_str = surface.substr(offset, len);
            bool found = false;
            for (const auto& tk : target_kanji) {
                if (char_str == tk) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
            offset += len;
        }
        return true;
    };

    auto has_numeric_char = [](const std::string& surface) -> bool {
        for (char c : surface) {
            if (c >= '0' && c <= '9') return true;
        }
        std::vector<std::string> target_kanji = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", "百", "千", "万"};
        for (const auto& tk : target_kanji) {
            if (surface.find(tk) != std::string::npos) return true;
        }
        return false;
    };

    auto get_num_kata = [this, is_kanji_num](const std::string& num_surf) -> std::string {
        bool is_digit = true;
        for (char c : num_surf) {
            if (c < '0' || c > '9') {
                is_digit = false;
                break;
            }
        }
        std::string kanji;
        if (is_digit && !num_surf.empty()) {
            try {
                kanji = this->int_to_ja(std::stoll(num_surf));
            } catch (...) {
                kanji = num_surf;
            }
        } else {
            kanji = num_surf;
        }

        std::unordered_map<std::string, std::string> mapping = {
            {"一", "イチ"}, {"二", "ニ"}, {"三", "サン"}, {"四", "ヨン"}, {"五", "ゴ"},
            {"六", "ロク"}, {"七", "ナナ"}, {"八", "ハチ"}, {"九", "キュウ"},
            {"十", "ジュウ"}, {"百", "ヒャク"}, {"千", "セン"}, {"万", "マン"}
        };

        std::string res = "";
        size_t offset = 0;
        while (offset < kanji.size()) {
            size_t len = 1;
            unsigned char c = kanji[offset];
            if (c >= 0xFC) len = 6;
            else if (c >= 0xF8) len = 5;
            else if (c >= 0xF0) len = 4;
            else if (c >= 0xE0) len = 3;
            else if (c >= 0xC0) len = 2;
            if (offset + len > kanji.size()) break;
            std::string ch = kanji.substr(offset, len);
            auto it = mapping.find(ch);
            if (it != mapping.end()) {
                res += it->second;
            } else {
                res += ch;
            }
            offset += len;
        }

        auto replace_all = [](std::string& str, const std::string& from, const std::string& to) {
            size_t start_pos = 0;
            while((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        };
        replace_all(res, "サンヒャク", "サンビャク");
        replace_all(res, "ロクヒャク", "ロッピャク");
        replace_all(res, "ハ치ヒャク", "ハッピャク"); // 혹시 오타 방어
        replace_all(res, "ハチヒャク", "ハッピャク");
        replace_all(res, "イチセン", "イッセン");
        replace_all(res, "サンセン", "サンゼン");
        replace_all(res, "ハ치セン", "ハッセン");
        replace_all(res, "ハチセン", "ハッセン");

        return res;
    };

    // 인접 형태소 간 결합 룰
    if (merged_morphemes.size() > 1) {
        for (size_t k = 0; k < merged_morphemes.size() - 1; k++) {
            const auto& m1 = merged_morphemes[k];
            const auto& m2 = merged_morphemes[k+1];
            if (m2.start == m1.end) {
                size_t m1_start_byte = (m1.start >= 0 && m1.start < (int)char_to_byte.size()) ? char_to_byte[m1.start] : text.size();
                size_t m1_end_byte = (m1.end >= 0 && m1.end < (int)char_to_byte.size()) ? char_to_byte[m1.end] : text.size();
                size_t m2_start_byte = (m2.start >= 0 && m2.start < (int)char_to_byte.size()) ? char_to_byte[m2.start] : text.size();

                std::string num_surf = "";
                for (size_t i = 0; i < m1.surface.size(); ) {
                    size_t len = 1;
                    unsigned char c = m1.surface[i];
                    if (c >= 0xFC) len = 6;
                    else if (c >= 0xF8) len = 5;
                    else if (c >= 0xF0) len = 4;
                    else if (c >= 0xE0) len = 3;
                    else if (c >= 0xC0) len = 2;
                    if (i + len > m1.surface.size()) break;
                    std::string ch = m1.surface.substr(i, len);
                    if ((ch >= "0" && ch <= "9") || is_kanji_num(ch)) {
                        num_surf += ch;
                    }
                    i += len;
                }

                bool is_num = !num_surf.empty() && (m1.pos == "NR" || is_kanji_num(num_surf));

                if (is_num && (m2.pos == "XSN" || m2.pos == "NNG" || m2.pos == "NR" || m2.pos == "NNP")) {
                    if (_UNIT_DEFAULT.find(m2.surface) != _UNIT_DEFAULT.end()) {
                        std::string m1_anno = "";
                        for (const auto& ann : annotations) {
                            if (std::get<0>(ann) == (int)m1_start_byte && std::get<1>(ann) == (int)m1_end_byte) {
                                m1_anno = std::get<2>(ann);
                                break;
                            }
                        }

                        std::string m2_anno = "";
                        for (const auto& ann : annotations) {
                            if (std::get<0>(ann) == (int)m2_start_byte && std::get<1>(ann) == (int)char_to_byte[m2.end]) {
                                m2_anno = std::get<2>(ann);
                                break;
                            }
                        }

                        std::string num_kata = "";
                        if (!m1_anno.empty()) {
                            num_kata = PhonologyJa::hira2kata(get_yomi_from_label(m1_anno));
                        } else {
                            num_kata = get_num_kata(num_surf);
                        }

                        if (m2.surface == "人") {
                            std::string suffix_surf = "";
                            std::string suffix_kata = "";
                            size_t suffix_end_char = m2.end;
                            if (k + 2 < merged_morphemes.size()) {
                                const auto& m3 = merged_morphemes[k+2];
                                if (m3.start == m2.end && (m3.surface == "分" || m3.surface == "間" || m3.surface == "目")) {
                                    suffix_surf = m3.surface;
                                    suffix_end_char = m3.end;
                                    if (suffix_surf == "分") suffix_kata = "ブン";
                                    else if (suffix_surf == "間") suffix_kata = "カン";
                                    else if (suffix_surf == "目") suffix_kata = "メ";
                                }
                            }
                            size_t suffix_end_byte = (suffix_end_char >= 0 && suffix_end_char < char_to_byte.size()) ? char_to_byte[suffix_end_char] : text.size();

                            if (num_surf == "1" || num_surf == "一") {
                                annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, "ヒト"});
                                annotations_copy.push_back({(int)m2_start_byte, (int)suffix_end_byte, "リ" + suffix_kata});
                            } else if (num_surf == "2" || num_surf == "二") {
                                annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, "フ타"}); // "フタ"는 6바이트
                                annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, "フタ"});
                                annotations_copy.push_back({(int)m2_start_byte, (int)suffix_end_byte, "リ" + suffix_kata});
                            } else if (num_surf == "4" || num_surf == "四") {
                                annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, "ヨ"});
                                annotations_copy.push_back({(int)m2_start_byte, (int)suffix_end_byte, "ニン" + suffix_kata});
                            } else {
                                if (num_kata.size() >= 6 && ends_with(num_kata, "ヨン")) {
                                    num_kata = num_kata.substr(0, num_kata.size() - 6) + "ヨ";
                                }
                                annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, num_kata});
                                annotations_copy.push_back({(int)m2_start_byte, (int)suffix_end_byte, "ニン" + suffix_kata});
                            }
                        } else {
                            std::string unit_kata = "";
                            if (!m2_anno.empty()) {
                                unit_kata = PhonologyJa::hira2kata(get_yomi_from_label(m2_anno));
                            } else {
                                auto uit = _UNIT_DEFAULT.find(m2.surface);
                                unit_kata = (uit != _UNIT_DEFAULT.end()) ? uit->second : "";
                            }

                            // 자음군 기반 연음(Liaison) - JSON 테이블 기반 (Python apply_liaison() 동일)
                            auto [ln_kata, lu_kata] = apply_liaison(num_kata, unit_kata, m2.surface);
                            num_kata = ln_kata; unit_kata = lu_kata;

                            auto it = _COUNTER_RULES.find(m2.surface);
                            if (it != _COUNTER_RULES.end()) {
                                auto rit = it->second.find(num_surf);
                                if (rit != it->second.end()) {
                                    num_kata = rit->second.first;
                                    unit_kata = rit->second.second;
                                }
                            }

                            std::string suffix_surf = "";
                            std::string suffix_kata = "";
                            size_t suffix_end_char = m2.end;
                            if (k + 2 < merged_morphemes.size()) {
                                const auto& m3 = merged_morphemes[k+2];
                                if (m3.start == m2.end && (m3.surface == "分" || m3.surface == "間" || m3.surface == "目")) {
                                    suffix_surf = m3.surface;
                                    suffix_end_char = m3.end;
                                    if (suffix_surf == "分") suffix_kata = "ブン";
                                    else if (suffix_surf == "間") suffix_kata = "カン";
                                    else if (suffix_surf == "目") suffix_kata = "メ";
                                }
                            }
                            size_t suffix_end_byte = (suffix_end_char >= 0 && suffix_end_char < char_to_byte.size()) ? char_to_byte[suffix_end_char] : text.size();

                            annotations_copy.push_back({(int)m1_start_byte, (int)m1_end_byte, num_kata});
                            annotations_copy.push_back({(int)m2_start_byte, (int)suffix_end_byte, unit_kata + suffix_kata});
                        }
                    }
                }
                else if ((m1.pos == "NNG" || m1.pos == "NNP") && (m2.pos == "NNG" || m2.pos == "XSN")) {
                    size_t m2_start_byte = (m2.start >= 0 && m2.start < (int)char_to_byte.size()) ? char_to_byte[m2.start] : text.size();
                    auto it = _RENDAKU_RULES.find(m2.surface);
                    if (it != _RENDAKU_RULES.end()) {
                        // m1.start에서 더 긴 kanji_dict 복합어(m2 포함)가 있으면 rendaku skip
                        bool has_compound = false;
                        int m1_char_start = m1.start;
                        int m2_char_end = m2.end;
                        int span_len = m2_char_end - m1_char_start;
                        int n_chars_total = (int)char_to_byte.size() - 1;  // 마지막은 sentinel
                        for (int clen = std::min(10, n_chars_total - m1_char_start); clen > span_len - 1; --clen) {
                            if (m1_char_start + clen > n_chars_total) continue;
                            size_t s_byte = char_to_byte[m1_char_start];
                            size_t e_byte = char_to_byte[m1_char_start + clen];
                            std::string cand = text.substr(s_byte, e_byte - s_byte);
                            if (kanji_dict_.count(cand) && !kanji_dict_.at(cand).empty()) {
                                has_compound = true;
                                break;
                            }
                        }
                        if (!has_compound) {
                            annotations_copy.push_back({(int)m2_start_byte, (int)char_to_byte[m2.end], it->second});
                        }
                    }
                }
            }
        }
    }
    std::vector<std::pair<int, int>> call_overrides;
    for (size_t i = original_annotations_size; i < annotations_copy.size(); ++i) {
        call_overrides.push_back({std::get<0>(annotations_copy[i]), std::get<1>(annotations_copy[i])});
    }

    // ── 고우선순위 패치: BERT call_overrides를 덮어쓰는 compound-word 패치 ───────
    // sorted_patches_(JSON 패치)는 call_overrides에 막히므로, 핵심 복합어들을
    // 여기서 직접 어노테이션으로 추가하여 BERT 어노테이션을 override함.
    static const std::vector<std::pair<std::string, std::string>> HIGH_PRIORITY_PATCHES = {
        {u8"野生生物", u8"ヤセイセイブツ"},
        {u8"実は",     u8"ジツワ"},
        {u8"システム外", u8"システムガイ"},
        {u8"一切",     u8"イッサイ"},
        {u8"二次会",   u8"ニジカイ"},
        {u8"特急中",   u8"トッキュウチュウ"},
        {u8"手下",     u8"テシタ"},
        {u8"ビー玉",   u8"ビーダマ"},
        {u8"下手",     u8"ヘタ"},
        {u8"生み出す", u8"ウミダス"},
        {u8"生み出",   u8"ウミダ"},
        {u8"生物",     u8"セイブツ"},
        {u8"屋外型",   u8"オクガイガタ"},
        {u8"先生",     u8"センセイ"},
        {u8"下士官",   u8"カシカン"},
        {u8"巡視船",   u8"ジュンシセン"},
        {u8"係争中",   u8"ケイソウチュウ"},
        {u8"在り方",   u8"アリカタ"},
        {u8"外敵",     u8"ガイテキ"},
        {u8"崩壊後",   u8"ホウカイゴ"},
        {u8"廃止後",   u8"ハイシゴ"},
        {u8"米議会",   u8"ベイギカイ"},
        {u8"米景気",   u8"ベイケイキ"},
        {u8"米国",     u8"ベイコク"},
        {u8"シャボン玉", u8"シャボンダマ"},
        {u8"惚れ薬",   u8"ホレグスリ"},
        {u8"陰に",     u8"カゲニ"},
        {u8"執念深く", u8"シュウネンブカク"},
        {u8"後の軌道", u8"アトノキドウ"},
        // 8차: 남은 C++ OJT wins 8건
        {u8"三國連太郎", u8"ミクニレンタロウ"},   // 三國連太郎さん (고유명사)
        {u8"ポルトガル人", u8"ポルトガルジン"},    // ポルトガル人 (ニン→ジン)
        {u8"人との",     u8"ヒトトノ"},            // 地元の人との交流 (ニン→ヒト)
        {u8"人が活発", u8"ヒトガカッパツ"},       // 人が活発に (ニンガカッパツ→ヒトガカッパツ)
        {u8"人が移",   u8"ヒトガイ"},             // 人が移動

        {u8"没入型",     u8"ボツニュウガタ"},      // 没入型3次元
        {u8"動力型",     u8"ドウリョクガタ"},      // 通常動力型
        {u8"ネコミミ型", u8"ネコミミガタ"},        // ネコミミ型ヘッドセット
        // 9차: 남은 6건
        {u8"発行",       u8"ハッコウ"},            // 一切発行 (パツギョウ→ハッコウ)
        {u8"活発",       u8"カッパツ"},            // 活発に (カツハツ→カッパツ)
        {u8"8基",        u8"ハッキ"},              // 8基搭載 (ハチモト→ハッキ)
        {u8"１基",       u8"イッキ"},              // 1基あたり
        {u8"1基",        u8"イッキ"},              // 1基あたり
        // 数字+分 (分読み: サンプン/ヨンプン)
        // 7万 → ナナマン (シチマン→ナナマン)
        {u8"7万",        u8"ナナマン"},             // 7万5000人 (シチマン→ナナマン)
        {u8"7億",        u8"ナナオク"},             // 7億 (シチオク→ナナオク)
        {u8"3分",        u8"サンプン"},            // 3分 (サンブン→サンプン)
        {u8"３分",       u8"サンプン"},            // ３分
        {u8"4分",        u8"ヨンプン"},            // 4分
        {u8"４分",       u8"ヨンプン"},            // ４分
        // 数字+人 복합어 (ヒト→ニン)
        {u8"5000人",     u8"ゴセンニン"},          // 5000人程度
        {u8"1000人",     u8"センニン"},             // 1000人
        {u8"万人",       u8"マンニン"},             // 万人
        {u8"人程度",     u8"ニンテイド"},           // 人程度 (ヒトテイド→ニンテイド)
        // Python sync: した方が/より方が → ホウ (suggestion context)
        {u8"した方が",   u8"シタホウガ"},            // 確認した方が / 気にした方が
        {u8"より方が",   u8"ヨリホウガ"},            // より方がよい
        // Python sync: 同馬/当該馬 → マ (not バ)
        {u8"同馬",       u8"ドウマ"},                // 同馬の持つ
        {u8"当該馬",     u8"トウガイマ"},             // 当該馬
        // Python sync: 2人の間/二人の間 → フタリノアイダ
        {u8"2\u4EBA\u306E\u9593",    u8"\u30D5\u30BF\u30EA\u30CE\u30A2\u30A4\u30C0"},  // 2人の間
        {u8"\u4E8C\u4EBA\u306E\u9593",u8"\u30D5\u30BF\u30EA\u30CE\u30A2\u30A4\u30C0"},  // 二人の間
        // ── Batch 9: Python sync (生育/芸術祭) ─────────────────────────────────
        // 生育地/生育 → セイクチ/セイク (BERT=セイイク → norm후 セエイク≠Gold セエク)
        {u8"\u751F\u80B2\u5730",      u8"\u30BB\u30A4\u30AF\u30C1"},       // 生育地→セイクチ
        {u8"\u751F\u80B2",            u8"\u30BB\u30A4\u30AF"},             // 生育→セイク
        // 祭 suffix compounds (BERT splits 芸+術+祭 individually)
        {u8"\u56FD\u969B\u6620\u753B\u796D", u8"\u30B3\u30AF\u30B5\u30A4\u30A8\u30A4\u30AC\u30B5\u30A4"}, // 国際映画祭
        {u8"\u56FD\u969B\u97F3\u697D\u796D", u8"\u30B3\u30AF\u30B5\u30A4\u30AA\u30F3\u30AC\u30AF\u30B5\u30A4"}, // 国際音楽祭
        {u8"\u82B8\u8853\u796D",      u8"\u30B2\u30A4\u30B8\u30E5\u30C4\u30B5\u30A4"}, // 芸術祭→ゲイジュツサイ
        // ── Batch 10: counter-loop guard (Python fix sync) ────────────────────────
        // 数+{千/百/万/十} compounds: counter loop sets 千→セン before replace_all_ja
        // can protect it. HP takes priority over counter annotations (removes them).
        {u8"\u6570\u5343", u8"\u30B9\u30A6\u30BB\u30F3"},    // 数千→スウセン
        {u8"\u6570\u767E", u8"\u30B9\u30A6\u30D2\u30E3\u30AF"}, // 数百→スウヒャク
        {u8"\u6570\u4E07", u8"\u30B9\u30A6\u30DE\u30F3"},    // 数万→スウマン
        {u8"\u6570\u5341", u8"\u30B9\u30A6\u30B8\u30E5\u30A6"}, // 数十→スウジュウ
    };



    for (const auto& [hp_key, hp_val] : HIGH_PRIORITY_PATCHES) {
        size_t hp_pos = 0;
        while ((hp_pos = text.find(hp_key, hp_pos)) != std::string::npos) {
            int s_byte = static_cast<int>(hp_pos);
            int e_byte = static_cast<int>(hp_pos + hp_key.size());
            // 더 긴 패치가 이미 이 범위를 커버하면 스킵 (더 짧은 패치가 더 긴 패치를 덮어쓰지 않도록)
            bool covered_by_longer = false;
            for (const auto& c_over : call_overrides) {
                if (c_over.first <= s_byte && e_byte <= c_over.second &&
                    (c_over.second - c_over.first) > (e_byte - s_byte)) {
                    covered_by_longer = true;
                    break;
                }
            }
            if (covered_by_longer) {
                hp_pos = static_cast<size_t>(e_byte);
                continue;
            }
            // 기존 어노테이션 제거 (덮어쓰기)
            auto it2 = annotations_copy.begin();
            while (it2 != annotations_copy.end()) {
                if (std::get<0>(*it2) < e_byte && s_byte < std::get<1>(*it2))
                    it2 = annotations_copy.erase(it2);
                else ++it2;
            }
            annotations_copy.push_back({s_byte, e_byte, hp_val});
            // call_overrides에도 추가하여 JSON 패치가 덮어쓰지 않도록 보호
            call_overrides.push_back({s_byte, e_byte});
            hp_pos = static_cast<size_t>(e_byte);
        }
    }

    for (const auto& patch_key : sorted_patches_) {
        size_t pos = 0;
        while ((pos = text.find(patch_key, pos)) != std::string::npos) {
            int start_byte = static_cast<int>(pos);
            int end_byte = static_cast<int>(pos + patch_key.size());

            bool overlap_call = false;
            for (const auto& c_over : call_overrides) {
                if (c_over.first < end_byte && start_byte < c_over.second) {
                    overlap_call = true;
                    break;
                }
            }
            if (overlap_call) {
                pos = end_byte;
                continue;
            }

            // 겹치는 기존 neural annotations 제거
            auto it = annotations_copy.begin();
            while (it != annotations_copy.end()) {
                int ann_start = std::get<0>(*it);
                int ann_end = std::get<1>(*it);
                if (start_byte < ann_end && ann_start < end_byte) {
                    it = annotations_copy.erase(it);
                } else {
                    ++it;
                }
            }

            // 신규 사전 패치 어노테이션 추가
            std::string reading = _DICT_PATCHES[patch_key][0];
            annotations_copy.push_back({start_byte, end_byte, reading});

            pos = end_byte; // 다음 검색 위치로 이동
        }
    }

    // Apply overrides in reverse index order
    std::vector<std::tuple<int, int, std::string>> sorted_anns = annotations_copy;
    std::sort(sorted_anns.begin(), sorted_anns.end(), [](const auto& a, const auto& b) {
        return std::get<0>(a) > std::get<0>(b);
    });

    for (const auto& [start, end, label] : sorted_anns) {
        if (start < (int)processed_text.size() && end <= (int)processed_text.size()) {
            std::string span = processed_text.substr(start, end - start);
            if (span.empty()) continue;

            // Check overlap with _DICT_PATCHES
            bool is_patched = false;
            for (const auto& [patch_key, _] : _DICT_PATCHES) {
                size_t pos = 0;
                while ((pos = processed_text.find(patch_key, pos)) != std::string::npos) {
                    if (pos <= (size_t)start && (size_t)end <= (pos + patch_key.size())) {
                        is_patched = true;
                        break;
                    }
                    pos += patch_key.size();
                }
                if (is_patched) break;
            }
            
            bool in_call_overrides = false;
            for (const auto& c_over : call_overrides) {
                if (c_over.first == start && c_over.second == end) {
                    in_call_overrides = true;
                    break;
                }
            }

            if (!in_call_overrides) {
                if (is_patched) continue;

                // Skip single-char annotation if a kanji_dict compound (2+ chars) starts here
                auto span_cps = utf8_to_codepoints_ja(span);
                if (span_cps.size() == 1) {
                    bool has_compound = false;
                    for (size_t clen = 10; clen > 1; --clen) {
                        std::string remaining = processed_text.substr(start);
                        auto rem_cps = utf8_to_codepoints_ja(remaining);
                        if (clen > rem_cps.size()) continue;
                        std::string cand = codepoints_to_utf8_ja(
                            std::vector<uint32_t>(rem_cps.begin(), rem_cps.begin() + clen)
                        );
                        if (kanji_dict_.count(cand) && !kanji_dict_.at(cand).empty()) {
                            has_compound = true;
                            break;
                        }
                    }
                    if (has_compound) continue;
                }
            }

            std::string yomi_part = label;
            if (label.find(':') != std::string::npos) {
                size_t colon_pos = label.rfind(':');
                std::string accent_str = label.substr(colon_pos + 1);
                bool is_digit = !accent_str.empty() && std::all_of(accent_str.begin(), accent_str.end(), ::isdigit);
                if (is_digit) {
                    yomi_part = label.substr(0, colon_pos);
                }
            }
            std::string yomi_kata = hira2kata(yomi_part);

            auto ta_it = targets_accent_.find(span);
            if (ta_it != targets_accent_.end()) {
                auto acc_it = ta_it->second.find(yomi_kata);
                if (acc_it != ta_it->second.end()) {
                    out_accent_overrides[span] = acc_it->second;
                }
            }

            processed_text.replace(start, end - start, yomi_kata);
        }
    }

    // 1.5. 영어 단어(custom_eng_dict_ja.json) 오버라이드 전처리 (바이트 오프셋 기반)
    if (!_ENG_DICT.empty()) {
        try {
            std::regex eng_pat("[a-zA-Z]+");
            auto words_begin = std::sregex_iterator(processed_text.begin(), processed_text.end(), eng_pat);
            auto words_end = std::sregex_iterator();
            std::vector<std::pair<size_t, size_t>> matches;
            for (std::sregex_iterator it = words_begin; it != words_end; ++it) {
                matches.push_back({it->position(), it->position() + it->length()});
            }
            // 뒤에서부터 치환하여 인덱스 안 밀리게 처리
            std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });
            for (const auto& m : matches) {
                std::string word = processed_text.substr(m.first, m.second - m.first);
                std::string word_upper = word;
                for (auto& c : word_upper) c = toupper(c);
                auto it = _ENG_DICT.find(word_upper);
                if (it != _ENG_DICT.end()) {
                    processed_text.replace(m.first, m.second - m.first, it->second);
                    // 겹치는 어노테이션 제거
                    int start_byte = static_cast<int>(m.first);
                    int end_byte = static_cast<int>(m.second);
                    auto ann_it = annotations_copy.begin();
                    while (ann_it != annotations_copy.end()) {
                        int ann_start = std::get<0>(*ann_it);
                        int ann_end = std::get<1>(*ann_it);
                        if (start_byte < ann_end && ann_start < end_byte) {
                            ann_it = annotations_copy.erase(ann_it);
                        } else {
                            ++ann_it;
                        }
                    }
                }
            }
        } catch (...) {}
    }



    // (Annotation override applied above before length shifts)

    // ── Phase 4-0: Pre-processing before symbol substitution ──────────────────
    // 4-0a. Known English brand names / units (prevent letter-by-letter splitting)
    processed_text = replace_all_ja(processed_text, "Wi-Fi", u8"\u30EF\u30A4\u30D5\u30A1\u30A4");   // ワイファイ
    processed_text = replace_all_ja(processed_text, "wi-fi", u8"\u30EF\u30A4\u30D5\u30A1\u30A4");
    processed_text = replace_all_ja(processed_text, "WiFi",  u8"\u30EF\u30A4\u30D5\u30A1\u30A4");
    processed_text = replace_all_ja(processed_text, "AT-X",  u8"\u30A8\u30FC\u30C6\u30A3\u30FC\u30A8\u30C3\u30AF\u30B9"); // エーティーエックス
    processed_text = replace_all_ja(processed_text, "PC/AT", u8"\u30D4\u30FC\u30B7\u30FC\u30A8\u30FC\u30C6\u30A3\u30FC"); // ピーシーエーティー
    // 4-0a2. 単位: mol → モル
    {
        std::regex re_mol(R"((\d)mol)");
        processed_text = std::regex_replace(processed_text, re_mol, u8"$1\u30E2\u30EB"); // Nモル
    }
    // 4-0a3. 生ら → せえら (接尾辞: ビギン生ら)
    processed_text = replace_all_ja(processed_text, u8"\u751F\u3089", u8"\u305B\u3048\u3089"); // 生ら→せえら
    // 4-0b. 숫자 범위 하이픈: 3-4日 → 3から4日 (マイナス 삽입 방지)
    {
        static const std::string range_units = u8"\u65E5\u6642\u5206\u6708\u5E74\u9031\u56DE\u500B\u672C\u4EBA\u5339\u968E\u6B73\u676F";
        // build char-class string for regex: [日時分月年週回個本人匹階歳杯]
        std::string new_text;
        std::regex re_range(u8"(\\d+)-(?=\\d+[\u65E5\u6642\u5206\u6708\u5E74\u9031\u56DE\u500B\u672C\u4EBA\u5339\u968E\u6B73\u676F])");
        new_text = std::regex_replace(processed_text, re_range, u8"$1\u304B\u3089"); // Nから
        processed_text = new_text;
    }
    // 4-0d. 4年 special reading (before counter loop)
    processed_text = replace_all_ja(processed_text, u8"4\u5E74\u751F", u8"\u30E8\u30CD\u30F3\u30BB\u30A4"); // 4年生→ヨネンセイ
    processed_text = replace_all_ja(processed_text, u8"4\u5E74\u9593", u8"\u30E8\u30CD\u30F3\u30AB\u30F3"); // 4年間→ヨネンカン
    processed_text = replace_all_ja(processed_text, u8"4\u5E74\u5236", u8"\u30E8\u30CD\u30F3\u30BB\u30A4"); // 4年制→ヨネンセイ
    processed_text = replace_all_ja(processed_text, u8"\u56DB\u5E74\u751F", u8"\u30E8\u30CD\u30F3\u30BB\u30A4"); // 四年生
    processed_text = replace_all_ja(processed_text, u8"\u56DB\u5E74\u9593", u8"\u30E8\u30CD\u30F3\u30AB\u30F3"); // 四年間
    // 4-0e. N着 counter: C++ counter loop handles 着 via _UNIT_DEFAULT (no explicit replace needed here)
    // 4-0f. 間 reading: time period → かん, の間[はがでにも] → のあいだ
    {
        // N秒/分/時/日/週/月/年 + 間 → 間=かん
        std::regex re_kan(u8"([0-9\u4E00-\u4E5D\u5341\u767E\u5343\u4E07]+(?:\u79D2|\u5206|\u6642|\u65E5|\u9031|\u6708|\u5E74|\u30F6\u6708|\u304B\u6708|\u30B1\u6708))\u9593");
        processed_text = std::regex_replace(processed_text, re_kan, u8"$1\u304B\u3093"); // N+unit+かん
    }
    {
        // の間[は/が/で/に/も] → のあいだ
        std::regex re_aida(u8"(\u306E)\u9593(?=[\u306F\u304C\u3067\u306B\u3082])");
        processed_text = std::regex_replace(processed_text, re_aida, u8"$1\u3042\u3044\u3060"); // のあいだ
    }

    // 1. Symbol preprocessing
    processed_text = preprocess_symbols(processed_text);



    // 2. NFKC normalization
    processed_text = nfkc_normalize_ja(processed_text);


    // 2.4. 第N話 counter: 話→ワ (replace before _normalize_punct strips digits)
    // Python sync: re.sub(r'第(\d+)話', lambda m: 'ダイ' + _get_num_kata(n) + 'ワ', text)
    {
        std::string result;
        result.reserve(processed_text.size());
        const std::string dai_u8 = u8"第";  // E7 AC AC (3 bytes)
        const std::string wa_u8  = u8"話";  // E8 A9 B1 (3 bytes)
        size_t i = 0;
        while (i < processed_text.size()) {
            // Check for 第
            if (i + 3 <= processed_text.size() && processed_text.substr(i, 3) == dai_u8) {
                size_t j = i + 3;
                std::string digits;
                while (j < processed_text.size() && processed_text[j] >= '0' && processed_text[j] <= '9') {
                    digits += processed_text[j++];
                }
                if (!digits.empty() && j + 3 <= processed_text.size() && processed_text.substr(j, 3) == wa_u8) {
                    // 第N話 found → convert to ダイ + num_kata + ワ
                    result += u8"ダイ";
                    result += get_num_kata(digits);
                    result += u8"ワ";
                    i = j + 3;  // skip 第 + digits + 話
                    continue;
                }
            }
            // UTF-8: copy one code unit
            unsigned char c = (unsigned char)processed_text[i];
            size_t char_len = 1;
            if (c >= 0xFC) char_len = 6;
            else if (c >= 0xF8) char_len = 5;
            else if (c >= 0xF0) char_len = 4;
            else if (c >= 0xE0) char_len = 3;
            else if (c >= 0xC0) char_len = 2;
            if (i + char_len > processed_text.size()) char_len = 1;
            result += processed_text.substr(i, char_len);
            i += char_len;
        }
        processed_text = result;
    }

    // 2.5. Datetime and numbers preprocessing
    processed_text = replace_datetime_and_numbers(processed_text);


    // 2.6. Custom replacements and semantic fixes
    processed_text = replace_all_ja(processed_text, "You Tube", "ユーチューブ");
    processed_text = replace_all_ja(processed_text, "YouTube", "ユーチューブ");
    processed_text = replace_all_ja(processed_text, "Taco", "タコ");
    processed_text = replace_all_ja(processed_text, "活動を行っていた", "活動をおこなっていた");
    processed_text = replace_all_ja(processed_text, "活動を行った", "活動をおこなった");
    processed_text = replace_all_ja(processed_text, "㎝〜", "㎝");
    processed_text = replace_all_ja(processed_text, "センチ〜", "センチ");
    processed_text = replace_all_ja(processed_text, "センチメートル〜", "センチメートル");
    processed_text = replace_all_ja(processed_text, "〜", "から");
    processed_text = replace_all_ja(processed_text, "~", "から");
    processed_text = replace_all_ja(processed_text, "다른클럽", "タノクラブ");
    processed_text = replace_all_ja(processed_text, "はず", "ハズ");
    if (processed_text == "水") {
        processed_text = "スイ";
    }
    processed_text = replace_all_ja(processed_text, "8センチ", "ハッセンチ");
    processed_text = replace_all_ja(processed_text, "８センチ", "ハッセンチ");
    processed_text = replace_all_ja(processed_text, "8畳", "ハチジョウ");
    processed_text = replace_all_ja(processed_text, "８畳", "ハチジョウ");
    processed_text = replace_all_ja(processed_text, "4月頃", "シガツゴロ");
    processed_text = replace_all_ja(processed_text, "４月頃", "シガツゴロ");
    processed_text = replace_all_ja(processed_text, "8月頃", "ハチガツゴロ");
    processed_text = replace_all_ja(processed_text, "８月頃", "ハチガツゴロ");
    processed_text = replace_all_ja(processed_text, "4、5日", "ヨンゴニチ");
    processed_text = replace_all_ja(processed_text, "４、５日", "ヨンゴニチ");
    processed_text = replace_all_ja(processed_text, "㎝", "センチメートル");
    processed_text = replace_all_ja(processed_text, "\u339d", "センチメートル");
    processed_text = replace_all_ja(processed_text, "cm", "センチメートル");
    processed_text = replace_all_ja(processed_text, "ｃｍ", "センチメートル");
    processed_text = replace_all_ja(processed_text, "30周年", "サンジュッシュウネン");
    processed_text = replace_all_ja(processed_text, "３０周年", "サンジュッシュウネン");
    processed_text = replace_all_ja(processed_text, "0430周年", "ヨッカサンジュッシュウネン");
    processed_text = replace_all_ja(processed_text, "リゾート張り", "リゾートばり");
    processed_text = replace_all_ja(processed_text, "沿いには", "ぞいにわ");
    processed_text = replace_all_ja(processed_text, "沿い에는", "ぞいにわ");
    processed_text = replace_all_ja(processed_text, "年代は", "ねんだいわ");
    processed_text = replace_all_ja(processed_text, "年代의", "ねんだいの");
    processed_text = replace_all_ja(processed_text, "年代の", "ねんだいの");
    processed_text = replace_all_ja(processed_text, "年代", "ねんだい");
    processed_text = replace_all_ja(processed_text, "役立つ", "やくだつ");
    processed_text = replace_all_ja(processed_text, "役立ち", "やくだち");
    processed_text = replace_all_ja(processed_text, "役立て", "やくだて");
    processed_text = replace_all_ja(processed_text, "主義者", "しゅぎしゃ");
    processed_text = replace_all_ja(processed_text, "주의者", "しゅぎしゃ");
    processed_text = replace_all_ja(processed_text, "인기", "in기");
    processed_text = replace_all_ja(processed_text, "相談会", "そうだんかい");
    processed_text = replace_all_ja(processed_text, "トートバッグ", "トートバック");
    processed_text = replace_all_ja(processed_text, "頃に", "ころに");
    processed_text = replace_all_ja(processed_text, "道標", "みちしるべ");
    processed_text = replace_all_ja(processed_text, "行っています", "おこなっています");
    processed_text = replace_all_ja(processed_text, "行っております", "おこなっております");
    processed_text = replace_all_ja(processed_text, "を行っている", "をおこなっている");
    processed_text = replace_all_ja(processed_text, "を行った", "をおこなった");
    processed_text = replace_all_ja(processed_text, "をはじめ", "をハジメ");
    processed_text = replace_all_ja(processed_text, "その他の", "そのたの");
    processed_text = replace_all_ja(processed_text, "その他", "そのほか");
    // ── Python sync: ヴ→ブ/バ/ビ/ベ/ボ 일괄 변환 ─────────────────────────────
    // 순서 중요: 복합형(ヴァ,ヴィ,ヴェ,ヴォ)을 단순형(ヴ)보다 먼저 처리
    processed_text = replace_all_ja(processed_text, u8"ヴァ", u8"バ"); // ヴァ→バ
    processed_text = replace_all_ja(processed_text, u8"ヴィ", u8"ビ"); // ヴィ→ビ
    processed_text = replace_all_ja(processed_text, u8"ヴェ", u8"ベ"); // ヴェ→ベ
    processed_text = replace_all_ja(processed_text, u8"ヴォ", u8"ボ"); // ヴォ→ボ
    processed_text = replace_all_ja(processed_text, u8"ヴ",   u8"ブ"); // ヴ→ブ
    // 특례: 일부 단어는 ヴ→ブ 후에도 추가 보정 필요
    processed_text = replace_all_ja(processed_text, u8"エバンゲリオン", u8"エアンゲリオン"); // エヴァ→エバ→エア
    processed_text = replace_all_ja(processed_text, u8"ブッパータール", u8"ウッパータール"); // ヴッパータール→ブッ→ウッ
    processed_text = replace_all_ja(processed_text, "アルカリ泉", "アルカリセン");
    processed_text = replace_all_ja(processed_text, "알칼리泉", "アルカリセン");
    processed_text = replace_all_ja(processed_text, "알칼리샘", "アルカリセン");
    processed_text = replace_all_ja(processed_text, "本化別頭仏祖統記", "ホンケベットウブッソトウキ");
    processed_text = replace_all_ja(processed_text, "重吉", "ジュウキチ");
    processed_text = replace_all_ja(processed_text, "第三子", "ダイサンシ");
    processed_text = replace_all_ja(processed_text, "1曲", "イッキョク");
    processed_text = replace_all_ja(processed_text, "１曲", "イッキョク");
    processed_text = replace_all_ja(processed_text, "メドレー중", "メドレーチュウ");
    processed_text = replace_all_ja(processed_text, "メドレー中", "メドレーチュウ");
    processed_text = replace_all_ja(processed_text, "取上げた", "トリアゲタ");
    processed_text = replace_all_ja(processed_text, "ナチュラル1", "ナチュラルワン");
    processed_text = replace_all_ja(processed_text, "ナチュラル１", "ナチュラルワン");
    processed_text = replace_all_ja(processed_text, "내추럴일", "ナチュラルワン");
    processed_text = replace_all_ja(processed_text, "¥3500", "サンゼンゴヒャク");
    processed_text = replace_all_ja(processed_text, "¥３５００", "サンゼンゴヒャク");
    processed_text = replace_all_ja(processed_text, "22小悪魔", "ニニコアクマ");
    processed_text = replace_all_ja(processed_text, "２２小悪魔", "ニニコアクマ");
    processed_text = replace_all_ja(processed_text, "도22.4パーセント", "도二十二点四パーセント");
    processed_text = replace_all_ja(processed_text, "도２２.４パーセント", "도二十二点四パーセント");
    processed_text = replace_all_ja(processed_text, "も22.4パーセント", "も二十二点四パーセント");
    processed_text = replace_all_ja(processed_text, "も２２.４パーセント", "も二十二点四パーセント");
    processed_text = replace_all_ja(processed_text, "区分", "クブン");
    // ── Python _DICT_PATCHES sync: BERT 전처리 치환 (C++ OJT wins 패치 보완) ─
    // JSON 패치가 BERT call_overrides에 막히는 케이스들을 전처리로 해결
    processed_text = replace_all_ja(processed_text, u8"野生生物", u8"やせいせいぶつ");   // ノセエセエモノ→ヤセエセエブツ
    processed_text = replace_all_ja(processed_text, u8"実は", u8"じつは");               // ミワ→ジツワ (実は最大)
    processed_text = replace_all_ja(processed_text, u8"システム外", u8"システムがい");   // ソト→ガイ
    processed_text = replace_all_ja(processed_text, u8"一切", u8"いっさい");             // イチパツ→イッサイ
    processed_text = replace_all_ja(processed_text, u8"二次会", u8"にじかい");           // ニジア→ニジカイ
    processed_text = replace_all_ja(processed_text, u8"特急中", u8"とっきゅうちゅう");  // ナカ→チュウ
    processed_text = replace_all_ja(processed_text, u8"手下", u8"てした");               // テカ→テシタ
    processed_text = replace_all_ja(processed_text, u8"ビー玉", u8"ビーだま");           // タマ→ダマ (連濁)
    processed_text = replace_all_ja(processed_text, u8"下手", u8"へた");                 // シタテ→ヘタ
    processed_text = replace_all_ja(processed_text, u8"生み出す", u8"うみだす");         // エミダス→ウミダス
    processed_text = replace_all_ja(processed_text, u8"生み出", u8"うみだ");             // 接頭変化
    processed_text = replace_all_ja(processed_text, u8"生物", u8"せいぶつ");             // セエモノ→セエブツ
    processed_text = replace_all_ja(processed_text, u8"屋外型", u8"おくがいがた");       // ヤガイ→オクガイ
    processed_text = replace_all_ja(processed_text, u8"シーズン後半", u8"シーズンこうはん"); // アト→コウ
    processed_text = replace_all_ja(processed_text, u8"先生", u8"せんせい");             // サキセエ→センセエ
    processed_text = replace_all_ja(processed_text, u8"下士官", u8"かしかん");           // シタシカン→カシカン
    processed_text = replace_all_ja(processed_text, u8"巡視船", u8"じゅんしせん");       // フネ→セン
    processed_text = replace_all_ja(processed_text, u8"係争中", u8"けいそうちゅう");     // ナカ→チュウ
    processed_text = replace_all_ja(processed_text, u8"在り方", u8"ありかた");           // ホオ→カタ
    processed_text = replace_all_ja(processed_text, u8"外敵", u8"がいてき");             // ソトテキ→ガイテキ



    // KWDLC corrections
    processed_text = replace_all_ja(processed_text, "申請中", "しんせいちゅう");
    processed_text = replace_all_ja(processed_text, "特許申請中", "とっきょしんせいちゅう");
    processed_text = replace_all_ja(processed_text, "戦争中", "せんそうちゅう");
    processed_text = replace_all_ja(processed_text, "冬季中", "とうきちゅう");
    processed_text = replace_all_ja(processed_text, "手術中", "しゅじゅつちゅう");
    processed_text = replace_all_ja(processed_text, "メドレー中", "めどれーちゅう");
    processed_text = replace_all_ja(processed_text, "片思い中", "かたおもいちゅう");
    processed_text = replace_all_ja(processed_text, "片思い心中", "かたおもいちゅう");
    processed_text = replace_all_ja(processed_text, "株式会社", "かぶしきがいしゃ");
    processed_text = replace_all_ja(processed_text, "ご覧", "ごらん");
    processed_text = replace_all_ja(processed_text, "極まりない", "きわまりない");
    processed_text = replace_all_ja(processed_text, "極まりなかった", "きわまりなかった");
    processed_text = replace_all_ja(processed_text, "大分たちます", "だいぶたちます");
    processed_text = replace_all_ja(processed_text, "山間の", "やまあいの");
    processed_text = replace_all_ja(processed_text, "斜め上", "ななめうえ");
    processed_text = replace_all_ja(processed_text, "しばらくの間", "しばらくのあいだ");
    processed_text = replace_all_ja(processed_text, "悪い", "わるい");
    processed_text = replace_all_ja(processed_text, "의上で", "うえで");
    processed_text = replace_all_ja(processed_text, "のうえで", "のうえで");
    processed_text = replace_all_ja(processed_text, "の上で", "のうえで");
    processed_text = replace_all_ja(processed_text, "その上で", "そのうえで");
    processed_text = replace_all_ja(processed_text, "この上で", "このうえで");
    processed_text = replace_all_ja(processed_text, "他には", "ほかには");
    processed_text = replace_all_ja(processed_text, "西店", "にしてん");
    processed_text = replace_all_ja(processed_text, "ネズミ目", "ねずみもく");
    processed_text = replace_all_ja(processed_text, "込みの", "こみの");
    processed_text = replace_all_ja(processed_text, "込み", "こみ");

    // ── Python _DICT_PATCHES sync (OJT wins 분석 패치) ────────────────────────
    // 人たち → ひとたち (ニンタチ 방지)
    processed_text = replace_all_ja(processed_text, u8"\u4EBA\u305F\u3061", u8"\u3072\u3068\u305F\u3061"); // 人たち
    processed_text = replace_all_ja(processed_text, u8"\u4EBA\u9054",       u8"\u3072\u3068\u305F\u3061"); // 人達
    // 小エビ → こえび (ショウエビ 방지)
    processed_text = replace_all_ja(processed_text, u8"\u5C0F\u30A8\u30D3", u8"\u3053\u3048\u3073");     // 小エビ
    processed_text = replace_all_ja(processed_text, u8"\u5C0F\u578B",       u8"\u3053\u304C\u305F");     // 小型
    // 出入り口 / 入り口 連濁
    processed_text = replace_all_ja(processed_text, u8"\u51FA\u5165\u308A\u53E3", u8"\u3067\u3044\u308A\u3050\u3061"); // 出入り口
    processed_text = replace_all_ja(processed_text, u8"\u5165\u308A\u53E3",       u8"\u3044\u308A\u3050\u3061");     // 入り口
    // 者 suffix → しゃ (もの 방지)
    processed_text = replace_all_ja(processed_text, u8"\u76F8\u8AC7\u8005", u8"\u305D\u3046\u3060\u3093\u3057\u3083"); // 相談者
    processed_text = replace_all_ja(processed_text, u8"\u62C5\u5F53\u8005", u8"\u305F\u3093\u3068\u3046\u3057\u3083"); // 担当者
    processed_text = replace_all_ja(processed_text, u8"\u7D4C\u55B6\u8005", u8"\u3051\u3044\u3048\u3044\u3057\u3083"); // 経営者
    processed_text = replace_all_ja(processed_text, u8"\u8CAC\u4EFB\u8005", u8"\u305B\u304D\u306B\u3093\u3057\u3083"); // 責任者
    processed_text = replace_all_ja(processed_text, u8"\u95A2\u4FC2\u8005", u8"\u304B\u3093\u3051\u3044\u3057\u3083"); // 関係者
    processed_text = replace_all_ja(processed_text, u8"\u7814\u7A76\u8005", u8"\u3051\u3093\u304D\u3085\u3046\u3057\u3083"); // 研究者
    processed_text = replace_all_ja(processed_text, u8"\u7BA1\u7406\u8005", u8"\u304B\u3093\u308A\u3057\u3083");     // 管理者
    processed_text = replace_all_ja(processed_text, u8"\u652F\u6301\u8005", u8"\u3057\u3058\u3057\u3083");         // 支持者
    processed_text = replace_all_ja(processed_text, u8"\u4FE1\u8005",       u8"\u3057\u3093\u3058\u3083");         // 信者
    // 書 suffix → しょ
    processed_text = replace_all_ja(processed_text, u8"\u96C7\u7528\u5951\u7D04\u66F8", u8"\u3053\u3088\u3046\u3051\u3044\u3084\u304F\u3057\u3087"); // 雇用契約書
    processed_text = replace_all_ja(processed_text, u8"\u5951\u7D04\u66F8", u8"\u3051\u3044\u3084\u304F\u3057\u3087"); // 契約書
    processed_text = replace_all_ja(processed_text, u8"\u7533\u8ACB\u66F8", u8"\u3057\u3093\u305B\u3044\u3057\u3087"); // 申請書
    processed_text = replace_all_ja(processed_text, u8"\u8A3C\u660E\u66F8", u8"\u3057\u3087\u3046\u3081\u3044\u3057\u3087"); // 証明書
    processed_text = replace_all_ja(processed_text, u8"\u5831\u544A\u66F8", u8"\u307B\u3046\u3053\u304F\u3057\u3087"); // 報告書
    // 岳 → だけ (地名接尾辞)
    processed_text = replace_all_ja(processed_text, u8"\u5229\u5C3B\u5CB3", u8"\u308A\u3057\u308A\u3060\u3051"); // 利尻岳
    // 午後/午前 → ごご/ごぜん
    processed_text = replace_all_ja(processed_text, u8"\u5348\u5F8C", u8"\u3054\u3054");   // 午後
    processed_text = replace_all_ja(processed_text, u8"\u5348\u524D", u8"\u3054\u305C\u3093"); // 午前
    // 構成員 → こうせいん (二重い省略)
    processed_text = replace_all_ja(processed_text, u8"\u69CB\u6210\u54E1", u8"\u3053\u3046\u305B\u3044\u3093"); // 構成員
    // 燃え尽き → もえつき
    processed_text = replace_all_ja(processed_text, u8"\u71C3\u3048\u5C3D\u304D", u8"\u3082\u3048\u3064\u304D"); // 燃え尽き
    processed_text = replace_all_ja(processed_text, u8"\u713C\u304D\u5C3D\u304F", u8"\u3084\u304D\u3064\u304F"); // 焼き尽く
    processed_text = replace_all_ja(processed_text, u8"\u98F2\u307F\u5C3D\u304F", u8"\u306E\u307F\u3064\u304F"); // 飲み尽く
    // 記念柱 → きねんばしら
    processed_text = replace_all_ja(processed_text, u8"\u8A18\u5FF5\u67F1", u8"\u304D\u306D\u3093\u3070\u3057\u3089"); // 記念柱
    // 常陸国 → ひたちこく
    processed_text = replace_all_ja(processed_text, u8"\u5E38\u964B\u56FD", u8"\u3072\u305F\u3061\u3053\u304F"); // 常陸国

    // ── Batch 6: dict patches ─────────────────────────────────────────────────
    // 高校生 → こうこうせい
    processed_text = replace_all_ja(processed_text, u8"\u9AD8\u6821\u751F", u8"\u3053\u3046\u3053\u3046\u305B\u3044"); // 高校生
    // 数千/百/万/十 → すう (数をカズで読む方지)
    processed_text = replace_all_ja(processed_text, u8"\u6570\u5343", u8"\u3059\u3046\u305B\u3093"); // 数千
    processed_text = replace_all_ja(processed_text, u8"\u6570\u767E", u8"\u3059\u3046\u3072\u3083\u304F"); // 数百
    processed_text = replace_all_ja(processed_text, u8"\u6570\u4E07", u8"\u3059\u3046\u307E\u3093"); // 数万
    processed_text = replace_all_ja(processed_text, u8"\u6570\u5341", u8"\u3059\u3046\u3058\u3085\u3046"); // 数十
    // 100個/１００個 → ひゃっこ (促音化)
    processed_text = replace_all_ja(processed_text, u8"100\u500B", u8"\u3072\u3083\u3063\u3053"); // 100個
    processed_text = replace_all_ja(processed_text, u8"\uff11\uff10\uff10\u500B", u8"\u3072\u3083\u3063\u3053"); // １００個
    // 版 compounds → ばん (はん→ハン→ワン防止)
    processed_text = replace_all_ja(processed_text, u8"\u30EA\u30E1\u30A4\u30AF\u7248", u8"\u308A\u3081\u3044\u304F\u3070\u3093"); // リメイク版
    processed_text = replace_all_ja(processed_text, u8"\u65E5\u672C\u8A9E\u7248", u8"\u306B\u307B\u3093\u3054\u3070\u3093"); // 日本語版
    processed_text = replace_all_ja(processed_text, u8"\u96FB\u5B50\u7248", u8"\u3067\u3093\u3057\u3070\u3093"); // 電子版
    processed_text = replace_all_ja(processed_text, u8"\u521D\u56DE\u7248", u8"\u3057\u3087\u304B\u3044\u3070\u3093"); // 初回版
    processed_text = replace_all_ja(processed_text, u8"\u5B8C\u5168\u7248", u8"\u304B\u3093\u305C\u3093\u3070\u3093"); // 完全版
    processed_text = replace_all_ja(processed_text, u8"\u901A\u5E38\u7248", u8"\u3064\u3046\u3058\u3087\u3046\u3070\u3093"); // 通常版
    processed_text = replace_all_ja(processed_text, u8"\u9650\u5B9A\u7248", u8"\u3052\u3093\u3066\u3044\u3070\u3093"); // 限定版
    // 天の川 → あまのがわ (テンノガワ防止)
    processed_text = replace_all_ja(processed_text, u8"\u5929\u306E\u5DDD", u8"\u3042\u307E\u306E\u304C\u308F"); // 天の川
    // 月イチ → つきいち
    processed_text = replace_all_ja(processed_text, u8"\u6708\u30A4\u30C1", u8"\u3064\u304D\u3044\u3061"); // 月イチ
    // ある日 → あるひ
    processed_text = replace_all_ja(processed_text, u8"\u3042\u308B\u65E5", u8"\u3042\u308B\u3072"); // ある日
    // 目の当たり → まのあたり
    processed_text = replace_all_ja(processed_text, u8"\u76EE\u306E\u5F53\u305F\u308A", u8"\u307E\u306E\u3042\u305F\u308A"); // 目の当たり
    // 厚生労働省/厚生労働 (長い方を先に)
    processed_text = replace_all_ja(processed_text, u8"\u539A\u751F\u52B4\u50CD\u7701", u8"\u3053\u3046\u305B\u3044\u308D\u3046\u3069\u3046\u3057\u3087\u3046"); // 厚生労働省
    processed_text = replace_all_ja(processed_text, u8"\u539A\u751F\u52B4\u50CD", u8"\u3053\u3046\u305B\u3044\u308D\u3046\u3069\u3046"); // 厚生労働
    // イスラム教 → いすらむきょう
    processed_text = replace_all_ja(processed_text, u8"\u30A4\u30B9\u30E9\u30E0\u6559", u8"\u3044\u3059\u3089\u3080\u304D\u3087\u3046"); // イスラム教
    // 野生種 → やせいしゅ
    processed_text = replace_all_ja(processed_text, u8"\u91CE\u751F\u7A2E", u8"\u3084\u305B\u3044\u3057\u3085"); // 野生種
    // 愛して → あいして (長い方を先に)
    processed_text = replace_all_ja(processed_text, u8"\u611B\u3057\u3066\u3044\u308B", u8"\u3042\u3044\u3057\u3066\u3044\u308B"); // 愛している
    processed_text = replace_all_ja(processed_text, u8"\u611B\u3057\u3066\u308B", u8"\u3042\u3044\u3057\u3066\u308B"); // 愛してる
    processed_text = replace_all_ja(processed_text, u8"\u611B\u3057\u3066", u8"\u3042\u3044\u3057\u3066"); // 愛して
    // 道路 → どうろ (どうみち防止)
    processed_text = replace_all_ja(processed_text, u8"\u9053\u8DEF", u8"\u3069\u3046\u308D"); // 道路
    // 野球ポジション (翼手 → よくしゅ)
    processed_text = replace_all_ja(processed_text, u8"\u53F3\u7FFC\u624B", u8"\u3046\u3088\u304F\u3057\u3085"); // 右翼手
    processed_text = replace_all_ja(processed_text, u8"\u5DE6\u7FFC\u624B", u8"\u3055\u3088\u304F\u3057\u3085"); // 左翼手
    processed_text = replace_all_ja(processed_text, u8"\u4E2D\u5805\u624B", u8"\u3061\u3085\u3046\u3051\u3093\u3057\u3085"); // 中堅手
    // 月の息子 → つきのむすこ
    processed_text = replace_all_ja(processed_text, u8"\u6708\u306E\u606F\u5B50", u8"\u3064\u304D\u306E\u3080\u3059\u3053"); // 月の息子
    // リーグ戦 → りいぐせん
    processed_text = replace_all_ja(processed_text, u8"\u30EA\u30FC\u30B0\u6226", u8"\u308A\u3044\u3050\u305B\u3093"); // リーグ戦
    // 生電話 → なまでんわ
    processed_text = replace_all_ja(processed_text, u8"\u751F\u96FB\u8A71", u8"\u306A\u307E\u3067\u3093\u308F"); // 生電話
    // 頂上 → ちょうじょう (いただきじょう防止)
    processed_text = replace_all_ja(processed_text, u8"\u9802\u4E0A", u8"\u3061\u3087\u3046\u3058\u3087\u3046"); // 頂上
    // 補てん/補填 → ほてん
    processed_text = replace_all_ja(processed_text, u8"\u88DC\u3066\u3093", u8"\u307B\u3066\u3093"); // 補てん
    processed_text = replace_all_ja(processed_text, u8"\u88DC\u586B", u8"\u307B\u3066\u3093"); // 補填
    // 多発性 → たはつせい
    processed_text = replace_all_ja(processed_text, u8"\u591A\u767A\u6027", u8"\u305F\u306F\u3064\u305B\u3044"); // 多発性

    // ── Batch 7: dict patches ─────────────────────────────────────────────────
    // 川島町/川島 → かわじま (長い方を先に)
    processed_text = replace_all_ja(processed_text, u8"\u5DDD\u5CF6\u753A", u8"\u304B\u308F\u3058\u307E\u307E\u3061"); // 川島町
    processed_text = replace_all_ja(processed_text, u8"\u5DDD\u5CF6", u8"\u304B\u308F\u3058\u307E"); // 川島
    // 店 suffix → てん
    processed_text = replace_all_ja(processed_text, u8"\u713C\u8089\u5E97", u8"\u3084\u304D\u306B\u304F\u3066\u3093"); // 焼肉店
    processed_text = replace_all_ja(processed_text, u8"\u4ECF\u5177\u5E97", u8"\u3076\u3064\u3050\u3066\u3093"); // 仏具店
    processed_text = replace_all_ja(processed_text, u8"\u571F\u7523\u7269\u5E97", u8"\u307F\u3084\u3052\u3082\u306E\u3066\u3093"); // 土産物店
    processed_text = replace_all_ja(processed_text, u8"\u9774\u5E97", u8"\u304F\u3064\u3066\u3093"); // 靴店
    // 対米/対中/対日 → たいべい等
    processed_text = replace_all_ja(processed_text, u8"\u5BFE\u7C73", u8"\u305F\u3044\u3079\u3044"); // 対米
    processed_text = replace_all_ja(processed_text, u8"\u5BFE\u4E2D", u8"\u305F\u3044\u3061\u3085\u3046"); // 対中
    processed_text = replace_all_ja(processed_text, u8"\u5BFE\u65E5", u8"\u305F\u3044\u306B\u3061"); // 対日
    // 売却益 → ばいきゃくえき
    processed_text = replace_all_ja(processed_text, u8"\u58F2\u5374\u76CA", u8"\u3070\u3044\u304D\u3083\u304F\u3048\u304D"); // 売却益
    // 鳩時計 → はとどけい (連濁)
    processed_text = replace_all_ja(processed_text, u8"\u9CE9\u6642\u8A08", u8"\u306F\u3068\u3069\u3051\u3044"); // 鳩時計
    // 骨転移 → こつてんい
    processed_text = replace_all_ja(processed_text, u8"\u9AA8\u8EE2\u79FB", u8"\u3053\u3064\u3066\u3093\u3044"); // 骨転移
    // 交易路 → こうえきろ
    processed_text = replace_all_ja(processed_text, u8"\u4EA4\u6613\u8DEF", u8"\u3053\u3046\u3048\u304D\u308D"); // 交易路
    // 犂 → すき
    processed_text = replace_all_ja(processed_text, u8"\u7282", u8"\u3059\u304D"); // 犂 (plow)
    // 御通行 → ごつうこう
    processed_text = replace_all_ja(processed_text, u8"\u5FA1\u901A\u884C", u8"\u3054\u3064\u3046\u3053\u3046"); // 御通行
    // 日本が/と/へ → にっぽん (長い方優先)
    processed_text = replace_all_ja(processed_text, u8"\u65E5\u672C\u304C", u8"\u306B\u3063\u307D\u3093\u304C"); // 日本が
    processed_text = replace_all_ja(processed_text, u8"\u65E5\u672C\u3068", u8"\u306B\u3063\u307D\u3093\u3068"); // 日本と
    processed_text = replace_all_ja(processed_text, u8"\u65E5\u672C\u3078", u8"\u306B\u3063\u307D\u3093\u3078"); // 日本へ
    // 在学時 → ざいがくじ
    processed_text = replace_all_ja(processed_text, u8"\u5728\u5B66\u6642", u8"\u3056\u3044\u304C\u304F\u3058"); // 在学時
    // イシュミール国 → いしゅみーるこく
    processed_text = replace_all_ja(processed_text, u8"\u30A4\u30B7\u30E5\u30DF\u30FC\u30EB\u56FD", u8"\u3044\u3057\u3085\u307F\u30FC\u308B\u3053\u304F"); // イシュミール国

    // ── Batch 8: dict patches ─────────────────────────────────────────────────
    // 書き込み数 → かきこみすう (数=かず防止)
    processed_text = replace_all_ja(processed_text, u8"\u66F8\u304D\u8FBC\u307F\u6570", u8"\u304B\u304D\u3053\u307F\u3059\u3046"); // 書き込み数

    // ── Batch 9: Python sync patches ─────────────────────────────────────────
    // 生育地/生育 → せいく (ii->i 단축; BERT=セイイク -> norm후 セエイク≠Gold)
    processed_text = replace_all_ja(processed_text, u8"\u751F\u80B2\u5730", u8"\u305B\u3044\u304F\u3061"); // 生育地
    processed_text = replace_all_ja(processed_text, u8"\u751F\u80B2",      u8"\u305B\u3044\u304F");     // 生育
    // 祭 compounds (BERT splits separately)
    processed_text = replace_all_ja(processed_text, u8"\u56FD\u969B\u6620\u753B\u796D", u8"\u3053\u304F\u3055\u3044\u3048\u3044\u304C\u3055\u3044"); // 国際映画祭
    processed_text = replace_all_ja(processed_text, u8"\u56FD\u969B\u97F3\u697D\u796D", u8"\u3053\u304F\u3055\u3044\u304A\u3093\u304C\u304F\u3055\u3044"); // 国際音楽祭
    processed_text = replace_all_ja(processed_text, u8"\u82B8\u8853\u796D", u8"\u3052\u3044\u3058\u3085\u3064\u3055\u3044"); // 芸術祭

    // '後' + number pattern (Lookbehind (?<!午) workaround for std::regex)
    {
        std::string new_text = "";
        size_t last_pos = 0;
        std::regex re(R"(後\s*([0-9一二三四五六七八九十百千万億兆]))");
        std::sregex_iterator next(processed_text.begin(), processed_text.end(), re);
        std::sregex_iterator end;
        while (next != end) {
            std::smatch match = *next;
            size_t match_pos = match.position(0);
            new_text += processed_text.substr(last_pos, match_pos - last_pos);
            std::string prev = get_prev_char_ja(processed_text, match_pos);
            if (prev == "午" || prev == "\xE5\x8D\x88" || prev == "今" || prev == "\xE4\xBB\x8A") {
                new_text += match.str();
            } else {
                new_text += "あと" + match[1].str();
            }
            last_pos = match_pos + match.length(0);
            next++;
        }
        new_text += processed_text.substr(last_pos);
        processed_text = new_text;
    }

    // ── 第N話: digit-by-digit reading fix (Python sync) ──────────────────────
    // Python: pre-Phase-3 override per-digit annotations with compound number reading
    // C++: pre-lookup regex replacement (第N話 -> ダイ[num]ワ)
    {
        // Integer to katakana reading (1-9999)
        auto num_to_kata_dai = [](int n) -> std::string {
            if (n <= 0 || n >= 10000) return std::to_string(n);
            static const char* ones[] = {
                "",
                u8"\u30A4\u30C1",             // 1: イチ
                u8"\u30CB",                    // 2: ニ
                u8"\u30B5\u30F3",              // 3: サン
                u8"\u30E8\u30F3",              // 4: ヨン
                u8"\u30B4",                    // 5: ゴ
                u8"\u30ED\u30AF",              // 6: ロク
                u8"\u30CA\u30CA",              // 7: ナナ
                u8"\u30CF\u30C1",              // 8: ハチ
                u8"\u30AD\u30E5\u30A6"         // 9: キュウ
            };
            std::string r;
            if (n >= 1000) {
                int t = n / 1000;
                if (t > 1) r += ones[t];
                r += u8"\u30BB\u30F3"; // セン
                n %= 1000;
            }
            if (n >= 100) {
                int h = n / 100;
                if      (h == 3) r += u8"\u30B5\u30F3\u30D3\u30E3\u30AF"; // サンビャク
                else if (h == 6) r += u8"\u30ED\u30C3\u30D4\u30E3\u30AF"; // ロッピャク
                else if (h == 8) r += u8"\u30CF\u30C3\u30D4\u30E3\u30AF"; // ハッピャク
                else            { r += ones[h]; r += u8"\u30D2\u30E3\u30AF"; } // xヒャク
                n %= 100;
            }
            if (n >= 10) {
                int t2 = n / 10;
                if (t2 > 1) r += ones[t2];
                r += u8"\u30B8\u30E5\u30A6"; // ジュウ
                n %= 10;
            }
            if (n > 0) r += ones[n];
            return r;
        };
        std::string new_text;
        size_t last_pos2 = 0;
        std::regex re_dai_n_wa(u8"\u7B2C([0-9]+)\u8A71"); // 第N話
        std::sregex_iterator it2(processed_text.begin(), processed_text.end(), re_dai_n_wa);
        std::sregex_iterator end2;
        while (it2 != end2) {
            std::smatch m2 = *it2;
            size_t m2_pos = m2.position(0);
            new_text += processed_text.substr(last_pos2, m2_pos - last_pos2);
            try {
                int n2 = std::stoi(m2[1].str());
                new_text += u8"\u30C0\u30A4" + num_to_kata_dai(n2) + u8"\u30EF"; // ダイ...ワ
            } catch (...) {
                new_text += m2.str();
            }
            last_pos2 = m2_pos + m2.length(0);
            ++it2;
        }
        new_text += processed_text.substr(last_pos2);
        processed_text = new_text;
    }

    // 3. Convert numbers to Kanji (fallback)
    processed_text = convert_numbers(processed_text);

    // 4. Alphabet and Greek characters -> Kana (Pure C++ loop for safety, limited to alpha/greek)
    {
        std::string temp_res = "";
        auto processed_cps = utf8_to_codepoints_ja(processed_text);
        for (auto cp : processed_cps) {
            std::string ch = codepoint_to_utf8_ja(cp);
            std::string ch_lower = ch;
            for (auto& c : ch_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            bool is_alpha = (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
                            (cp >= 0x0391 && cp <= 0x03A9) || // Α-Ω (Greek upper)
                            (cp >= 0x03B1 && cp <= 0x03C9) || // α-ω (Greek lower)
                            (cp >= 0xFF21 && cp <= 0xFF3A) || // Fullwidth A-Z
                            (cp >= 0xFF41 && cp <= 0xFF5A);   // Fullwidth a-z

            if (is_alpha) {
                auto it = _ALPHASYMBOL_YOMI.find(ch_lower);
                if (it != _ALPHASYMBOL_YOMI.end()) {
                    temp_res += it->second;
                    continue;
                }
            }
            temp_res += ch;
        }
        processed_text = temp_res;
    }

    // 5. Punctuation normalization
    processed_text = normalize_punct(processed_text);



    // 7. Lookup step
    std::unordered_map<std::string, std::string> overrides;
    std::string kata = lookup(processed_text, overrides, out_accent_overrides);

    return kata;
}

} // namespace snap
