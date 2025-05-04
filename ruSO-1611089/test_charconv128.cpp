// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-05-01 08:40:09 - Создан.
//

#define __STDC_WANT_IEC_60559_EXT__ (1)
#define __STDC_WANT_IEC_60559_TYPES_EXT__ (1)
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
#define CATCH_CONFIG_ENABLE_BENCHMARKING
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
    static constexpr const char *ofmt = (std::is_same_v<float, fmt_t> ? "%.*f" :
                       std::is_same_v<double, fmt_t> ? "%.*lf" :
                       std::is_same_v<long double, fmt_t> ? "%.*Lf" : nullptr);
    static constexpr const char *ifmt = (std::is_same_v<float, fmt_t> ? "%f" :
                       std::is_same_v<double, fmt_t> ? "%lf" :
                       std::is_same_v<long double, fmt_t> ? "%Lf" : nullptr);
    static string test_snprintf(T x) {
        const auto check_ = check;
        char buf[width + 1];
        int len = snprintf(buf, sizeof(buf), ofmt, precision, x);
        REQUIRE(check_ <= len);
        REQUIRE(string(check_, '0') == string(buf + len - check_, buf + len));
        return string(buf);
    }
    static T test_sscanf(const string& buf) {
        fmt_t x;
        REQUIRE(1 == sscanf(buf.c_str(), ifmt, &x));
        return x;
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
void test_x_sscanf(const T x) {
    #ifdef __FreeBSD__
        // man sscanf: BUGS: Numerical strings are truncated to 512 characters
        constexpr bool fbsd_bug = fix<T>::ifmt &&
                        (std::string("%Lf") == std::string(fix<T>::ifmt));
    #else
        const bool fbsd_bug = false;
    #endif
    if constexpr (fix<T>::ofmt && !fbsd_bug) {
        std::string p = fix<T>::test_snprintf(x);
        T sx = fix<T>::test_sscanf(p);
        CHECK(x == sx);
    } else {
        std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
    }
}
template <class T>
void test_x_to_chars(const T x) {
    if constexpr (fix<T>::ofmt) {
        std::string p = fix<T>::test_snprintf(x);
        std::string t = fix<T>::test_to_chars(x);
        CHECK(p == t);
    } else {
        std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
    }
}
template <class T>
void test_x_from_chars(const T x) {
    if constexpr (fix<T>::ofmt) {
        std::string p = fix<T>::test_snprintf(x);
        T fx = fix<T>::test_from_chars(p);
        CHECK(x == fx);
    } else {
        std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
    }
}
template <class T>
void test_x_to_from_chars(const T x) {
    std::string t = fix<T>::test_to_chars(x);
    T fx = fix<T>::test_from_chars(t);
    CHECK(x == fx);
}
template <class T>
void test_sscanf() {
    T tcs[] = { std::numeric_limits<T>::max(),
                std::numeric_limits<T>::min(),
                std::numeric_limits<T>::denorm_min()};
    for (const auto& tc : tcs) {
        test_x_sscanf(tc);
        test_x_sscanf(-tc);
        if (tc >= T(1)/std::numeric_limits<T>::max()) {
            test_x_sscanf(1/tc);
            test_x_sscanf(-1/tc);
        }
    }
}
template <class T>
void test_to_chars() {
    std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
}
template <class T> requires has_to_chars<T>
void test_to_chars() {
    T tcs[] = { std::numeric_limits<T>::max(),
                std::numeric_limits<T>::min(),
                std::numeric_limits<T>::denorm_min()};
    for (const auto& tc : tcs) {
        test_x_to_chars(tc);
        test_x_to_chars(-tc);
        if (tc >= T(1)/std::numeric_limits<T>::max()) {
            test_x_to_chars(1/tc);
            test_x_to_chars(-1/tc);
        }
    }
}
template <class T>
void test_from_chars() {
    std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
}
template <class T> requires has_from_chars<T>
void test_from_chars() {
    T tcs[] = { std::numeric_limits<T>::max(),
                std::numeric_limits<T>::min(),
                std::numeric_limits<T>::denorm_min()};
    for (const auto& tc : tcs) {
        test_x_from_chars(tc);
        test_x_from_chars(-tc);
        if (tc >= T(1)/std::numeric_limits<T>::max()) {
            test_x_from_chars(1/tc);
            test_x_from_chars(-1/tc);
        }
    }
}
template <class T>
void test_to_from_chars() {
    std::cout << "SKIP: " << __func__ << ": " << typeid(T).name() << '\n';
}
template <class T> requires has_from_chars<T> && has_to_chars<T>
void test_to_from_chars() {
    T tcs[] = { std::numeric_limits<T>::max(),
                std::numeric_limits<T>::min(),
                std::numeric_limits<T>::denorm_min()};
    for (const auto& tc : tcs) {
        test_x_to_from_chars(tc);
        test_x_to_from_chars(-tc);
        if (tc >= T(1)/std::numeric_limits<T>::max()) {
            test_x_to_from_chars(1/tc);
            test_x_to_from_chars(-1/tc);
        }
    }
}
#ifdef __STDCPP_FLOAT128_T__
    #define F_LIST float, double, long double, std::float128_t
#else
    #define F_LIST float, double, long double
#endif
TEMPLATE_TEST_CASE("to_from_chars", "[to_chars][from_chars]", F_LIST) {
    test_sscanf<TestType>();
    test_to_chars<TestType>();
    test_from_chars<TestType>();
    test_to_from_chars<TestType>();
}
