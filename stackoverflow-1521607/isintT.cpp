// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-04-13 01:26:25 - Создан.
//
// Check double variable if it contains an integer, and not floating point
//
// https://stackoverflow.com/questions/1521607/check-double-variable-if-it-contains-an-integer-and-not-floating-point
// https://stackoverflow.com/q/1521607/8585880
// https://stackoverflow.com/a/79570329/8585880
//

#define __STDC_WANT_IEC_60559_EXT__ (1)
#include <cmath>
#include <cstdint>
#include <limits>

// #define PYTHON_INF_NOT_INT (1)

#ifndef PYTHON_INF_NOT_INT
    template<typename T>
    bool isint_ceil(T x) noexcept {
        return std::ceil(x) == x;
    }
    template<typename T>
    constexpr bool isint_denorm(T x) noexcept {
        using nlT = std::numeric_limits<T>;
        static_assert(nlT::denorm_min() != nlT::min());
        return ((x*nlT::denorm_min())/nlT::denorm_min()) == x;
    }
    template<typename T>
    bool isint_floor(T x) noexcept {
        return std::floor(x) == x;
    }
    template<typename T, typename I = int64_t>
    constexpr bool isint_intN(T x) noexcept {
        return static_cast<T>(static_cast<I>(x)) == x;
    }
    template<typename T, typename I = int64_t, typename U = uint64_t>
    constexpr bool isint_intN_inf(T x) noexcept {
        using nlT = std::numeric_limits<T>;
        using nlI = std::numeric_limits<I>;
        constexpr T last_non_int = T(U(1) << (nlT::digits - 1)) - T(0.5L);
        static_assert(nlT::digits - 1 <= nlI::digits && sizeof(U) == sizeof(I));
        if constexpr (last_non_int >= T(1) + T(nlI::max())) throw x;  // assert
        return (x < -last_non_int || last_non_int < x) || isint_intN<T,I>(x);
    }
    template<typename T>
    bool isint_modf(T x) noexcept {
        T intpart{};
        return std::modf(x, &intpart) == 0;
    }
    template<typename T>
    bool isint_nearbyint(T x) noexcept {
        return std::nearbyint(x) == x;
    }
    template<typename T>
    bool isint_rint(T x) noexcept {
        return std::rint(x) == x;
    }
    template<typename T>
    bool isint_round(T x) noexcept {
        return std::round(x) == x;
    }
    template<typename T>
    bool isint_trunc(T x) noexcept {
        return std::trunc(x) == x;
    }
#else
    template<typename T>
    bool isint_ceil(T x) noexcept {
        return std::ceil(x) - x == 0;
    }
    template<typename T>
    constexpr bool isint_denorm(T x) noexcept {
        using nlT = std::numeric_limits<T>;
        static_assert(nlT::denorm_min() != nlT::min());
        return ((x*nlT::denorm_min())/nlT::denorm_min()) - x == 0;
    }
    template<typename T>
    bool isint_floor(T x) noexcept {
        return std::floor(x) - x == 0;
    }
    template<typename T, typename I  = int64_t>
    constexpr bool isint_intN(T x) noexcept {
        return static_cast<T>(static_cast<I>(x)) - x == 0;
    }
    template<typename T, typename I = int64_t, typename U = uint64_t>
    constexpr bool isint_intN_inf(T x) noexcept {
        using nlT = std::numeric_limits<T>;
        using nlI = std::numeric_limits<I>;
        constexpr T last_non_int = T(U(1) << (nlT::digits - 1)) - T(0.5L);
        static_assert(nlT::digits - 1 <= nlI::digits && sizeof(U) == sizeof(I));
        if constexpr (last_non_int >= T(1) + T(nlI::max())) throw x;  // assert
        return INFINITY != x && -INFINITY != x  &&
               ((x < -last_non_int || last_non_int < x) || isint_intN<T,I>(x));
    }
    template<typename T>
    bool isint_modf(T x) noexcept {
        T intpart{};
        return std::isfinite(x) && std::modf(x, &intpart) == 0;
    }
    template<typename T>
    bool isint_nearbyint(T x) noexcept {
        return std::nearbyint(x) - x == 0;
    }
    template<typename T>
    bool isint_rint(T x) noexcept {
        return std::rint(x) - x == 0;
    }
    template<typename T>
    bool isint_round(T x) noexcept {
        return std::round(x) - x == 0;
    }
    template<typename T>
    bool isint_trunc(T x) noexcept {
        return std::trunc(x) - x == 0;
    }
#endif

// Use Catch2: https://github.com/catchorg/Catch2/
// Header-only (without installing etc.), see:
// https://github.com/catchorg/Catch2/tree/v2.x/single_include/catch2
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch.hpp"

#if __has_include(<version>)
    #include <version>
#endif
#if __has_include(<format>) && defined(__cpp_lib_format)
    #include <format>
    // gcc unimplemented std::format() for std::float128_t
    // clang bug for long double
    template<typename T>
    void INFO_D(T d) {
        double hi = double(d);
        double lo = double(d - T(hi));
        const T limb = T(1e19);
        uint64_t ihi = uint64_t(d/limb);
        uint64_t ilo = uint64_t(d - T(ihi)*limb);
        UNSCOPED_INFO(std::format(
                      "{:a},{:a} {:.17g},{:.17g} : "
                      "{:#x}*{:#x}+{:#x} {:d}*{:d}+{:d}",
                      hi, lo, hi, lo,
                      ihi, uint64_t(limb), ilo, ihi, uint64_t(limb), ilo));
    }
#else
    #define INFO_D(d)  INFO(d)
#endif
#include <cfenv>
#include <cfloat>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif
// Tests (test data) for isint_intN<..., int64_t>()
static_assert(isint_intN<double> == isint_intN<double, int64_t>);
#define DT(fn, intN, std, cexpr, notexc_, chkexc_ ) \
                template<typename T> \
                struct fn##_t { \
                    typedef T t; \
                    static const bool is_intN = intN; \
                    static const bool is_std = std; \
                    static const bool is_constexpr = sizeof(#cexpr) > 1; \
                    static const int notexc = notexc_; \
                    static const bool chkexc = chkexc_; \
                    static cexpr bool f(T x) noexcept { return fn(x); } \
                };
// nearbyint(), modf() - FE_INEXACT is never raised;
// trunc(), floor(), ceil(), round() - FE_INEXACT is sometimes not raised,
// system/compiler depended;
// rint(), intN_t(), x*nlT::denorm_min() - raise FE_INEXACT
// for non-integer.
DT(isint_ceil,      false, true,  ,          0,          false);
DT(isint_denorm,    false, false, constexpr, 0,          true);
DT(isint_floor,     false, true,  ,          0,          false);
DT(isint_intN,      true,  false, constexpr, 0,          true);
DT(isint_intN_inf,  false, false, constexpr, 0,          true);
DT(isint_modf,      false, true,  ,          FE_INEXACT, true);
DT(isint_nearbyint, false, true,  ,          FE_INEXACT, true);
DT(isint_rint,      false, true,  ,          0,          true);
DT(isint_round,     false, true,  ,          0,          false);
DT(isint_trunc,     false, true,  ,          0,          false);
#define DT_LIST (isint_ceil_t, isint_denorm_t, isint_floor_t, isint_intN_t, \
                 isint_intN_inf_t, isint_modf_t, isint_nearbyint_t, \
                 isint_rint_t, isint_round_t, isint_trunc_t)
template<typename T>
struct test_cases {
    struct case_t {
        bool domain_intN;
        bool isint;
        T d;
    };
    static const case_t common[];
    static const case_t bytarg[];
    static constexpr size_t common_size() {
       return sizeof(common)/sizeof(common[0]);
    }
    static constexpr size_t bytarg_size() {
       return sizeof(bytarg)/sizeof(bytarg[0]);
    }
};
template<typename T>
const test_cases<T>::case_t test_cases<T>::common[] = {
    {true,  false, NAN},
    {true,  false, std::numeric_limits<T>::quiet_NaN()},
    {true,  true,  T(0)},
    {true,  false, std::numeric_limits<T>::denorm_min()},
    {true,  false, std::numeric_limits<T>::min()},
    {true,  false, T(0.1L)},
    {true,  true,  T(1)},
    {true,  false, T(1.1L)},
    {false, true,  std::numeric_limits<T>::max()},
#ifndef PYTHON_INF_NOT_INT
    {false, true,  std::numeric_limits<T>::infinity()},
    {false, true,  INFINITY},
#else
    {false, false, std::numeric_limits<T>::infinity()},
    {false, false, INFINITY},
#endif
};
template<>
const test_cases<float>::case_t test_cases<float>::bytarg[] = {
    {true,  false,           0x7ffffe.8p0f},
    {true,  true,            0x7fffff.0p0f},
    {true,  false,           0x7fffff.8p0f},
    {true,  true,            0x800000.0p0f},
    {true,  true,            0x800001.0p0f},
    {true,  true,            0x800002.0p0f},
    {true,  true,            0x800003.0p0f},
    {true,  true,  0x7fffff0000000000.0p0f},
    {true,  true,  0x7fffff8000000000.0p0f},
    {false, true,  0x8000000000000000.0p0f},
    {false, true,  0x8000010000000000.0p0f},
    {false, true,  0x8000020000000000.0p0f},
};
template<>
const test_cases<double>::case_t test_cases<double>::bytarg[] = {
    {true,  false,    0xffffffffffffe.8p0},
    {true,  true,     0xfffffffffffff.0p0},
    {true,  false,    0xfffffffffffff.8p0},
    {true,  true,    0x10000000000000.0p0},
    {true,  true,    0x10000000000001.0p0},
    {true,  true,    0x10000000000002.0p0},
    {true,  true,    0x10000000000003.0p0},
    {true,  true,  0x7ffffffffffff800.0p0},
    {true,  true,  0x7ffffffffffffc00.0p0},
    {false, true,  0x8000000000000000.0p0},
    {false, true,  0x8000000000000800.0p0},
    {false, true,  0x8000000000001000.0p0},
};
template struct test_cases<float>;
template struct test_cases<double>;
#if 64 != LDBL_MANT_DIG
    #define FL_LD80_BINARY64_EXT
#else
    static_assert(64 == std::numeric_limits<long double>::digits);
    #define FL_LD80_BINARY64_EXT  , long double
    template<>
    const test_cases<long double>::case_t test_cases<long double>::bytarg[] = {
        {true,  false, 0x7ffffffffffffffc.8p0L},
        {true,  true,  0x7ffffffffffffffd.0p0L},
        {true,  false, 0x7ffffffffffffffd.8p0L},
        {true,  true,  0x7ffffffffffffffe.0p0L},
        {true,  false, 0x7ffffffffffffffe.8p0L},
        {true,  true,  0x7fffffffffffffff.0p0L},
        {true,  false, 0x7fffffffffffffff.8p0L},
        {false, true,  0x8000000000000000.0p0L},
        {false, true,  0x8000000000000001.0p0L},
        {false, true,  0x8000000000000002.0p0L},
        {false, true,  0x8000000000000003.0p0L},
    };
    template struct test_cases<long double>;
#endif
#if !(defined(__STDCPP_FLOAT128_T__) || 113 == LDBL_MANT_DIG)
    #define FL_FLOAT128_T
    typedef void t_float128_t;
    const bool t_float128_t_skip_std = false;
#else
    #if 113 == LDBL_MANT_DIG
        typedef long double t_float128_t;
        const bool t_float128_t_skip_std = false;
    #else
        typedef std::float128_t t_float128_t;
        #if !defined(__clang__) && \
            (defined(__GNUC__) && __GNUC__*100 + __GNUC_MINOR__ <= 1501)
            const bool t_float128_t_skip_std = true;
        #else
            const bool t_float128_t_skip_std = false;
        #endif
    #endif
    static_assert(113 == std::numeric_limits<t_float128_t>::digits);
    #define FL_FLOAT128_T  , t_float128_t
    template<>
    constexpr bool isint_intN<t_float128_t>(t_float128_t x) noexcept {
        return isint_intN<t_float128_t, __int128>(x);
    }
    template<>
    constexpr bool isint_intN_inf<t_float128_t>(t_float128_t x) noexcept {
        return isint_intN_inf<t_float128_t, __int128, unsigned __int128>(x);
    }
    template<>
    const test_cases<t_float128_t>::case_t test_cases<t_float128_t>::bytarg[] = {
        {true,   false,     0xfffffffffffffffffffffffffffe.8p0f128},
        {true,   true,      0xffffffffffffffffffffffffffff.0p0f128},
        {true,   false,     0xffffffffffffffffffffffffffff.8p0f128},
        {true,   true,     0x10000000000000000000000000000.0p0f128},
        {true,   true,     0x10000000000000000000000000001.0p0f128},
        {true,   true,     0x10000000000000000000000000002.0p0f128},
        {true,   true,     0x10000000000000000000000000003.0p0f128},
        {true,   true,  0x7fffffffffffffffffffffffffff8000.0p0f128},
        {true,   true,  0x7fffffffffffffffffffffffffffc000.0p0f128},
        {false,  true,  0x80000000000000000000000000000000.0p0f128},
        {false,  true,  0x80000000000000000000000000008000.0p0f128},
        {false,  true,  0x80000000000000000000000000010000.0p0f128},
    };
