/**
 * text_normalize_kr.cpp — Korean text normalization engine
 * =========================================================
 * Port of RaconVoice V6 korean.py text normalization pipeline.
 *
 * Pipeline: preprocess_symbols → normalize_units_and_numbers →
 *           transliterate_english_fallback → adversarial_spelling_ko →
 *           adversarial_jongseong_ko → normalize_final
 */

#include "snap/text_normalize_kr.h"
#include "snap/num2words_ko.h"
#include "snap/classifier.h"

#include <nlohmann/json.hpp>
#include <regex>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <cassert>

namespace snap {

} // namespace snap (temporarily closed for file-local helpers)

namespace {

// ============================================================
// Static data: ENG_ALPHABET
// ============================================================
static const std::unordered_map<char, std::string>& eng_alphabet() {
    static const std::unordered_map<char, std::string> m = {
        {'A', u8"에이"}, {'B', u8"비"}, {'C', u8"씨"}, {'D', u8"디"},
        {'E', u8"이"}, {'F', u8"에프"}, {'G', u8"지"}, {'H', u8"에이치"},
        {'I', u8"아이"}, {'J', u8"제이"}, {'K', u8"케이"}, {'L', u8"엘"},
        {'M', u8"엠"}, {'N', u8"엔"}, {'O', u8"오"}, {'P', u8"피"},
        {'Q', u8"큐"}, {'R', u8"알"}, {'S', u8"에스"}, {'T', u8"티"},
        {'U', u8"유"}, {'V', u8"브이"}, {'W', u8"더블유"}, {'X', u8"엑스"},
        {'Y', u8"와이"}, {'Z', u8"제트"},
    };
    return m;
}

// ============================================================
// Static data: DIGIT_KR (phone number digit reading)
// ============================================================
static const std::unordered_map<char, std::string>& digit_kr() {
    static const std::unordered_map<char, std::string> m = {
        {'0', u8"공"}, {'1', u8"일"}, {'2', u8"이"}, {'3', u8"삼"}, {'4', u8"사"},
        {'5', u8"오"}, {'6', u8"육"}, {'7', u8"칠"}, {'8', u8"팔"}, {'9', u8"구"},
    };
    return m;
}

// ============================================================
// Static data: SUFFIX_UNIT_MAP (sorted by key length desc)
// ============================================================
static const std::vector<std::pair<std::string, std::string>>& suffix_unit_map() {
    static const std::vector<std::pair<std::string, std::string>> m = []() {
        std::vector<std::pair<std::string, std::string>> v = {
            {"GB/s", u8"기가바이트퍼세컨드"},
            {"MB/s", u8"메가바이트퍼세컨드"},
            {"KB/s", u8"킬로바이트퍼세컨드"},
            {"Gbps", u8"기가비피에스"},
            {"Mbps", u8"메가비피에스"},
            {"Kbps", u8"킬로비피에스"},
            {"ms",   u8"밀리초"},
            {"ns",   u8"나노초"},
            {u8"μs", u8"마이크로초"},
            {"Gb",   u8"기가바이트"},
            {"Mb",   u8"메가바이트"},
            {"Tb",   u8"테라바이트"},
            {"Kb",   u8"킬로바이트"},
            {"dB",   u8"데시벨"},
            {"cc",   u8"씨씨"},
            {"L",    u8"리터"},
            {"kHz",  u8"킬로헤르츠"},
            {"mL",   u8"밀리리터"}, {"ml", u8"밀리리터"},
            {"km²",  u8"제곱킬로미터"}, {"km", u8"킬로미터"}, {"KM", u8"킬로미터"},
            {"kg",   u8"킬로그램"}, {"KG", u8"킬로그램"},
            {"ug",   u8"마이크로그램"}, {"mg", u8"밀리그램"}, {"g", u8"그램"},
            {"kt",   u8"킬로톤"}, {"Mt", u8"메가톤"}, {"t", u8"톤"},
            {"mm",   u8"밀리미터"}, {"MM", u8"밀리미터"},
            {"cm",   u8"센티미터"}, {"CM", u8"센티미터"},
            {"m²",   u8"제곱미터"}, {"m2", u8"제곱미터"},
            {"m",    u8"미터"},
            {"%p",   u8"퍼센트포인트"}, {"%P", u8"퍼센트포인트"},
            {"%",    u8"퍼센트"},
            {"px",   u8"픽셀"},
            {"GB",   u8"기가바이트"}, {"MB", u8"메가바이트"}, {"TB", u8"테라바이트"},
            {"GHz",  u8"기가헤르츠"}, {"MHz", u8"메가헤르츠"},
            {"GWh",  u8"기가와트시"}, {"MWh", u8"메가와트시"}, {"kWh", u8"킬로와트시"},
            {"GW",   u8"기가와트"}, {"MW", u8"메가와트"}, {"kW", u8"킬로와트"}, {"KW", u8"킬로와트"},
            {"W",    u8"와트"},
            {u8"㎾h", u8"킬로와트시"},
            {u8"㎾",  u8"킬로와트"},
            {u8"㎿",  u8"메가와트"},
            {u8"㎽",  u8"밀리와트"},
            {u8"㎎",  u8"밀리그램"},
            {u8"㎏",  u8"킬로그램"},
            {u8"㎞",  u8"킬로미터"},
            {u8"㎝",  u8"센티미터"},
            {u8"㎜",  u8"밀리미터"},
            {u8"㎡",  u8"제곱미터"},
            {u8"㎢",  u8"제곱킬로미터"},
            {u8"㎖",  u8"밀리리터"},
            {u8"㎍",  u8"마이크로그램"},
            {u8"㎛",  u8"마이크로미터"},
            {u8"㎐",  u8"헤르츠"},
            {u8"㎑",  u8"킬로헤르츠"},
            {u8"㎒",  u8"메가헤르츠"},
            {"Hz",   u8"헤르츠"}
        };
        // Sort by key length descending (stable sort to keep order for same length)
        std::stable_sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
            return a.first.size() > b.first.size();
        });
        return v;
    }();
    return m;
}

