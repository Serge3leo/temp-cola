// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-05-02 08:28:06 - Создан.
//
// Функции `charconv_repair::imp::from_chars()` и
// `charconv_repair::imp::to_chars()` на основе
// `strtold()/strtoflt128()/snprintf()/quadmath_snprintf()` для дополнения
// реализации `std::float128_t` g++ (неполна везде, кроме linux [TODO] glibc) и
// замены некорректной реализации `long double` clang++ libc++.
//
// Концепции `charconv_repair::has_std_from_chars<T>` и
// `charconv_repair::has_std_from_chars<T>` проверяют наличие корректных
// функций `std::from_chars()/std::to_chars()`.
//
// Функции `charconv_repair::from_chars()` и `charconv_repair::to_chars()`
// совпадают, либо с функциями `std::`, либо с функциями `charconv_repair::`.
//
// Концепции `charconv_repair::has_from_chars<T>` и
// `charconv_repair::has_from_chars<T>` истинны, если есть реализация
// `charconv_repair::from_chars()/to_chars()`.
//
// TODO: Реализация `to_chars()` без аргумента `precision` неэффективна, т.к.
// тупо проверяет совпадение обратного преобразования в цикле увеличения
// точности.

#ifndef CHARCONV_REPAIR_HPP_4248
#define CHARCONV_REPAIR_HPP_4248

#include <cassert>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif
#include <string>

#if __has_include(<quadmath.h>)
    #include <quadmath.h>  // Need: -lquadmath
#endif

namespace charconv_repair { namespace imp {
    template<class F>
    struct f_t {
        static F ceil(F num) { return std::ceil(num); }
        static F floor(F num) { return std::floor(num); }
        static F log10(F num) { return std::log10(num); }
        static F nextafter(F x, F y) { return std::nextafter(x, y); }
        static long double strtof_(const char *s, const char **sp) {
            return strtold(s, const_cast<char **>(sp));
        }
        static int snprintf_(char *s, size_t size, const char *format,
                             int precision, F value) {
            return std::snprintf(s, size, format, precision, value);
        }
        static constexpr const bool nL_ = (sizeof(F) <= sizeof(double));
        static constexpr const char *fmt_(std::chars_format fmt) {
            return (std::chars_format::fixed == fmt ? (nL_ ? "%.*f" : "%.*Lf")
               :std::chars_format::general == fmt ? (nL_ ? "%.*g" : "%.*Lg")
               :std::chars_format::hex == fmt ? (nL_ ? "%.*a" : "%.*La")
               :std::chars_format::scientific == fmt ? (nL_ ? "%.*e" : "%.*Le")
               :nullptr);
        }
    };
    #if defined(__STDCPP_FLOAT128_T__) && __has_include(<quadmath.h>)
        template<> struct f_t<std::float128_t> {
            static std::float128_t ceil(std::float128_t num) {
                return ceilq(num);
            }
            static std::float128_t floor(std::float128_t num) {
                return floorq(num);
            }
            static std::float128_t log10(std::float128_t num) {
                return log10q(num);
            }
            static std::float128_t nextafter(std::float128_t x,
                                             std::float128_t y) {
                return nextafterq(x, y);
            }
            static std::float128_t strtof_(const char *s, const char **sp) {
                return strtoflt128(s, const_cast<char **>(sp));
            }
            static int snprintf_(char *s, size_t size, const char *format,
                                 int precision, std::float128_t value) {
                return quadmath_snprintf(s, size, format, precision, value);
            }
            static constexpr const char *fmt_(std::chars_format fmt) {
                return (std::chars_format::fixed == fmt ? "%.*Qf"
                       :std::chars_format::general == fmt ? "%.*Qg"
                       :std::chars_format::hex == fmt ? "%.*Qa"
                       :std::chars_format::scientific == fmt ? "%.*Qe"
                       :nullptr);
            }
        };
    #endif
    template <class F>
    std::from_chars_result
    from_chars(const char *first, const char *last, F &value,
               std::chars_format fmt = std::chars_format::general) noexcept {
#if 0
        const bool hex = (fmt == std::chars_format::hex);
        std::string s = (hex ? "0x" : "") + std::string(first, last);
        const char *start = s.c_str() + 2*hex;
#else
        auto s = first;
        auto start = s;
        std::string ss;
        if (fmt == std::chars_format::hex) {
            ss = "0x" + std::string(first, last);
            s = ss.c_str();
            start = s + 2;
        }
#endif
        auto sp = start;
        auto v = f_t<F>::strtof_(s, &sp);
        if (sp > start) {  // TODO: HUGE_VALQ ?
            value = v;
            return {first + (sp - start), std::errc{}};
        }
        return {first, std::errc::invalid_argument};
    }
    template <class F>
    std::to_chars_result
    to_chars(char* first, char* last, F value,
             std::chars_format fmt = std::chars_format::general,
             int precision = -1) noexcept {
        const char *f = f_t<F>::fmt_(fmt);
        if (!f) {
            return {first, std::errc::invalid_argument};
        }
        int pprecision = precision;
        int pwidth = last - first;
        if (-1 == precision) {
            // TODO: Shortest representation: smallest number of chars
            // pprecision = ... ; len = ... ; etc...
            switch (fmt) {
             case std::chars_format::fixed:
                {
                    F l10v = f_t<F>::log10(value);
                    pprecision = std::max(0,
                                    std::numeric_limits<F>::max_digits10 -
                                    int(f_t<F>::floor(l10v)));
                    pwidth = 3 + size_t(f_t<F>::ceil(l10v)) + pprecision;
                }
                break;
             case std::chars_format::scientific:
             case std::chars_format::general:
                pprecision = std::numeric_limits<F>::max_digits10;
                pwidth = 8 + pprecision;
                break;
             case std::chars_format::hex:
                pprecision = std::numeric_limits<F>::digits/4;
                pwidth = 11 + pprecision;
                break;
             default:
                return {first, std::errc::invalid_argument};
            }
        }
        std::string ss(pwidth + 1, '\0');
        int len = f_t<F>::snprintf_(ss.data(), ss.size(), f, pprecision, value);
        if (0 < len && len <= pwidth) {
            if (-1 == precision) {
                #if 0  //ndef NDEBUG
                    F nexts[] = {f_t<F>::nextafter(value, -INFINITY),
                                 f_t<F>::nextafter(value, INFINITY)};
                    std::string snext(len + pprecision + 1, '\0');
                    for (const auto& next : nexts) {
                        int slen = quadmath_snprintf(snext.data(), snext.size(),
                                                     f, pprecision, next);
                        assert(0 < slen && slen <= int(snext.size()));
                        assert(std::string(ss.data(), len)
                               != std::string(snext.data(), slen));
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
}}
namespace charconv_repair {
    template <class T>
    concept has_std_from_chars =
    requires(const char* first, const char* last, T& x) {
        {std::from_chars(first, last, x)} ->
                std::same_as<std::from_chars_result>;
    };
    template <class T>
    concept has_std_to_chars =
        #if defined(__clang__) && defined(_LIBCPP_VERSION)
            (!std::is_same_v<T, long double> ||
             std::numeric_limits<long double>::digits ==
             std::numeric_limits<double>::digits) &&
        #endif
    requires(char* first, char* last, T x) {
        {std::to_chars(first, last, x)} -> std::same_as<std::to_chars_result>;
    };
    template <has_std_from_chars F>
    std::from_chars_result
    from_chars(const char *first, const char *last, F &value,
               std::chars_format fmt = std::chars_format::general) {
        return std::from_chars(first, last, value, fmt);
    }
    template <class F>
    std::from_chars_result
    from_chars(const char *first, const char *last, F &value,
               std::chars_format fmt = std::chars_format::general) noexcept {
        return imp::from_chars(first, last, value, fmt);
    }
    template <has_std_to_chars F>
    std::to_chars_result
    to_chars(char* first, char* last, F value,
             std::chars_format fmt = std::chars_format::general,
             int precision = -1) {
        return std::to_chars(first, last, value, fmt, precision);
    }
    template <class F>
    std::to_chars_result
    to_chars(char* first, char* last, F value,
             std::chars_format fmt = std::chars_format::general,
             int precision = -1) noexcept {
        return imp::to_chars(first, last, value, fmt, precision);
    }
    template <class F>
    concept has_from_chars = std::is_class_v<imp::f_t<F>>;
    template <class F>
    concept has_to_chars = std::is_class_v<imp::f_t<F>>;
}
#endif  // CHARCONV_REPAIR_HPP_4248
