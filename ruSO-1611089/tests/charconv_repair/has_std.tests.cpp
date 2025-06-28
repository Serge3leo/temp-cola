// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-06-28 03:06:46 - Создан.
//

#define __STDC_WANT_IEC_60559_BFP_EXT__  (1)  // for strfrom*() family

#include "float_format/charconv_repair.hpp"

#include <cfloat>
#include <charconv>
#include <iostream>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif

#ifndef CATCH2_SINGLE_INCLUDE
    #include <catch2/catch_all.hpp>
#else
    #include "catch2/catch.hpp"
#endif

TEST_CASE("has_std_from_to_chars") {
    using charconv_repair::has_std_from_chars;
    using charconv_repair::has_std_to_chars;
    std::cout << "C++: " << __cplusplus << ", "
        #if defined(__GNUC__)
            #if defined(__clang__)
                << "clang++: "
            #else
                << "GNU g++: "
            #endif
            << __VERSION__ << ", "
        #elif defined(_MSC_VER)
            "MSVC: " << _MSC_FULL_VER << ", "
        #else
            "Unknown compiler"
        #endif
        #if defined(_LIBCPP_VERSION)
            "libc++: " << _LIBCPP_VERSION << ", "
        #endif
        #ifdef __GLIBCXX__
            "libstdc++: " << __GLIBCXX__ << ", "
        #endif
        #ifdef __GLIBC__
            "glibc: " << __GLIBC__ << ", "
        #endif
        << "Catch2: " << Catch::libraryVersion()
        << '\n';
    #if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 199999
        CHECK(!has_std_from_chars<float>);
        CHECK(!has_std_from_chars<double>);
    #else
        CHECK(has_std_from_chars<float>);
        CHECK(has_std_from_chars<double>);
    #endif
    CHECK(has_std_to_chars<float>);
    CHECK(has_std_to_chars<double>);
    #if defined(_LIBCPP_VERSION) && \
        DBL_MANT_DIG < LDBL_MANT_DIG && LDBL_MANT_DIG < 113
        CHECK(!has_std_from_chars<long double>);
        CHECK(!has_std_to_chars<long double>);
    #else
        CHECK(has_std_from_chars<long double>);
        CHECK(has_std_to_chars<long double>);
    #endif
    #if defined(__STDCPP_FLOAT128_T__)
        #if defined(__linux__)  // TODO glibc define to_chars/from_chars for float128_t
            CHECK(has_std_from_chars<std::float128_t>);
            CHECK(has_std_to_chars<std::float128_t>);
        #else
            CHECK(!has_std_from_chars<std::float128_t>);
            CHECK(!has_std_to_chars<std::float128_t>);
        #endif
    #endif
}
TEST_CASE("has_from_to_chars") {
    using charconv_repair::has_from_chars;
    using charconv_repair::has_to_chars;
    CHECK(has_from_chars<float>);
    CHECK(has_to_chars<float>);
    CHECK(has_from_chars<double>);
    CHECK(has_to_chars<double>);
    CHECK(has_from_chars<long double>);
    CHECK(has_to_chars<long double>);
    #if defined(__STDCPP_FLOAT128_T__)
        CHECK(has_from_chars<std::float128_t>);
        CHECK(has_to_chars<std::float128_t>);
    #endif
}