// ============================================================
// Static data: PREFIX_UNIT_MAP (currency symbols)
// ============================================================
struct PrefixUnit { std::string symbol; std::string korean; };
static const std::vector<PrefixUnit>& prefix_unit_map() {
    static const std::vector<PrefixUnit> m = {
        {"$",    u8"달러"},
        {u8"€",  u8"유로"},
        {u8"¥",  u8"엔"},
        {u8"£",  u8"파운드"},
        {u8"₩",  u8"원"},
    };
    return m;
}

static const std::vector<std::string>& native_units_list() {
    static const std::vector<std::string> v = []() {
        std::vector<std::string> c = {
            u8"개", u8"명", u8"살", u8"마리", u8"잔", u8"병", u8"벌", u8"켤레", u8"채",
            u8"자루", u8"포기", u8"곡", u8"그릇", u8"바퀴", u8"가지",
            u8"줄", u8"뼘", u8"톨", u8"쌍", u8"모금", u8"숟가락", u8"움큼",
            u8"통", u8"그루", u8"송이", u8"다발", u8"봉지", u8"상자", u8"묶음",
            u8"시", u8"권", u8"편", u8"장", u8"대"
        };
        std::stable_sort(c.begin(), c.end(), [](const auto& a, const auto& b) {
            return a.size() > b.size();
        });
        return c;
    }();
    return v;
}

static const std::vector<std::string>& sino_units_list() {
    static const std::vector<std::string> v = []() {
        std::vector<std::string> c = {
            u8"층", u8"호", u8"번",
            u8"년", u8"월", u8"일", u8"분", u8"초",
            u8"원", u8"평", u8"학년", u8"주년", u8"학기",
            u8"세",
            u8"도", u8"kg", u8"km", u8"m", u8"cm", u8"mm", u8"g", u8"ml", u8"l",
            u8"점", u8"위", u8"등", u8"배", u8"회", u8"차",
            u8"페이지", u8"쪽"
        };
        std::stable_sort(c.begin(), c.end(), [](const auto& a, const auto& b) {
            return a.size() > b.size();
        });
        return c;
    }();
    return v;
}

// ============================================================
// Static data: NATIVE_COUNTERS
// ============================================================
static const std::vector<std::string>& native_counters() {
    // Sorted by length descending for regex matching
    static const std::vector<std::string> v = []() {
        std::vector<std::string> c = {
            u8"켤레", u8"마리", u8"그루", u8"송이", u8"포기",
            u8"시", u8"개", u8"명", u8"살", u8"대",
            u8"권", u8"잔", u8"번", u8"벌", u8"채",
            u8"척", u8"통",
        };
        std::stable_sort(c.begin(), c.end(), [](const auto& a, const auto& b) {
            return a.size() > b.size();
        });
        return c;
    }();
    return v;
}

// ============================================================
// UTF-8 helpers
// ============================================================

/// Decode a single UTF-8 codepoint from position pos. Advances pos.
static uint32_t utf8_decode(const std::string& s, size_t& pos) {
    if (pos >= s.size()) return 0;
    unsigned char c = (unsigned char)s[pos];
    if (c < 0x80) {
        pos += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0) {
        if (pos + 1 >= s.size()) { pos = s.size(); return 0; }
        uint32_t cp = (c & 0x1F) << 6;
        cp |= ((unsigned char)s[pos + 1] & 0x3F);
        pos += 2;
        return cp;
    }
    if ((c & 0xF0) == 0xE0) {
        if (pos + 2 >= s.size()) { pos = s.size(); return 0; }
        uint32_t cp = (c & 0x0F) << 12;
        cp |= ((unsigned char)s[pos + 1] & 0x3F) << 6;
        cp |= ((unsigned char)s[pos + 2] & 0x3F);
        pos += 3;
        return cp;
    }
    if ((c & 0xF8) == 0xF0) {
        if (pos + 3 >= s.size()) { pos = s.size(); return 0; }
        uint32_t cp = (c & 0x07) << 18;
        cp |= ((unsigned char)s[pos + 1] & 0x3F) << 12;
        cp |= ((unsigned char)s[pos + 2] & 0x3F) << 6;
        cp |= ((unsigned char)s[pos + 3] & 0x3F);
        pos += 4;
        return cp;
    }
    pos += 1;
    return 0;
}

/// Encode a single codepoint to UTF-8 string
static std::string utf8_encode(uint32_t cp) {
    std::string s;
    if (cp < 0x80) {
        s += (char)cp;
    } else if (cp < 0x800) {
        s += (char)(0xC0 | (cp >> 6));
        s += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += (char)(0xE0 | (cp >> 12));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    } else {
        s += (char)(0xF0 | (cp >> 18));
        s += (char)(0x80 | ((cp >> 12) & 0x3F));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    }
    return s;
}

/// Check if all chars are uppercase ASCII alpha
static bool is_all_upper_alpha(const std::string& s) {
    for (char c : s) {
        if (c < 'A' || c > 'Z') return false;
    }
    return !s.empty();
}

/// Check if string has any lowercase letter
static bool has_lower(const std::string& s) {
    for (char c : s) {
        if (c >= 'a' && c <= 'z') return true;
    }
    return false;
}
} // anonymous namespace

