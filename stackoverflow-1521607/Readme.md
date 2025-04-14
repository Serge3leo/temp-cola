# Сhecks if the given number has integral value

I came across an old question:
[Check double variable if it contains an integer, and not floating point](https://stackoverflow.com/q/1521607/8585880),
17 answers, but somehow everything is without a system. And the question 
is interesting, as we say, with a trick.

Probably, the simplest ways should be considered: 
`std::int(x) == x`, `std::ceil(x) == x`, as the shortest ones. However, 
they are not exactly identical, and also slightly different from the 17 
others methods.

Main issues and features:
1. ±∞ is an integer or not, both of these methods consider it to be
   an integer. But, for example, Python `float.is_integer()` considers
   it a non-integer number. To get the result identical Python should be
   used: `std::rint(x) - x == 0.`;
2. `constexpr`, according to existing C/C++ standards, these
   expressions are impossible use when calculating the `constexpr'
   values (see other methods below);
3. `FE_INEXACT`, by C/C++ standards, `std::rint()` always registers
   (or causes) a `FE_INEXACT` exception for non-integers, but for 
   `std::ceil()` this will be determined by the implementation.
   It should be noted that maintenance of the FPU status can lead
   to a noticeable drop in performance when running on virtual
   machines (depending on compiler, OS, and virtual machine);
4. Performance, theoretically, since `std::rint()` uses
   the current rounding method, while `std::ceil()` rounds down,
   `std::rint()` may be more efficient (but the efficiency may also be
   affected by the previous paragraph).

10 ways to check integral value of a floating number with a single 
expression:
```
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
template<typename T>
constexpr bool isint_int64(T x) noexcept {
    return static_cast<T>(static_cast<int64_t>(x)) == x;
}
template<typename T>
constexpr bool isint_int64_inf(T x) noexcept {
    using nlT = std::numeric_limits<T>;
    static_assert(nlT::digits - 1 <= std::numeric_limits<int64_t>::digits);
    const T last_non_int = static_cast<T>(1ULL << (nlT::digits - 1))
                           - static_cast<T>(0.5);
    return (x < -last_non_int || last_non_int < x) || isint_int64(x);
}
template<typename T>
bool isint_modf(T x) noexcept {
    T intpart{};
    return std::modf(x, &intpart) == 0.0;
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
```

Summary table of these 10 methods with a conditional performance rating:

 Name           | Rating   | constexpr | FE_INEXACT | For Python          | Notes
----------------|----------|-----------|------------|---------------------|-----------------------------------
isint_int64     | 1        | constexpr | yes        | -                   | -2<sup>63</sup>..2<sup>63</sup>-1
isint_rint      | 2 or 10¹ |           | must       | f(x) - x == 0       |
isint_ceil      | 3        |           |            | f(x) - x == 0       |
isint_floor     | 3        |           |            | f(x) - x == 0       |
isint_nearbyint | 3 or 9²  |           | no         | f(x) - x == 0       |
isint_trunc     | 3        |           |            | f(x) - x == 0       |
isint_round     | 4        |           |            | f(x) - x == 0       |
isint_int64_inf | 5        | constexpr | yes        | INFINITY != x &&... | Max binary80
isint_modf      | 6        |           | no         | isfinite(x) &&...   |
isint_denorm    | 10       | constexpr | yes        | f(x) - x == 0       | Any ISO/IEC 60559

¹) `std::rint()`, unexpectedly, turned out to be very inefficient for VS2022
on Win10 under Parallels on mac OS. The reasons for this are unclear;

²) `std::nearbyint()` often uses fast code, but with
save/restore FPU state, so when working on virtual machines, it can
be worse even than `std::modf()`.

If `constexpr` is not required, then the options are 
`std::floor()`/`std::floor()`/`std::trunk()` is a good choice with stable
performance for any input arguments. IMHO, implementation 
`float.is_integer()` CPython uses `floor()`.

The source code of Python-compatible variants `float.is_integer()`, and the tests:
[https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp](https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp)