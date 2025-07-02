// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-05-02 03:34:01 - Создан.
//

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <charconv>
#include <complex>
#include <iostream>
#include <limits>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif
#include <string>

#if !defined(__GLIBC__)
    #include "charconv128.hpp"
#endif

#define LITERAL128_METHOD (3)
#ifdef __STDCPP_FLOAT128_T__
    constexpr
    std::complex<std::float128_t> operator""_i128(unsigned long long ull) {
        return std::complex<std::float128_t>{0, std::float128_t(ull)};
    }
    #if 1 == LITERAL128_METHOD
        constexpr
        std::complex<std::float128_t> operator""_i128(long double ld) {
            static_assert(std::numeric_limits<std::float128_t>::digits
                          <= std::numeric_limits<long double>::digits);
            return std::complex<std::float128_t>{0, std::float128_t(ld)};
        }
    #elif 2 == LITERAL128_METHOD
        /* constexpr */
        std::complex<std::float128_t> operator""_i128(const char *s) {
            std::string ss(s);
            ss.erase(std::remove(ss.begin(), ss.end(), '\''), ss.end());
            std::float128_t imag;
            if (ss.starts_with("0x") || ss.starts_with("0X")) {
                std::ignore = std::from_chars(ss.data() + 2,
                        ss.data() + ss.size(), imag, std::chars_format::hex);
            } else {
                std::ignore = std::from_chars(ss.data(),
                        ss.data() + ss.size(), imag);
            }
            return std::complex<std::float128_t>{0, imag};
        }
    #elif 3 == LITERAL128_METHOD
        template <char... C>
        /* constexpr */
        std::complex<std::float128_t> operator""_i128() {
            char c[] = { C... };
            char *last = c;
            for (const char *i = c; i < c + sizeof(c); i++) {
                if (*i != '\'') {
                    *last++ = *i;
                }
            }
            std::float128_t imag;
            if ('0' == c[0] && ('x' == c[1] || 'X' == c[1])) {
                std::ignore = std::from_chars(c + 2, last, imag,
                                              std::chars_format::hex);
            } else {
                std::ignore = std::from_chars(c, last, imag);
            }
            return std::complex<std::float128_t>{0, imag};
        }
    #elif 4 == LITERAL128_METHOD
        constexpr std::complex<std::float128_t> operator""_i128(const char *s) {
            return std::operator""f128(s);
        }
    #elif 5 == LITERAL128_METHOD
        template <char... C>
        constexpr std::complex<std::float128_t> operator""_i128() {
            return std::operator""f128<C...>();
        }
    #else
        #error "LITERAL128_METHOD"
    #endif
    void test_complex_float128() {
        assert((std::complex<std::float128_t>{0, 1} == 1_i128));
        assert((std::complex<std::float128_t>{0, 1} == 1._i128));
        assert((std::complex<std::float128_t>{0, 0.1f128} == 0.1_i128));
        assert((std::complex<std::float128_t>{0, -0xfeP1f128} == -0xfeP1_i128));
    }
#endif

#define LITERAL64_METHOD (2)
#ifdef __STDCPP_FLOAT64_T__
    #if 1 == LITERAL64_METHOD
        static_assert(std::numeric_limits<std::float64_t>::digits
                      == std::numeric_limits<double>::digits);
        constexpr
        std::complex<std::float64_t> operator""_i64(unsigned long long ull) {
            return std::operator""i(ull);
        }
        constexpr
        std::complex<std::float64_t> operator""_i64(long double ld) {
            return std::operator""i(ld);
        }
    #elif 2 == LITERAL64_METHOD
        constexpr
        std::complex<std::float64_t> operator""_i64(unsigned long long ull) {
            return std::complex<std::float64_t>{0, std::float64_t(ull)};
        }
        constexpr
        std::complex<std::float64_t> operator""_i64(long double ld) {
            static_assert(std::numeric_limits<std::float64_t>::digits
                          <= std::numeric_limits<long double>::digits);
            return std::complex<std::float64_t>{0, std::float64_t(ld)};
        }
    #else
        #error "LITERAL64_METHOD"
    #endif
    static_assert(std::complex<std::float64_t>{0, 1} == 1_i64);
    static_assert(std::complex<std::float64_t>{0, 1} == 1._i64);
    static_assert(std::complex<std::float64_t>{0, 0.1} == 0.1_i64);
#endif

int main()
{
    #ifdef __STDCPP_FLOAT128_T__
        test_complex_float128();
        std::cout << "test_complex_float128()\n";
    #else
        std::cout << "No std::complex<std::float128_t>\n";
    #endif
    #ifndef __STDCPP_FLOAT64_T__
        std::cout << "No std::complex<std::float64_t>\n";
    #endif
    return 0;
}