namespace snap {

// ============================================================
// TextNormalizeKr implementation
// ============================================================

TextNormalizeKr::TextNormalizeKr() {}
TextNormalizeKr::~TextNormalizeKr() {}

// ── init ─────────────────────────────────────────────
bool TextNormalizeKr::init(const std::string& weights_dir) {
    using json = nlohmann::json;

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

    // If path ends with /models or /models/, strip it to get root
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

    std::string ko_dir;
    {
        std::string cand_models = snap_home + "/models/ko/";
        std::string cand_resources = snap_home + "/resources/";
        std::string cand_weights = snap_home + "/weights/ko/";
        std::string cand_direct = snap_home + "/ko/";
        std::ifstream f_models(cand_models + "dict_eng_merged.json", std::ios::binary);
        std::ifstream f_res(cand_resources + "dict_eng_merged.json", std::ios::binary);
        std::ifstream f_weights(cand_weights + "dict_eng_merged.json", std::ios::binary);

        if (f_models.good()) {
            ko_dir = cand_models;
        } else if (f_res.good()) {
            ko_dir = cand_resources;
        } else if (f_weights.good()) {
            ko_dir = cand_weights;
        } else {
            ko_dir = cand_models;
        }
    }

    // Add inline A-Z single-letter entries
    for (char c = 'A'; c <= 'Z'; ++c) {
        auto it = eng_alphabet().find(c);
        if (it != eng_alphabet().end()) {
            english_dictionary_exact_[std::string(1, c)] = it->second;
            english_dictionary_[std::string(1, c)] = it->second;
        }
    }

    // Load dict_eng_merged.json (Strict Exit Policy: 필수 사전 미존재 시 초기화 강제 실패)
    {
        std::ifstream ifs(ko_dir + "dict_eng_merged.json", std::ios::binary);
        if (!ifs.is_open()) {
            std::cerr << "[SNAP Strict Policy Error] 필수 영어 발음 사전을 찾을 수 없습니다!\n"
                      << "  - SNAP_HOME 앵커: " << snap_home << "\n"
                      << "  - 탐색 경로: " << ko_dir << "dict_eng_merged.json\n"
                      << "  - 필수 파일(dict_eng_merged.json)이 없으므로 초기화를 즉시 중단합니다." << std::endl;
            return false;
        }
        json j = json::parse(ifs, nullptr, false);
        if (!j.is_discarded() && j.is_object()) {
            for (auto& [k, v] : j.items()) {
                english_dictionary_exact_[k] = v.get<std::string>();
                english_dictionary_[to_upper(k)] = v.get<std::string>();
            }
        }
    }

    // Load etc_dictionary.json
    {
        std::ifstream ifs(ko_dir + "etc_dictionary.json", std::ios::binary);
        if (ifs.is_open()) {
            json j = json::parse(ifs, nullptr, false);
            if (!j.is_discarded() && j.is_object()) {
                for (auto& [k, v] : j.items()) {
                    etc_dictionary_[k] = v.get<std::string>();
                }
            }
        }
    }

    // Load known_words.json → vector of pairs
    {
        std::ifstream ifs(ko_dir + "known_words.json", std::ios::binary);
        if (ifs.is_open()) {
            json j = json::parse(ifs, nullptr, false);
            if (!j.is_discarded() && j.is_object()) {
                for (auto& [k, v] : j.items()) {
                    known_words_.emplace_back(k, v.get<std::string>());
                }
            }
        }
    }

    return true;
}

// ── normalize (public entry) ─────────────────────────
std::string TextNormalizeKr::normalize(const std::string& text) const {
    std::string t = text;

    // Trim leading/trailing whitespace
    {
        size_t s = t.find_first_not_of(" \t\r\n");
        size_t e = t.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) return "";
        t = t.substr(s, e - s + 1);
    }

    t = preprocess_symbols(t);
    t = normalize_units_and_numbers(t);
    t = transliterate_english_fallback(t);
    t = adversarial_spelling_ko(t);
    t = adversarial_jongseong_ko(t);
    t = normalize_final(t);
    return t;
}

static std::string decimal_to_kor_str(const std::string& s) {
    size_t dot = s.find('.');
    if (dot == std::string::npos) return num2words_ko(std::stoll(s));
    std::string int_s = s.substr(0, dot);
    std::string dec_s = s.substr(dot + 1);
    
    std::string int_kor = int_s.empty() ? u8"영" : num2words_ko(std::stoll(int_s));
    std::string dec_kor;
    for (char c : dec_s) {
        auto dit = digit_kr().find(c);
        if (dit != digit_kr().end()) {
            dec_kor += dit->second;
        }
    }
    return int_kor + u8"점" + dec_kor;
}

static std::string num_label_to_kor(int64_t n, const std::string& label, const std::string& after) {
    if (label == "native") {
        return to_native_korean(static_cast<int>(n));
    }
    if (label == "sino") {
        return num2words_ko(n);
    }
    // No label
    if (n >= 100) {
        return num2words_ko(n);
    }
    for (const auto& unit : native_units_list()) {
        if (after.size() >= unit.size() && after.compare(0, unit.size(), unit) == 0) {
            return to_native_korean(static_cast<int>(n));
        }
    }
    for (const auto& unit : sino_units_list()) {
        if (after.size() >= unit.size() && after.compare(0, unit.size(), unit) == 0) {
            return num2words_ko(n);
        }
    }
    return num2words_ko(n);
}

