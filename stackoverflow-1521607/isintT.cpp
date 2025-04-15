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
    #define INFO_D(d)  INFO(std::format("{:a} {:3f} : {:#x} {:d}", d, d, \
                            int64_t(d), int64_t(d)));
#else
    #define INFO_D(d)  INFO(d)
#endif
#include <cfenv>
#include <cfloat>

// Tests (test data) for isint_intN<..., int64_t>()
static_assert(isint_intN<double> == isint_intN<double, int64_t>);
#define DT(fn, int64, cexpr, notexc_, chkexc_ ) \
                template<typename T> \
                struct fn##_t { \
                    typedef T t; \
                    static const bool is_int64 = int64; \
                    static const bool is_constexpr = sizeof(#cexpr) > 1; \
                    static const int notexc = notexc_; \
                    static const bool chkexc = chkexc_; \
                    static cexpr bool f(T x) noexcept { return fn(x); } \
                };
// nearbyint(), modf() - FE_INEXACT is never raised;
// trunc(), floor(), ceil(), round() - FE_INEXACT is sometimes not raised,
// system/compiler depended;
// rint(), int64_t(), x*nlT::denorm_min() - raise FE_INEXACT
// for non-integer.
DT(isint_ceil,      false, ,          0,          false);
DT(isint_denorm,    false, constexpr, 0,          true);
DT(isint_floor,     false, ,          0,          false);
DT(isint_intN,      true,  constexpr, 0,          true);
DT(isint_intN_inf,  false, constexpr, 0,          true);
DT(isint_modf,      false, ,          FE_INEXACT, true);
DT(isint_nearbyint, false, ,          FE_INEXACT, true);
DT(isint_rint,      false, ,          0,          true);
DT(isint_round,     false, ,          0,          false);
DT(isint_trunc,     false, ,          0,          false);
#define DT_LIST (isint_ceil_t, isint_denorm_t, isint_floor_t, isint_intN_t, \
                 isint_intN_inf_t, isint_modf_t, isint_nearbyint_t, \
                 isint_rint_t, isint_round_t, isint_trunc_t)

template<typename T>
struct test_cases {
    struct case_t {
        bool int64;
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
    {true,   false,               8388606.5f },
    {true,   true,                8388607.0f },
    {true,   false,               8388607.5f },
    {true,   true,                8388608.0f },
    {true,   true,                8388609.0f },
    {true,   true,                8388610.0f },
    {true,   true,                8388611.0f },
    {true,   true,    9223370937343148032.0f },
    {true,   true,    9223371487098961920.0f },
    {false,  true,    9223372036854775808.0f },
    {false,  true,    9223373136366403584.0f },
    {false,  true,    9223374235878031360.0f },
};
template<>
const test_cases<double>::case_t test_cases<double>::bytarg[] = {
    {true,   false,      4503599627370494.5  },
    {true,   true,       4503599627370495.0  },
    {true,   false,      4503599627370495.5  },
    {true,   true,       4503599627370496.0  },
    {true,   true,       4503599627370497.0  },
    {true,   true,       4503599627370498.0  },
    {true,   true,       4503599627370499.0  },
    {true,   true,    9223372036854773760.0  },
    {true,   true,    9223372036854774784.0  },
    {false,  true,    9223372036854775808.0  },
    {false,  true,    9223372036854777856.0  },
    {false,  true,    9223372036854779904.0  },
};
template struct test_cases<float>;
template struct test_cases<double>;
#if LDBL_MANT_DIG == DBL_MANT_DIG
    #define F_LIST  (float, double)
#else
    #define F_LIST  (float, double, long double)
    static_assert(64 == std::numeric_limits<long double>::digits);
    template<>
    const test_cases<long double>::case_t test_cases<long double>::bytarg[] = {
        {true,   false,  9223372036854775804.5L  },
        {true,   true,   9223372036854775805.L   },
        {true,   false,  9223372036854775805.5L  },
        {true,   true,   9223372036854775806.L   },
        {true,   false,  9223372036854775806.5L  },
        {true,   true,   9223372036854775807.L   },
        {true,   false,  9223372036854775807.5L  },
        {false,  true,   9223372036854775808.L   },
        {false,  true,   9223372036854775809.L   },
        {false,  true,   9223372036854775810.L   },
        {false,  true,   9223372036854775811.L   },
    };
#endif
TEMPLATE_PRODUCT_TEST_CASE("Basic tests of isint", "[isint]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
    constexpr auto F = TestType::f;
    if constexpr (TestType::is_constexpr) {
        constexpr auto ce_true = F(T(42.0L));
        constexpr auto ce_false = F(T(0.42L));
        CHECK(ce_true);
        CHECK(!ce_false);
    }
    for (auto t : test_cases<T>::common) {
        if constexpr (TestType::is_int64) {
            if (!t.int64) {
                continue;
            }
        }
        CAPTURE(t.int64, t.isint);
        INFO_D(t.d);
        CHECK((!t.isint)^F(t.d));
        CHECK((!t.isint)^F(-t.d));
    }
    for (auto t : test_cases<T>::bytarg) {
        if constexpr (TestType::is_int64) {
            if (!t.int64) {
                continue;
            }
        }
        CAPTURE(t.int64, t.isint);
        INFO_D(t.d);
        CHECK((!t.isint) ^ F(t.d));
        CHECK((!t.isint) ^ F(-t.d));
    }
}
TEMPLATE_PRODUCT_TEST_CASE("Check FE_INEXACT", "[isint]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
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
TEMPLATE_PRODUCT_TEST_CASE("Benchmark", "[isint][!benchmark]",
                           DT_LIST, F_LIST) {
    typedef typename TestType::t T;
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
