// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-05-02 08:28:06 - Создан.
//

#ifndef CHARCONV128_HPP_4248
#define CHARCONV128_HPP_4248

#ifdef __has_include
#if __has_include(<stdfloat>) && __has_include(<quadmath.h>)
#include <charconv>
#include <cstring>
#include <limits>
#include <stdfloat>
#include <string>

#include <quadmath.h>  // Need: -lquadmath

namespace std {
from_chars_result
from_chars(const char *first, const char *last, std::float128_t &value,
           std::chars_format fmt = std::chars_format::general) noexcept {
    const bool hex = (fmt == std::chars_format::hex);
    std::string s = (hex ? "0x" : "") + std::string(first, last);
    const char *start = s.c_str() + 2*hex;
    char *sp = const_cast<char *>(start);
    value = strtoflt128(s.c_str(), &sp);
    return (sp > start ? from_chars_result{first + (sp - start), std::errc{}}
                       : from_chars_result{first, std::errc::invalid_argument});
}
template <class T>
int cc128_calculate_precision_(T value, std::chars_format) {
    // TODO: smallest precision for recovers value exactly by from_chars()
    return std::numeric_limits<T>::max_digits10 - 1;
}
to_chars_result
to_chars(char* first, char* last, std::float128_t value,
         std::chars_format fmt = std::chars_format::general,
         int precision = 0) noexcept {
    const char *f;
    switch (fmt) {
     case std::chars_format::fixed:
        f = "%.*Qf";
        break;
     case std::chars_format::scientific:
        f = "%.*Qe";
        break;
     case std::chars_format::general:
        f = "%.*Qg";
        break;
     case std::chars_format::hex:
        f = "%.*Qa";
        break;
     default:
        return {first, std::errc::invalid_argument};
    }
    if (0 >= precision) {
        if (std::chars_format::hex == fmt) {
            precision = std::numeric_limits<std::float128_t>::digits/4;
        } else {
            precision = cc128_calculate_precision_(value, fmt);
        }
    }
    std::string ss(last - first + 1, '\0');
    int len = quadmath_snprintf(ss.data(), ss.size(), f, precision, value);
    if (0 < len && len <= last - first) {
        memcpy(first, ss.data(), len);
        return {first + len, std::errc{}};
    }
    return {first, std::errc::invalid_argument};
}
}
#endif // __has_include(<stdfloat>) && __has_include(<quadmath.h>)
#endif // __has_include
#endif // CHARCONV128_HPP_4248