std::vector<NormalizedSpan> TextNormalizeKr::scan(
        const std::string& text,
        const std::vector<NumberItem>& numbers) const {
    
    std::vector<NormalizedSpan> spans;
    std::vector<std::pair<size_t, size_t>> claimed;

    // byte to char index mapping
    std::vector<size_t> byte_to_char_idx(text.size() + 1, 0);
    {
        size_t char_count = 0;
        size_t byte_pos = 0;
        while (byte_pos < text.size()) {
            byte_to_char_idx[byte_pos] = char_count;
            size_t next_pos = byte_pos;
            utf8_decode(text, next_pos);
            for (size_t i = byte_pos + 1; i < next_pos; ++i) {
                byte_to_char_idx[i] = char_count;
            }
            char_count++;
            byte_pos = next_pos;
        }
        byte_to_char_idx[text.size()] = char_count;
    }

    auto add_span = [&](size_t start, size_t end, const std::string& repl) {
        // UTF-8 boundary check
        if (start > 0 && start < text.size() && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
            return false;
        }
        if (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) {
            return false;
        }

        for (const auto& [s, e] : claimed) {
            if (start < e && end > s) {
                return false;
            }
        }
        claimed.push_back({start, end});
        
        // Convert byte index to char index
        size_t start_char = byte_to_char_idx[start];
        size_t end_char = byte_to_char_idx[end];
        spans.push_back({start_char, end_char, repl});
        return true;
    };

    // Step 0: Remove parenthesized annotations — e.g. 가속(CUDA)을 → 가속을
    //   Only properly CLOSED parentheses (1-30 chars inside).
    //   Unclosed parentheses are NOT matched → content kept as-is.
    //   Mirrors Python scan() step 0: re.finditer(r'\([^)]{1,30}\)', text)
    {
        std::regex paren_re(R"(\([^)]{1,30}\))");
        auto pit = std::sregex_iterator(text.begin(), text.end(), paren_re);
        auto pend = std::sregex_iterator();
        for (; pit != pend; ++pit) {
            size_t s = static_cast<size_t>(pit->position());
            size_t e = s + static_cast<size_t>(pit->length());
            add_span(s, e, "");
        }
    }

    // 1. Prefix units (e.g. $100 -> 백달러)
    for (const auto& pu : prefix_unit_map()) {
        std::string escaped;
        for (char c : pu.symbol) {
            if (c == '$' || c == '(' || c == ')' || c == '+' || c == '*' || c == '?' || c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == '\\' || c == '^' || c == '.' || c == '/') {
                escaped += '\\';
            }
            escaped += c;
        }
        std::regex re(escaped + R"(\s*(\d+))");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            int64_t n = std::stoll((*it)[1].str());
            add_span(it->position(), it->position() + it->length(), num2words_ko(n) + pu.korean);
        }
    }

    // 2. IP Addresses (Exclude \b border to align with Python)
    {
        std::regex re(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string ip = it->str();
            std::stringstream ss(ip);
            std::string octet;
            std::vector<std::string> parts;
            while (std::getline(ss, octet, '.')) {
                parts.push_back(num2words_ko(std::stoll(octet)));
            }
            if (parts.size() == 4) {
                std::string ip_kor = parts[0] + u8" 점 " + parts[1] + u8" 점 " + parts[2] + u8" 점 " + parts[3];
                add_span(it->position(), it->position() + it->length(), ip_kor);
            }
        }
    }

    // 3. Phone numbers (C++ logic-based boundary check to avoid lookaround)
    {
        std::regex re(R"((\+\d{1,3}[-.]?\d{1,4}[-.]?\d{3,4}[-.]?\d{4}|0\d{1,2}[-.]?\d{3,4}[-.]?\d{4}|1[0-9]{3}[-.]?\d{4}))");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            size_t start = it->position();
            size_t length = it->length();
            size_t end_pos = start + length;

            if (start > 0 && std::isdigit((unsigned char)text[start - 1])) {
                continue;
            }
            if (end_pos < text.size() && std::isdigit((unsigned char)text[end_pos])) {
                continue;
            }

            std::string matched = it->str();
            std::string phone_clean = matched;
            if (!phone_clean.empty() && phone_clean[0] == '+') {
                phone_clean = phone_clean.substr(1);
            }

            std::vector<std::string> groups;
            std::string current_group;
            for (char c : phone_clean) {
                if (c == '-' || c == '.' || c == ' ') {
                    if (!current_group.empty()) {
                        groups.push_back(current_group);
                        current_group.clear();
                    }
                } else {
                    current_group += c;
                }
            }
            if (!current_group.empty()) {
                groups.push_back(current_group);
            }

            std::string converted;
            bool first_group = true;
            for (const auto& g : groups) {
                bool all_digit = true;
                for (char c : g) {
                    if (!std::isdigit((unsigned char)c)) {
                        all_digit = false;
                        break;
                    }
                }
                if (all_digit && !g.empty()) {
                    if (!first_group) converted += " ";
                    for (char c : g) {
                        auto dit = digit_kr().find(c);
                        if (dit != digit_kr().end()) converted += dit->second;
                    }
                    first_group = false;
                }
            }
            add_span(start, end_pos, converted);
        }
    }

    // 4. Suffix units (e.g. 100km, 3.439%)
    for (const auto& [unit, kor_unit] : suffix_unit_map()) {
        std::string escaped;
        for (char c : unit) {
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '(' ||
                c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
                c == '\\' || c == '^' || c == '$' || c == '|' || c == '/') {
                escaped += '\\';
            }
            escaped += c;
        }
        // 4a. Decimal + unit
        {
            std::regex re("(\\d+\\.\\d+)\\s*" + escaped);
            std::sregex_iterator it(text.begin(), text.end(), re);
            std::sregex_iterator end;
            for (; it != end; ++it) {
                size_t end_pos = it->position() + it->length();
                if (end_pos < text.size() && std::isalpha((unsigned char)text[end_pos])) {
                    continue;
                }
                std::string num_str = (*it)[1].str();
                add_span(it->position(), end_pos, decimal_to_kor_str(num_str) + kor_unit);
            }
        }
        // 4b. Integer + unit
        {
            std::regex re("(\\d+)\\s*" + escaped);
            std::sregex_iterator it(text.begin(), text.end(), re);
            std::sregex_iterator end;
            for (; it != end; ++it) {
                size_t end_pos = it->position() + it->length();
                if (end_pos < text.size() && std::isalpha((unsigned char)text[end_pos])) {
                    continue;
                }
                int64_t val = std::stoll((*it)[1].str());
                add_span(it->position(), end_pos, num2words_ko(val) + kor_unit);
            }
        }
    }

    // 5. Decimal numbers with commas (e.g. 2,650.32 -> 이천육백오십점삼이)
    {
        std::regex re(R"((\d{1,3}(?:,\d{3})+)\.(\d+))");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string int_str = (*it)[1].str();
            std::string dec_str = (*it)[2].str();
            int_str.erase(std::remove(int_str.begin(), int_str.end(), ','), int_str.end());
            int64_t int_val = std::stoll(int_str);
            std::string dec_kor;
            for (char c : dec_str) {
                auto dit = digit_kr().find(c);
                if (dit != digit_kr().end()) dec_kor += dit->second;
            }
            add_span(it->position(), it->position() + it->length(), num2words_ko(int_val) + u8"점" + dec_kor);
        }
    }

    // 6. Standalone decimal numbers (3.14 -> 삼점일사)
    {
        std::regex re(R"(\d+\.\d+)");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string after = text.substr(it->position() + it->length());
            if (!after.empty() && after[0] == '.' && after.size() > 1 && std::isdigit((unsigned char)after[1])) {
                continue;
            }
            add_span(it->position(), it->position() + it->length(), decimal_to_kor_str(it->str()));
        }
    }

    // 7. Numbers (with numbers label mapping)
    {
        std::map<int, std::string> num_labels;
        for (const auto& item : numbers) {
            num_labels[item.start] = item.label;
        }

        std::regex re(R"(\d{1,3}(?:,\d{3})+|\d+)");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string raw = it->str();
            raw.erase(std::remove(raw.begin(), raw.end(), ','), raw.end());
            int64_t n = std::stoll(raw);
            size_t start_pos = it->position();
            std::string before = text.substr(0, start_pos);
            std::string after = text.substr(start_pos + it->length());

            std::string label;
            size_t start_char = byte_to_char_idx[start_pos];
            auto lit = num_labels.find(static_cast<int>(start_char));
            if (lit != num_labels.end()) {
                label = lit->second;
            }

            std::string before_trimmed = before;
            while (!before_trimmed.empty() && std::isspace((unsigned char)before_trimmed.back())) {
                before_trimmed.pop_back();
            }
            std::string je = "\xEC\xA0\x9C";
            if (before_trimmed.size() >= je.size() && before_trimmed.compare(before_trimmed.size() - je.size(), je.size(), je) == 0) {
                label = "sino";
            }

            std::string repl;
            // --- Rule: number after English letter (product/model context) ---
            // Mirrors Python text_normalize_kr.py logic
            bool preceded_by_eng = false;
            if (!before_trimmed.empty()) {
                // before_trimmed is ASCII-safe; last char check
                char last_c = before_trimmed.back();
                preceded_by_eng = (last_c >= 'A' && last_c <= 'Z') ||
                                  (last_c >= 'a' && last_c <= 'z');
            }

            // Check if a known unit suffix follows the number
            bool after_has_unit = false;
            if (preceded_by_eng) {
                for (const auto& u : native_units_list()) {
                    if (after.rfind(u, 0) == 0) { after_has_unit = true; break; }
                }
                if (!after_has_unit) {
                    for (const auto& u : sino_units_list()) {
                        if (after.rfind(u, 0) == 0) { after_has_unit = true; break; }
                    }
                }
            }

            if (raw.size() > 1 && raw[0] == '0') {
                // Leading 0 -> digit-by-digit
                for (char c : raw) {
                    auto dit = digit_kr().find(c);
                    if (dit != digit_kr().end()) repl += dit->second;
                }
            } else if (raw.size() >= 6 && label.empty() && it->str().find(',') == std::string::npos) {
                // 6+ digits, no label -> digit-by-digit
                for (char c : raw) {
                    auto dit = digit_kr().find(c);
                    if (dit != digit_kr().end()) repl += dit->second;
                }
            } else if (preceded_by_eng && !after_has_unit) {
                // English product/model context
                static const std::unordered_map<int64_t, std::string> eng_digits = {
                    {1,"원"},{2,"투"},{3,"쓰리"},{4,"포"},{5,"파이브"},
                    {6,"식스"},{7,"세븐"},{8,"에잇"},{9,"나인"},{10,"텐"}
                };
                if (n >= 1 && n <= 10) {
                    // M4->포, i9->나인, Windows 10->텐
                    repl = eng_digits.at(n);
                } else if (raw.size() >= 4 && it->str().find(',') == std::string::npos) {
                    if (n % 100 == 0) {
                        // a1000->천, a5000->오천 (clean multiple of 100 -> sino)
                        repl = num_label_to_kor(n, "sino", after);
                    } else {
                        // RTX 3090->삼공구공, a1549->일오사구 (irregular -> digit-by-digit)
                        for (char c : raw) {
                            auto dit = digit_kr().find(c);
                            if (dit != digit_kr().end()) repl += dit->second;
                        }
                    }
                } else {
                    // Windows 11->십일, iPhone 16->십육 (2-3 digit -> sino)
                    repl = num_label_to_kor(n, label, after);
                }
            } else {
                repl = num_label_to_kor(n, label, after);
            }
            add_span(start_pos, start_pos + it->length(), repl);

        }
    }

    // 7.5 Standalone Unicode unit symbols
    {
        static const std::vector<std::string> unicode_units = {
            u8"㎾h", u8"㎾", u8"㎿", u8"㎽", u8"㎎", u8"㎏", u8"㎞", u8"㎝", u8"㎜",
            u8"㎡", u8"㎢", u8"㎖", u8"㎍", u8"㎛", u8"㎐", u8"㎑", u8"㎒"
        };
        for (const auto& usym : unicode_units) {
            std::string kor_unit;
            for (const auto& [u, k] : suffix_unit_map()) {
                if (u == usym) {
                    kor_unit = k;
                    break;
                }
            }
            if (kor_unit.empty()) continue;

            std::string escaped;
            for (char c : usym) {
                if (c == '.' || c == '+' || c == '*' || c == '?' || c == '(' ||
                    c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
                    c == '\\' || c == '^' || c == '$' || c == '|' || c == '/') {
                    escaped += '\\';
                }
                escaped += c;
            }
            std::regex re(escaped);
            std::sregex_iterator it(text.begin(), text.end(), re);
            std::sregex_iterator end;
            for (; it != end; ++it) {
                size_t start = it->position();
                size_t end_pos = start + it->length();
                if (start > 0 && std::isdigit((unsigned char)text[start - 1])) {
                    continue;
                }
                add_span(start, end_pos, kor_unit);
            }
        }
    }

    // 8. English words ([A-Za-z]+)
    {
        std::regex re("[A-Za-z]+");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            add_span(it->position(), it->position() + it->length(), english_word_to_korean(it->str()));
        }
    }

    // 9. Hanja in parentheses (e.g. (漢字))
    {
        std::regex re(R"(\(([^)]+)\))");
        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string content = (*it)[1].str();
            if (content.empty()) continue;

            bool all_hanja = true;
            size_t pos = 0;
            while (pos < content.size()) {
                uint32_t cp = utf8_decode(content, pos);
                if (cp < 0x4E00 || cp > 0x9FFF) {
                    all_hanja = false;
                    break;
                }
            }

            if (all_hanja) {
                add_span(it->position(), it->position() + it->length(), "");
            }
        }
    }

    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.start < b.start;
    });

    return spans;
}

