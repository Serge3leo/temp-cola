// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-04-16 00:50:39 - Создан.
// 2025-04-20 12:47:01 - https://godbolt.org/z/chY3GrhKd
//

#include <charconv>
#include <concepts>
#include <iostream>
#include <type_traits>

template <typename T>
concept OStreamable = requires(T x, std::ostream& o) {
    {o << x} -> std::same_as<std::ostream&>;
};
template<typename T>
concept TO_Charsable =
    #if defined(__clang__) && defined(_LIBCPP_VERSION)
        // https://github.com/llvm/llvm-project/issues/62282
        // См. примечание к P0067R5 в https://libcxx.llvm.org/Status/Cxx17.html
        (!std::is_same_v<T, long double> ||
         std::is_same_v<double, long double>) &&
    #endif
requires(T x, char* first, char* last) {
    {std::to_chars(first, last, x)} -> std::same_as<std::to_chars_result>;
};
#if __has_include(<format>) && (__cplusplus >= 202106L || __cpp_lib_format)
    #include <format>
    template <typename T>
    concept Formattable =
        #if defined(__clang__) && defined(_LIBCPP_VERSION)
            (!std::is_same_v<T, long double> ||
             std::is_same_v<double, long double>) &&
        #endif
        std::formattable<T, char>;
        // Паллиатив: std::is_default_constructible_v<std::formatter<T>>;
        #define TEST_CPP_FORMATTABLE (1)
#endif

// Тесты (примеры использования)

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#if __has_include(<stdfloat>)
    #include <stdfloat>
#endif
#include <sstream>
#include <typeinfo>

#if __STDCPP_FLOAT128_T__
    using testfloat_t = std::float128_t;
    testfloat_t test_e = 2.7182818284'5904523536'0287471352'66249'8f128;
#else
    using testfloat_t = long double;
    testfloat_t test_e = 2.7182818284'5904523536'0287471352'66249'8L;
#endif
const int test_precision = std::numeric_limits<testfloat_t>::max_digits10;

struct test_iostream {
    template <OStreamable T>
    std::string operator()(T x, const std::string& low) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(test_precision) << x;
        return o.str();
    }
    static_assert(OStreamable<bool>);
    static_assert(OStreamable<int>);
    static_assert(OStreamable<double>);
    static_assert(OStreamable<std::string>);
};
struct test_to_chars {
    template <TO_Charsable T>
    std::string  operator()(T x, const std::string& low) {
        const size_t ipart = size_t(std::ceil(std::log10(T(1) + std::abs(x))));
        std::string str(2 + ipart + test_precision, '\0');
        auto r = std::to_chars(str.data(), str.data() + str.size(),
                               x, std::chars_format::fixed, test_precision);
        str.resize(r.ptr - str.data());
        return str;
    }
    static_assert(!TO_Charsable<bool>);
    static_assert(TO_Charsable<int>);
    static_assert(TO_Charsable<double>);
    static_assert(!TO_Charsable<std::string>);
};
#ifdef TEST_CPP_FORMATTABLE
    struct test_format {
        template <Formattable T>
        std::string  operator()(T x, const std::string& low) {
            return std::format("{:.{}f}", x, test_precision);
        }
        static_assert(Formattable<bool>);
        static_assert(Formattable<int>);
        static_assert(Formattable<double>);
        static_assert(Formattable<std::string>);
    };
#endif
template <typename F, typename T>
std::string test_cvt_one(testfloat_t x, const std::string &low) {
    if constexpr (std::is_invocable_v<F, T, decltype(low)>) {
        F f;
        std::string str = f(T(x), low);
        std::cout << str
                  << (low == str ? ": FAIL (== low precision): " : ": ")
                  << __func__ << ": " << typeid(F).name() << ": "
                  << typeid(T).name() << '\n';
        return str;
    }
    std::cout << "Unimplemented: " << __func__
              << ": " << typeid(F).name() << ": " << typeid(T).name() << '\n';
    return low;
}
struct unimplemented {};
template <typename F>
void test_cvt_all(testfloat_t x) {
    std::string low{""};
    low = test_cvt_one<F, float>(x, low);
    low = test_cvt_one<F, double>(x, low);
    if constexpr (std::numeric_limits<long double>::digits !=
                  std::numeric_limits<double>::digits) {
        low = test_cvt_one<F, long double>(x, low);
    }
    #if __STDCPP_FLOAT128_T__
        if constexpr (std::numeric_limits<std::float128_t>::digits !=
                      std::numeric_limits<long double>::digits) {
            low = test_cvt_one<F, std::float128_t>(x, low);
        }
    #endif
    low = test_cvt_one<F, unimplemented>(x, low);
    (void)low;
}
int main() {
    std::cout << "long double: "
              << std::numeric_limits<long double>::digits
              << "bits (max_digits10: "
              << std::numeric_limits<long double>::max_digits10
              << ")\n";
    #if __STDCPP_FLOAT128_T__
        std::cout << "std::float128_t: "
                  << std::numeric_limits<std::float128_t>::digits
                  << "bits (max_digits10: "
                  << std::numeric_limits<std::float128_t>::max_digits10
                  << ")\n";
    #endif
    test_cvt_all<test_iostream>(test_e);
    test_cvt_all<test_to_chars>(test_e);
    #ifdef TEST_CPP_FORMATTABLE
        test_cvt_all<test_format>(test_e);
    #endif
}
