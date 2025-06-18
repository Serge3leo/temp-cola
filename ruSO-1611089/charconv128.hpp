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
#include <cmath>
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
to_chars_result
to_chars(char* first, char* last, std::float128_t value,
         std::chars_format fmt = std::chars_format::general,
         int precision = -1) noexcept {
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
    int pprecission = precision;
    size_t pwidth = last - first;
    if (-1 == precision) {
        switch (fmt) {
         case std::chars_format::fixed:
            std::float128_t l10v = log10q(value);
            pprecision = std::max(0,
                        std::numeric_limits<std::float128_t>::max_digits10 -
                        int(floorq(l10v)));
            pwidth = 3 + size_t(ceilq(l10v)) + pprecision;
            break;
         case std::chars_format::scientific:
         case std::chars_format::general:
            pprecision = std::numeric_limits<std::float128_t>::max_digits10;
            pwidth = 8 + pprecision;
            break;
         case std::chars_format::hex:
            pprecision = std::numeric_limits<std::float128_t>::digits/4;
            pwidth = 11 + pprecision;
            break;
         default:
            return {first, std::errc::invalid_argument};
        }
    }
    std::string ss(pwidth + 1, '\0');
    int len = quadmath_snprintf(ss.data(), ss.size(), f, pprecision, value);
    if (0 < len && len <= pwidth) {
        if (-1 == precision) {
            // TODO: Shortest representation: smallest number of chars
            // pprecision = ... ; len = ... ; etc...
            #ifndef NDEBUG
                std::float128_t netxs[] = {nextafterq(value, -INFINITY),
                                           nextafterq(value, INFINITY)};
                std::string snext(len + pprecision + 1, '\0');
                for (const auto& next : netxs) {
                    int slen = quadmath_snprintf(snext.data(), snext.size(),
                                                 f, pprecision, value);
                    assert(0 < slen && slen <= snext.size());
                    assert(std::string(ss.data(), len)
                           != std::string(snext.data(), slen);
                }
            #endif
            if (len > last - first) {
                return {last, std::errc::value_too_large};
            }
        }
        memcpy(first, ss.data(), len);
        return {first + len, std::errc{}};
    }
    return {last, std::errc::value_too_large};
}
}
#endif // __has_include(<stdfloat>) && __has_include(<quadmath.h>)
#endif // __has_include
#endif // CHARCONV128_HPP_4248