std::string TextNormalizeKr::apply_spans(const std::string& text,
        const std::vector<NormalizedSpan>& spans) const {
    if (spans.empty()) {
        return text;
    }

    // 1. Sort spans by start character index ascending
    std::vector<NormalizedSpan> sorted_spans = spans;
    std::sort(sorted_spans.begin(), sorted_spans.end(), [](const auto& a, const auto& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.end < b.end;
    });

    // 2. Build char to byte index mapping once
    std::vector<size_t> char_to_byte_idx;
    char_to_byte_idx.reserve(text.size() + 1);
    {
        size_t byte_pos = 0;
        while (byte_pos < text.size()) {
            char_to_byte_idx.push_back(byte_pos);
            utf8_decode(text, byte_pos);
        }
        char_to_byte_idx.push_back(text.size());
    }

    std::string result;
    result.reserve(text.size() * 2);

    size_t last_char_pos = 0;
    size_t num_chars = char_to_byte_idx.size() - 1;

    for (const auto& span : sorted_spans) {
        // Prevent overlapping or reversed invalid spans
        if (span.start < last_char_pos) {
            continue;
        }
        if (span.start > num_chars) {
            break;
        }

        // Copy raw text between last span end and current span start
        size_t copy_start_byte = char_to_byte_idx[last_char_pos];
        size_t copy_end_byte = char_to_byte_idx[span.start];
        if (copy_end_byte > copy_start_byte) {
            result.append(text, copy_start_byte, copy_end_byte - copy_start_byte);
        }

        // Add replacement
        result.append(span.replacement);

        // Update position
        last_char_pos = (span.end < num_chars) ? span.end : num_chars;
    }

    // Copy remaining raw text
    if (last_char_pos < num_chars) {
        size_t copy_start_byte = char_to_byte_idx[last_char_pos];
        result.append(text, copy_start_byte, text.size() - copy_start_byte);
    }

    return result;
}