#endif
#define F_LIST (float, double FL_LD80_BINARY64_EXT FL_FLOAT128_T)
// TODO:decoration: suffix in TEMPLATE_PRODUCT_TEST_CASE() code
TEMPLATE_PRODUCT_TEST_CASE("Basic tests of isint", "[isint]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
    if constexpr (!(std::is_same_v<T, t_float128_t> &&
                    t_float128_t_skip_std &&
                    TestType::is_std)) {
        constexpr auto F = TestType::f;
        if constexpr (TestType::is_constexpr) {
            constexpr auto ce_true = F(T(42.0L));
            constexpr auto ce_false = F(T(0.42L));
            CHECK(ce_true);
            CHECK(!ce_false);
        }
        for (auto t : test_cases<T>::common) {
            if constexpr (TestType::is_intN) {
                if (!t.domain_intN) {
                    continue;
                }
            }
            CAPTURE(t.domain_intN, t.isint);
            INFO_D(t.d);
            CHECK((!t.isint)^F(t.d));
            CHECK((!t.isint)^F(-t.d));
        }
        for (auto t : test_cases<T>::bytarg) {
            if constexpr (TestType::is_intN) {
                if (!t.domain_intN) {
                    continue;
                }
            }
            CAPTURE(t.domain_intN, t.isint);
            INFO_D(t.d);
            CHECK((!t.isint) ^ F(t.d));
            CHECK((!t.isint) ^ F(-t.d));
        }
    }
}
TEMPLATE_PRODUCT_TEST_CASE("Check FE_INEXACT", "[isint]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
    if constexpr (!(std::is_same_v<T, t_float128_t> &&
                    t_float128_t_skip_std &&
                    TestType::is_std)) {
        const auto& F = TestType::f;
        const auto NOTEXC = TestType::notexc;
        const auto CHKEXC = TestType::chkexc;
        volatile T x42_0 = T(42.0L);  // For volatile don't needed
        volatile T x0_42 = T(0.42L);  // FENV_ACCESS/fenv_access pragmas
        std::feclearexcept(FE_ALL_EXCEPT);
        CHECK(F(x42_0));
        CHECK(!F(x0_42));
        auto fe_inexact = std::fetestexcept(FE_INEXACT);
        int mask = (CHKEXC ? ~FE_ALL_EXCEPT : 0);
        CAPTURE(fe_inexact, mask);
        CHECK(0 == ((fe_inexact ^ (NOTEXC ^ FE_ALL_EXCEPT)) & mask));
    }
}
TEMPLATE_PRODUCT_TEST_CASE("Benchmark", "[isint][!benchmark]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
    if constexpr (!(std::is_same_v<T, t_float128_t> &&
                    t_float128_t_skip_std &&
                    TestType::is_std)) {
        const auto& F = TestType::f;
        volatile T x42_0 = T(42.0);  // Prevent remove code by optimization
        T t42_0 = x42_0;
        BENCHMARK("F(t42_0)") {
            return F(t42_0);
        };
        volatile T x0_42 = T(0.42L);  // Prevent remove code by optimization
        T t0_42 = x0_42;
        BENCHMARK("F(t0_42)") {
            return F(t0_42);
        };
    }
}
