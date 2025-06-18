// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-05-01 08:40:09 - Создан.
//

#define __STDC_WANT_IEC_60559_BFP_EXT__ (1)  // for strfrom*() family
#if !defined(__linux__)  // glibc define to_chars/from_chars for float128_t
    #include "charconv128.hpp"
#endif

#include <cassert>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <iostream>
#include <limits>
#include <ranges>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif
#include <string>
#include <typeinfo>

// Use Catch2: https://github.com/catchorg/Catch2/
// Header-only (without installing etc.), see:
// https://github.com/catchorg/Catch2/tree/v2.x/single_include/catch2
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std;

template <class T>
concept has_from_chars =
requires(const char* first, const char* last, T& x) {
    {std::from_chars(first, last, x)} -> std::same_as<std::from_chars_result>;
};
template <class T>
concept has_to_chars =
    #if defined(__clang__) && defined(_LIBCPP_VERSION)
        // https://github.com/llvm/llvm-project/issues/62282
        // See P0067R5 in https://libcxx.llvm.org/Status/Cxx17.html
        (!std::is_same_v<T, long double> ||
         std::numeric_limits<long double>::digits ==
         std::numeric_limits<double>::digits) &&
    #endif
requires(char* first, char* last, T x) {
    {std::to_chars(first, last, x)} -> std::same_as<std::to_chars_result>;
};
template <class T>
struct fix {
    static const int integral = 1 + std::numeric_limits<T>::max_exponent10;
    static const int fractional = 1 - std::numeric_limits<T>::min_exponent
                             + std::numeric_limits<T>::digits;
    static const int check = 7;  // > 5𝛔
    static const int precision = fractional + check;
    static const int width = 1 + integral + 1 + precision;
    using fmt_t = std::conditional_t<
                      std::numeric_limits<T>::digits
                                <= std::numeric_limits<float>::digits,
                      float,
                  std::conditional_t<
                      std::numeric_limits<T>::digits
                                <= std::numeric_limits<double>::digits,
                      double,
                  std::conditional_t<
                      std::numeric_limits<T>::digits
                                <= std::numeric_limits<long double>::digits,
                      long double, void>>>;
    static constexpr const char *ofmt =
                (std::is_same_v<float, fmt_t> ? "%.*f" :
                 std::is_same_v<double, fmt_t> ? "%.*lf" :
                 std::is_same_v<long double, fmt_t> ? "%.*Lf" : nullptr);
    static constexpr const char *ifmt =
                (std::is_same_v<float, fmt_t> ? "%f" :
                 std::is_same_v<double, fmt_t> ? "%lf" :
                 std::is_same_v<long double, fmt_t> ? "%Lf" : nullptr);
    static constexpr const bool has_snprintf = ofmt;
    static string test_snprintf(const T x) {
        const auto check_ = check;
        char buf[width + 1];
        int len = snprintf(buf, sizeof(buf), ofmt, precision, x);
        REQUIRE(check_ <= len);
        REQUIRE(string(check_, '0') == string(buf + len - check_, buf + len));
        return string(buf);
    }
    #if __STDC_IEC_60559_BFP__
        static constexpr const bool has_strfromT = true;
        static string test_strfromT(const T x) {
            char fmt[16];
            std::ignore = snprintf(fmt, sizeof(fmt), "%%.%df", precision);
            char buf[width + 1];
            int len;
            if constexpr (std::is_same_v<float, fmt_t>) {
                len = strfromf(buf, sizeof(buf), fmt, fmt_t(x));
            } else if constexpr (std::is_same_v<double, fmt_t>) {
                len = strfromd(buf, sizeof(buf), fmt, fmt_t(x));
            } else if constexpr (std::is_same_v<long double, fmt_t>) {
                len = strfroml(buf, sizeof(buf), fmt, fmt_t(x));
            } else if constexpr (std::is_same_v<void, fmt_t>) {
                len = strfromf128(buf, sizeof(buf), fmt, x);
            }
            REQUIRE(size_t(len) < sizeof(buf));
            return string(buf, size_t(len));
        }
    #else
        static constexpr const bool has_strfromT = has_snprintf;
        static string test_strfromT(const T x) { return test_snprintf(x); }
    #endif
    #if __STDC_IEC_60559_BFP__
        static constexpr const bool has_strtoT = true;
    #else
        static constexpr const bool has_strtoT = !std::is_same_v<void, fmt_t>;
    #endif
    static T test_strtoT(const char* str, char**str_end) {
        if constexpr (std::is_same_v<float, fmt_t>) {
            return std::strtof(str, str_end);
        } else if constexpr (std::is_same_v<double, fmt_t>) {
            return std::strtod(str, str_end);
        } else if constexpr (std::is_same_v<long double, fmt_t>) {
            return std::strtold(str, str_end);
        #if __STDC_IEC_60559_BFP__
            } else if constexpr (std::is_same_v<void, fmt_t>) {
                return strtof128(str, str_end);
        #endif
        }
        return NAN;
    }
    static string test_to_chars(T x) {
        const auto check_ = check;
        char buf[width];
        auto res = std::to_chars(buf, buf + sizeof(buf), x,
                                 std::chars_format::fixed, precision);
        REQUIRE(std::errc{} == res.ec);
        REQUIRE(check_ <= res.ptr - buf);
        REQUIRE(string(check_, '0') == string(res.ptr - check_, res.ptr));
        return string(buf, res.ptr);
    }
    static T test_from_chars(const string& buf) {
        T x;
        auto res = std::from_chars(buf.data(), buf.data() + buf.size(), x);
        REQUIRE(std::errc{} == res.ec);
        REQUIRE(ssize_t(buf.size()) == res.ptr - buf.data());
        return x;
    }
};
template <class T>
struct t_snprintf {
    using type = T;
    static constexpr bool skip() {
        return !fix<T>::has_strfromT || !fix<T>::has_snprintf;
    }
    void test(const T x) {
        std::string p = fix<T>::test_strfromT(x);
        std::string t = fix<T>::test_snprintf(x);
        CHECK(p == t);
    }
};
template <class T>
struct t_sscanf {
    using type = T;
    static constexpr bool skip() {
        #ifdef __FreeBSD__
            // man sscanf: BUGS: Numerical strings are truncated to 512 chars
            return (!fix<T>::has_strfromT || !fix<T>::ifmt ||
                    std::string("%Lf") == std::string(fix<T>::ifmt));
        #else
            return !fix<T>::has_strfromT || !fix<T>::ifmt;
        #endif
    }
    void test(const T x) {
        std::string p = fix<T>::test_strfromT(x);
        typename fix<T>::fmt_t fx;
        REQUIRE(1 == sscanf(p.c_str(), fix<T>::ifmt, &fx));
        CHECK(x == T(fx));
    }
};
template <class T>
struct t_strtoT {
    using type = T;
    static constexpr bool skip() {
        return !fix<T>::has_strfromT || !fix<T>::has_strtoT;
    }
    void test(const T x) {
        std::string p = fix<T>::test_strfromT(x);
        T sx = fix<T>::test_strtoT(p.c_str(), nullptr);
        CHECK(x == sx);
    }
};
template <class T>
struct t_to_chars {
    using type = T;
    static constexpr bool skip() {
        return !fix<T>::has_strfromT || !has_to_chars<T>;
    }
    void test(const T x) {
        std::string p = fix<T>::test_strfromT(x);
        std::string t = fix<T>::test_to_chars(x);
        CHECK(p == t);
    }
};
template <class T>
struct t_from_chars {
    using type = T;
    static constexpr bool skip() {
        return !fix<T>::has_strfromT || !has_from_chars<T>;
    }
    void test(const T x) {
        std::string p = fix<T>::test_strfromT(x);
        T fx = fix<T>::test_from_chars(p);
        CHECK(x == fx);
    }
};
template <class T>
struct t_to_from_chars {
    using type = T;
    static constexpr bool skip() {
        return !has_to_chars<T> || !has_from_chars<T>;
    }
    void test(const T x) {
        std::string t = fix<T>::test_to_chars(x);
        T fx = fix<T>::test_from_chars(t);
        CHECK(x == fx);
    }
};
TEMPLATE_PRODUCT_TEST_CASE("to_from_chars", "[to_chars][from_chars]",
            (t_snprintf, t_sscanf, t_strtoT,
             t_to_chars, t_from_chars, t_to_from_chars),
            #ifdef __STDCPP_FLOAT128_T__
                (float, double, long double, std::float128_t)
            #else
                (float, double, long double)
            #endif
            ) {
    TestType tt;
    using T = TestType::type;
    if constexpr (!TestType::skip()) {
        T tcs[] = { std::numeric_limits<T>::max(),
                    std::numeric_limits<T>::min(),
                    std::numeric_limits<T>::denorm_min()};
        for (const auto& tc : tcs) {
            tt.test(tc);
            tt.test(-tc);
            if (tc >= T(1)/std::numeric_limits<T>::max()) {
                tt.test(1/tc);
                tt.test(-1/tc);
            }
        }
    } else {
        std::cout << "SKIP: "
                  << typeid(TestType).name() << ": "
                  << typeid(T).name() << '\n';
    }
}