// ── preprocess_symbols ───────────────────────────────
std::string TextNormalizeKr::preprocess_symbols(const std::string& text) const {
    std::string t = text;

    // Remove markdown symbols: * _ ` # >
    {
        std::string result;
        result.reserve(t.size());
        for (char c : t) {
            if (c != '*' && c != '_' && c != '`' && c != '#' && c != '>') {
                result += c;
            }
        }
        t = result;
    }

    // Remove parenthesized content — e.g. 가속(CUDA)을 → 가속을
    // Only properly CLOSED parentheses (up to 30 chars). Unclosed → kept.
    t = std::regex_replace(t, std::regex(R"(\([^)]{1,30}\))"), "");

    // Normalize fullwidth tilde ～ → ~
    {
        std::string from = u8"～";
        size_t pos = 0;
        while ((pos = t.find(from, pos)) != std::string::npos) {
            t.replace(pos, from.size(), "~");
            pos += 1;
        }
    }

    // Clean up repeated punctuation
    t = std::regex_replace(t, std::regex(R"(\.{2,})"), ",");
    t = std::regex_replace(t, std::regex(R"(!{2,})"), "!");
    t = std::regex_replace(t, std::regex(R"(\?{2,})"), "?");

    // Remove em-dash/en-dash (manual UTF-8 replacement)
    {
        for (const auto& dash : {u8"—", u8"–"}) {
            std::string d(dash);
            size_t pos = 0;
            while ((pos = t.find(d, pos)) != std::string::npos) {
                t.replace(pos, d.size(), " ");
                pos += 1;
            }
        }
    }

    // Collapse multiple spaces
    t = std::regex_replace(t, std::regex(R"(\s{2,})"), " ");

    // Trim
    {
        size_t s = t.find_first_not_of(" \t\r\n");
        size_t e = t.find_last_not_of(" \t\r\n");
        if (s == std::string::npos) return "";
        t = t.substr(s, e - s + 1);
    }

    return t;
}

