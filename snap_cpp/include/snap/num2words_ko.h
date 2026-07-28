#pragma once
#include <string>
#include <cstdint>

namespace snap {

/// Convert an integer to Korean Sino-Korean reading (한자어 수사)
/// e.g. 32 → "삼십이", 2024 → "이천이십사", 100000 → "십만"
std::string num2words_ko(int64_t n);

/// Convert a float to Korean reading
/// e.g. 3.14 → "삼 점 일사", -2.5 → "마이너스 이 점 오"
std::string num2words_ko_float(double val);

/// Convert an integer to native Korean numeral (고유어 수사)
/// Used for counters: 3시→세시, 5개→다섯개
/// Range: 1-59. Falls back to Sino-Korean for out of range.
std::string to_native_korean(int n);

}  // namespace snap
