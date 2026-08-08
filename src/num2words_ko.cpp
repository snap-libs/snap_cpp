/**
 * num2words_ko — Korean number-to-words conversion
 * ==================================================
 * Sino-Korean (한자어): 32 → "삼십이"
 * Native Korean (고유어): 3 → "세" (for counters)
 */

#include "snap/num2words_ko.h"
#include <cmath>
#include <sstream>
#include <cstdlib>

namespace snap {

// ── Sino-Korean digits ──────────────────────────────
static const char* DIGITS[] = {
    "", "\xec\x9d\xbc", "\xec\x9d\xb4", "\xec\x82\xbc",    // 일이삼
    "\xec\x82\xac", "\xec\x98\xa4", "\xec\x9c\xa1",          // 사오육
    "\xec\xb9\xa0", "\xed\x8c\x94", "\xea\xb5\xac"           // 칠팔구
};

// ── Place value units within a 4-digit group ────────
// 십(10), 백(100), 천(1000)
static const char* PLACE_UNITS[] = {
    "",
    "\xec\x8b\xad",       // 십
    "\xeb\xb0\xb1",       // 백
    "\xec\xb2\x9c"        // 천
};

// ── Large group units: 만, 억, 조, 경 ───────────────
static const char* GROUP_UNITS[] = {
    "",
    "\xeb\xa7\x8c",       // 만
    "\xec\x96\xb5",       // 억
    "\xec\xa1\xb0",       // 조
    "\xea\xb2\xbd"        // 경
};

/// Convert a 4-digit group (0-9999) to Korean
static std::string group_to_korean(int n) {
    if (n == 0) return "";
    std::string result;
    int digits[4];
    digits[3] = n / 1000; n %= 1000;
    digits[2] = n / 100;  n %= 100;
    digits[1] = n / 10;   n %= 10;
    digits[0] = n;

    for (int i = 3; i >= 0; --i) {
        if (digits[i] == 0) continue;
        // Drop leading 일 for 십/백/천 (but keep for ones place)
        if (digits[i] == 1 && i > 0) {
            result += PLACE_UNITS[i];
        } else {
            result += DIGITS[digits[i]];
            result += PLACE_UNITS[i];
        }
    }
    return result;
}

std::string num2words_ko(int64_t n) {
    if (n == 0) return "\xec\x98\x81";  // 영

    std::string result;
    bool negative = false;
    if (n < 0) {
        negative = true;
        n = -n;
    }

    // Split into 4-digit groups from right
    // groups[0] = ones group, groups[1] = 만 group, etc.
    int groups[5] = {0};
    int num_groups = 0;
    int64_t tmp = n;
    while (tmp > 0 && num_groups < 5) {
        groups[num_groups++] = (int)(tmp % 10000);
        tmp /= 10000;
    }

    // Build from most significant group
    bool first_group = true;
    for (int i = num_groups - 1; i >= 0; --i) {
        if (groups[i] == 0) continue;
        std::string g = group_to_korean(groups[i]);
        // For 만+ groups, drop 일 if the group is exactly 1
        // e.g. 10000 → "만" not "일만"
        if (i >= 1 && groups[i] == 1) {
            g = "";  // just the unit
        }
        if (!first_group) {
            result += " ";
        }
        result += g;
        result += GROUP_UNITS[i];
        first_group = false;
    }

    if (negative) {
        result = "\xeb\xa7\x88\xec\x9d\xb4\xeb\x84\x88\xec\x8a\xa4 " + result;  // 마이너스
    }
    return result;
}

std::string num2words_ko_float(double val) {
    if (val < 0) {
        return "\xeb\xa7\x88\xec\x9d\xb4\xeb\x84\x88\xec\x8a\xa4 " +  // 마이너스
               num2words_ko_float(-val);
    }

    int64_t int_part = (int64_t)val;
    std::string result = num2words_ko(int_part);

    // Get decimal part as string
    std::ostringstream oss;
    oss << val;
    std::string s = oss.str();
    auto dot_pos = s.find('.');
    if (dot_pos != std::string::npos) {
        std::string frac = s.substr(dot_pos + 1);
        // Remove trailing zeros
        while (!frac.empty() && frac.back() == '0') frac.pop_back();
        if (!frac.empty()) {
            result += " \xec\xa0\x90 ";  // 점
            // Read each digit individually
            for (char c : frac) {
                int d = c - '0';
                if (d >= 0 && d <= 9) {
                    if (d == 0)
                        result += "\xec\x98\x81";  // 영
                    else
                        result += DIGITS[d];
                }
            }
        }
    }
    return result;
}

// ── Native Korean numerals (고유어 수사) ─────────────

struct NativeEntry { int n; const char* kr; };
static const NativeEntry NATIVE_NUMS[] = {
    {1,  "\xed\x95\x9c"},     // 한
    {2,  "\xeb\x91\x90"},     // 두
    {3,  "\xec\x84\xb8"},     // 세
    {4,  "\xeb\x84\xa4"},     // 네
    {5,  "\xeb\x8b\xa4\xec\x84\xaf"},   // 다섯
    {6,  "\xec\x97\xac\xec\x84\xaf"},   // 여섯
    {7,  "\xec\x9d\xbc\xea\xb3\xb1"},   // 일곱
    {8,  "\xec\x97\xac\xeb\x8d\x9f"},   // 여덟
    {9,  "\xec\x95\x84\xed\x99\x89"},   // 아홉
    {10, "\xec\x97\xb4"},     // 열
    {20, "\xec\x8a\xa4\xeb\xac\xbc"},   // 스물
    {30, "\xec\x84\x9c\xeb\xa5\xb8"},   // 서른
    {40, "\xeb\xa7\x88\xed\x9d\x94"},   // 마흔
    {50, "\xec\x89\xb0"},     // 쉰
    {60, "\xec\x98\x88\xec\x8a\x9c"},   // 예순
    {70, "\xec\x9d\xbc\xed\x9d\x94"},   // 일흔
    {80, "\xec\x97\xac\xeb\x93\xa0"},   // 여든
    {90, "\xec\x95\x84\xed\x9d\x94"},   // 아흔
    {0, nullptr}
};

static const char* find_native(int n) {
    for (int i = 0; NATIVE_NUMS[i].kr; ++i)
        if (NATIVE_NUMS[i].n == n) return NATIVE_NUMS[i].kr;
    return nullptr;
}

std::string to_native_korean(int n) {
    const char* direct = find_native(n);
    if (direct) return direct;

    if (n >= 11 && n <= 99) {
        int tens = (n / 10) * 10;
        int ones = n % 10;
        const char* t = find_native(tens);
        const char* o = find_native(ones);
        if (t && o) {
            return std::string(t) + o;
        }
    }
    return num2words_ko(n);
}

std::string to_native_korean_count(int n) {
    if (n <= 0 || n >= 100) return num2words_ko(n);
    if (n == 20) return "\xec\x8a\xa4\xeb\xac\xb4";  // 스무 (스무 개)
    if (n >= 21 && n <= 29) {
        static const char* count_ones[] = {"", "\xed\x95\x9c", "\xeb\x91\x90", "\xec\x84\xb8", "\xeb\x84\xa4", "\xeb\x8b\xa4\xec\x84\xaf", "\xec\x97\xac\xec\x84\xaf", "\xec\x9d\xbc\xea\xb3\xb1", "\xec\x97\xac\xeb\x8d\x9f", "\xec\x95\x84\xed\x99\x89"};
        int ones = n % 10;
        return std::string("\xec\x8a\xa4\xeb\xac\xbc") + count_ones[ones]; // 스물한, 스물두...
    }
    return to_native_korean(n);
}


}  // namespace snap