// ── normalize_units_and_numbers ──────────────────────
std::string TextNormalizeKr::normalize_units_and_numbers(const std::string& text) const {
    std::string t = text;

    // ===== 0. etc_dictionary pre-processing =====
    t = normalize_with_dictionary(t, etc_dictionary_);

    // ===== 1. Phone numbers (digit-by-digit) =====
    {
        std::regex re(R"(\b\d{2,4}-\d{3,4}-\d{4}\b)");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            std::string matched = it->str();
            // Split by '-' and convert each digit
            std::string converted;
            bool first_seg = true;
            std::istringstream ss(matched);
            std::string seg;
            while (std::getline(ss, seg, '-')) {
                if (!first_seg) converted += " ";
                for (char d : seg) {
                    auto dit = digit_kr().find(d);
                    if (dit != digit_kr().end()) converted += dit->second;
                }
                first_seg = false;
            }
            result += converted;
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 2. Date patterns (2024.05.13 etc.) =====
    {
        std::regex re(R"((\d{4})[.\-/](\d{1,2})[.\-/](\d{1,2}))");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            int y = std::stoi((*it)[1].str());
            int mo = std::stoi((*it)[2].str());
            int d = std::stoi((*it)[3].str());
            result += num2words_ko(y) + u8"년 " +
                      num2words_ko(mo) + u8"월 " +
                      num2words_ko(d) + u8"일";
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 3. Time patterns (AM/PM HH:MM) =====
    {
        // AM/PM prefix version
        std::regex re_ampm(R"((AM|PM|am|pm)\s*(\d{1,2}):(\d{2})(?=\s|$|[^\d]))");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re_ampm);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            std::string prefix = (*it)[1].str();
            int h = std::stoi((*it)[2].str());
            int mi = std::stoi((*it)[3].str());
            if (h > 24 || mi > 59) {
                result += it->str();
            } else {
                std::string ampm;
                if (prefix == "AM" || prefix == "am") ampm = u8"오전";
                else ampm = u8"오후";
                std::string hk = to_native_korean(h);
                std::string out;
                if (!ampm.empty()) out = ampm + " " + hk + u8"시";
                else out = hk + u8"시";
                if (mi > 0) {
                    out += " " + num2words_ko(mi) + u8"분";
                }
                result += out;
            }
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }
    {
        // No AM/PM — bare HH:MM followed by Korean particles
        // Simplified: match digit:digit patterns without lookbehind
        std::regex re_bare(u8R"((\d{1,2}):(\d{2}))");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re_bare);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            int h = std::stoi((*it)[1].str());
            int mi = std::stoi((*it)[2].str());
            if (h > 24 || mi > 59) {
                result += it->str();
            } else {
                std::string hk = to_native_korean(h);
                std::string out = hk + u8"시";
                if (mi > 0) {
                    out += " " + num2words_ko(mi) + u8"분";
                }
                result += out;
            }
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 4. Temperature (°C, °F) =====
    {
        std::regex re(u8R"((\d+\.?\d*)°([CcFf]))");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            std::string val = (*it)[1].str();
            char unit_ch = (*it)[2].str()[0];
            std::string unit = (unit_ch == 'C' || unit_ch == 'c') ? u8"도씨" : u8"도에프";
            result += val + unit;
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 5. Score/ratio (2:1 → 2 대 1) =====
    {
        std::regex re(R"((\d+):(\d+))");
        // Filter by not matching time patterns (already handled above));
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            result += (*it)[1].str() + u8" 대 " + (*it)[2].str();
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 6. Symbol conversion =====
    t = std::regex_replace(t, std::regex(R"(\+\s*(?=\d))"), u8"플러스 ");
    // Minus patterns (e.g. -5도, -10%) are edge cases — skip for now
    {
        // × → 곱하기
        std::string from = u8"×";
        size_t pos = 0;
        while ((pos = t.find(from, pos)) != std::string::npos) {
            t.replace(pos, from.size(), u8" 곱하기 ");
            pos += std::string(u8" 곱하기 ").size();
        }
    }
    {
        // ÷ → 나누기
        std::string from = u8"÷";
        size_t pos = 0;
        while ((pos = t.find(from, pos)) != std::string::npos) {
            t.replace(pos, from.size(), u8" 나누기 ");
            pos += std::string(u8" 나누기 ").size();
        }
    }
    {
        // = → 이퀄
        size_t pos = 0;
        while ((pos = t.find('=', pos)) != std::string::npos) {
            t.replace(pos, 1, u8" 이퀄 ");
            pos += std::string(u8" 이퀄 ").size();
        }
    }
    t = std::regex_replace(t, std::regex(R"(&)"), u8" 앤드 ");
    t = std::regex_replace(t, std::regex(R"(@)"), u8" 앳 ");
    t = std::regex_replace(t, std::regex(R"(~)"), u8" 에서 ");

    // ===== 7. Currency ($100 → 100달러) =====
    for (const auto& pu : prefix_unit_map()) {
        std::regex re(std::string("\\") + pu.symbol + R"(\s*(\d[\d,.]*))");
        // Need to use escaped symbol in regex
        std::string escaped_sym;
        for (char c : pu.symbol) {
            if (c == '$' || c == '(' || c == ')' || c == '[' || c == ']' ||
                c == '{' || c == '}' || c == '.' || c == '+' || c == '*' ||
                c == '?' || c == '\\' || c == '^' || c == '|') {
                escaped_sym += '\\';
            }
            escaped_sym += c;
        }
        // For multi-byte currency symbols, just use find-replace approach
        std::string result;
        size_t pos = 0;
        while (pos < t.size()) {
            size_t found = t.find(pu.symbol, pos);
            if (found == std::string::npos) {
                result += t.substr(pos);
                break;
            }
            result += t.substr(pos, found - pos);
            size_t after = found + pu.symbol.size();
            // Skip whitespace
            while (after < t.size() && t[after] == ' ') after++;
            // Read digits/commas/dots
            size_t num_start = after;
            while (after < t.size() && (std::isdigit((unsigned char)t[after]) ||
                   t[after] == ',' || t[after] == '.')) {
                after++;
            }
            if (after > num_start) {
                std::string num_str = t.substr(num_start, after - num_start);
                // Remove commas
                std::string clean;
                for (char c : num_str) {
                    if (c != ',') clean += c;
                }
                result += clean + pu.korean;
                pos = after;
            } else {
                result += pu.symbol;
                pos = found + pu.symbol.size();
            }
        }
        t = result;
    }

    // Remove commas in numbers (simple string replacement)
    {
        std::string result;
        for (size_t i = 0; i < t.size(); ++i) {
            if (t[i] == ',' && i > 0 && i + 3 < t.size() &&
                std::isdigit((unsigned char)t[i-1]) &&
                std::isdigit((unsigned char)t[i+1]) &&
                std::isdigit((unsigned char)t[i+2]) &&
                std::isdigit((unsigned char)t[i+3])) {
                continue;  // skip comma
            }
            result += t[i];
        }
        t = result;
    }

    // ===== 9. Decade N0대 (70대 → 칠십대) =====
    for (int n = 1; n <= 9; ++n) {
        std::string from = std::to_string(n) + u8"0대";
        std::string to = num2words_ko(n * 10) + u8"대";
        size_t pos = 0;
        while ((pos = t.find(from, pos)) != std::string::npos) {
            t.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    // ===== 10. Suffix units (100km → 100킬로미터) =====
    for (const auto& [unit, kor_unit] : suffix_unit_map()) {
        // Escape special regex chars in unit
        std::string escaped;
        for (char c : unit) {
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '(' ||
                c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
                c == '\\' || c == '^' || c == '$' || c == '|' || c == '/') {
                escaped += '\\';
            }
            escaped += c;
        }
        std::regex re("(\\d+\\.?\\d*)\\s*" + escaped + "(?![a-zA-Z])");
        t = std::regex_replace(t, re, "$1" + kor_unit);
    }

    // ===== 11. Decimal → Korean =====
    {
        std::regex re(R"(\d+\.\d+)");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            double val = std::stod(it->str());
            result += num2words_ko_float(val);
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 12. Native counters (3시→세시) =====
    {
        // Build counter alternation pattern (sorted by length desc)
        std::string counter_pat;
        for (const auto& c : native_counters()) {
            if (!counter_pat.empty()) counter_pat += "|";
            counter_pat += c;
        }
        std::regex re("(\\d+)(" + counter_pat + ")");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            int n = std::stoi((*it)[1].str());
            std::string counter = (*it)[2].str();
            result += to_native_korean(n) + counter;
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // ===== 13. Remaining integers → Korean (Sino) =====
    {
        std::regex re(R"(\d+)");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            int64_t val = 0;
            try { val = std::stoll(it->str()); } catch (...) {}
            result += num2words_ko(val);
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    return t;
}

// ── transliterate_english_fallback ───────────────────
std::string TextNormalizeKr::transliterate_english_fallback(const std::string& text) const {
    std::string t = text;

    // First apply known_words_ replacements (case-insensitive)
    for (const auto& [eng, kor] : known_words_) {
        std::regex re(std::regex_replace(eng, std::regex(R"([-[\]{}()*+?.,\\^$|#\s])"), R"(\$&)"),
                      std::regex_constants::icase);
        t = std::regex_replace(t, re, kor);
    }

    // Then replace remaining [a-zA-Z]+ → english_word_to_korean
    {
        std::regex re("[a-zA-Z]+");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            result += english_word_to_korean(it->str());
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    return t;
}

// ── english_word_to_korean ───────────────────────────
std::string TextNormalizeKr::english_word_to_korean(const std::string& word) const {
    // ① Exact case match
    {
        auto it = english_dictionary_exact_.find(word);
        if (it != english_dictionary_exact_.end()) return it->second;
    }
    // ② Uppercase fallback match
    {
        auto it = english_dictionary_.find(to_upper(word));
        if (it != english_dictionary_.end()) return it->second;
    }
    // ③ Letter-by-letter fallback
    std::string result;
    for (char c : word) {
        auto it = eng_alphabet().find((char)toupper(c));
        if (it != eng_alphabet().end()) {
            result += it->second;
        } else {
            result += c;
        }
    }
    return result;
}

// ── adversarial_spelling_ko ─────────────────────────
std::string TextNormalizeKr::adversarial_spelling_ko(const std::string& text) const {
    // Hardcoded replacements for corner cases
    static const std::vector<std::pair<std::string, std::string>> replacements = {
        {u8"공준",       u8"공 준"},
        {u8"쌍곡",       u8"쌍 곡"},
        {u8"동측내각",   u8"동측 내각"},
        {u8"고깔",       u8"고 깔"},
        {u8"유클리드",   u8"유 클리드"},
        {u8"르장드르",   u8"르 장드르"},
        {u8"로바쳅스키", u8"로바 쳅 스키"},
        {u8"사케리",     u8"사 케리"},
    };

    std::string t = text;
    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = t.find(from, pos)) != std::string::npos) {
            t.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    return t;
}

// ── adversarial_jongseong_ko ─────────────────────────
std::string TextNormalizeKr::adversarial_jongseong_ko(const std::string& text) const {
    std::string result;
    result.reserve(text.size());
    size_t pos = 0;
    while (pos < text.size()) {
        uint32_t cp = utf8_decode(text, pos);
        if (cp >= 0xAC00 && cp <= 0xD7A3) {
            uint32_t base = cp - 0xAC00;
            uint32_t cho = base / 588;
            uint32_t jung = (base % 588) / 28;
            uint32_t jong = base % 28;
            if (jong == 7) {  // ㄷ → ㅅ
                jong = 19;
                cp = 0xAC00 + cho * 588 + jung * 28 + jong;
            }
        }
        result += utf8_encode(cp);
    }
    return result;
}

// ── normalize_final ──────────────────────────────────
std::string TextNormalizeKr::normalize_final(const std::string& text) const {
    std::string t = text;

    // Remove CJK/Hanja chars (manual UTF-8 codepoint filtering)
    {
        std::string filtered;
        filtered.reserve(t.size());
        size_t pos = 0;
        while (pos < t.size()) {
            uint32_t cp = utf8_decode(t, pos);
            bool is_cjk = false;
            // CJK Unified Ideographs: 4E00-9FFF
            if (cp >= 0x4E00 && cp <= 0x9FFF) is_cjk = true;
            // CJK Extension A: 3400-4DBF
            else if (cp >= 0x3400 && cp <= 0x4DBF) is_cjk = true;
            // CJK Compatibility Ideographs: F900-FAFF
            else if (cp >= 0xF900 && cp <= 0xFAFF) is_cjk = true;
            // CJK Radicals Supplement: 2E80-2EFF
            else if (cp >= 0x2E80 && cp <= 0x2EFF) is_cjk = true;
            // Kangxi Radicals: 2F00-2FDF
            else if (cp >= 0x2F00 && cp <= 0x2FDF) is_cjk = true;
            // CJK Symbols: 3000-303F (keep some punctuation)
            else if (cp >= 0x3005 && cp <= 0x303F) is_cjk = true;

            if (!is_cjk) {
                filtered += utf8_encode(cp);
            }
        }
        t = filtered;
    }

    // Apply etc_dictionary
    t = normalize_with_dictionary(t, etc_dictionary_);

    // Apply english_dictionary for remaining [A-Za-z]+
    {
        std::regex re("[A-Za-z]+");
        std::string result;
        std::sregex_iterator it(t.begin(), t.end(), re);
        std::sregex_iterator end;
        size_t last = 0;
        for (; it != end; ++it) {
            result += t.substr(last, it->position() - last);
            std::string upper = to_upper(it->str());
            auto dit = english_dictionary_.find(upper);
            if (dit != english_dictionary_.end()) {
                result += dit->second;
            } else {
                result += it->str();
            }
            last = it->position() + it->length();
        }
        result += t.substr(last);
        t = result;
    }

    // Lowercase
    t = to_lower(t);

    return t;
}

// ── normalize_with_dictionary ────────────────────────
std::string TextNormalizeKr::normalize_with_dictionary(
        const std::string& text,
        const std::unordered_map<std::string, std::string>& dict) const {
    if (dict.empty()) return text;

    // Quick check: does any key exist in text?
    bool found_any = false;
    for (const auto& [key, val] : dict) {
        if (text.find(key) != std::string::npos) {
            found_any = true;
            break;
        }
    }
    if (!found_any) return text;

    // Build combined regex from dict keys
    std::string pattern;
    for (const auto& [key, val] : dict) {
        if (!pattern.empty()) pattern += "|";
        // Escape regex special chars
        for (char c : key) {
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '(' ||
                c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
                c == '\\' || c == '^' || c == '$' || c == '|') {
                pattern += '\\';
            }
            pattern += c;
        }
    }

    std::regex re(pattern);
    std::string result;
    std::sregex_iterator it(text.begin(), text.end(), re);
    std::sregex_iterator end;
    size_t last = 0;
    for (; it != end; ++it) {
        result += text.substr(last, it->position() - last);
        auto dit = dict.find(it->str());
        if (dit != dict.end()) {
            result += dit->second;
        } else {
            result += it->str();
        }
        last = it->position() + it->length();
    }
    result += text.substr(last);
    return result;
}

// ── to_upper / to_lower ──────────────────────────────
std::string TextNormalizeKr::to_upper(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        if (c >= 'a' && c <= 'z') c -= 32;
    }
    return r;
}

std::string TextNormalizeKr::to_lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        if (c >= 'A' && c <= 'Z') c += 32;
    }
    return r;
}

}  // namespace snap
