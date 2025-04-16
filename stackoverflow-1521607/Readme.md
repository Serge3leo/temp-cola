# Сhecks if the given number has integral value

I came across an old question:
[Check double variable if it contains an integer, and not floating point](https://stackoverflow.com/q/1521607/8585880),
17 answers, but somehow everything is without a system. And the question
is interesting, as we say, with a trick. Especially in light of the latest
C++ innovations (`constexpr`, `float128_t`, etc.).

Probably, the simplest ways should be considered:
`std::int(x) == x`, `std::ceil(x) == x`, as the shortest ones. However,
they are not exactly identical, and also slightly different from the 17
others methods.

Main issues and features:
1. ±∞ are integers or not, both of these methods consider them integers
   (which is logical, because in the arithmetic of floating
   numbers ∀x ∈ R<sub>fp</sub> x ∉ Z => |x| < Const). But, for example,
   Python `float.is_integer()` does not consider them to be integers. To get
   the result identical Python should be used: `std::rint(x) - x == 0`;
2. `constexpr`, according to existing C++ standards, these
   expressions are impossible use when calculating the `constexpr`
   values (see other expressions in `isint_denorm()`, `isint_intN_inf()`
   and `isint_intN_inf()` below);
3. `FE_INEXACT`, by C/C++ standards, `std::rint()` always registers
   (or causes) a `FE_INEXACT` exception for non-integers, but for
   `std::ceil()` this will be determined by the implementation.
   It should be noted that working with the FPU state can lead to
   a noticeable drop in performance when running on virtual machines
   (depending of compiler, OS, and VM);
4. Performance, theoretically, since `std::rint()` uses
   the current rounding method, while `std::ceil()` rounds toward positive,
   `std::rint()` may be more efficient (but the efficiency may also be
   affected by the previous paragraph);
5. Type support, incomplete ones are found for `float128_t`
   implementations without `std::rint()`, `std::ceil()`, etc. (see
   `isint_denorm()`, etc).

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
template<typename T, typename I  = int64_t>
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
```

Summary table of these 10 methods with a arbitrary performance rating:

 Name           | Rating  | constexpr | FE_INEXACT | As Python           | Notes
----------------|---------|-----------|------------|---------------------|-----------------------------------
isint_intN      | 1       | constexpr | yes        | -                   | -2<sup>63</sup>..2<sup>63</sup>-1
isint_rint      | 2 or 5¹ |           | must       | f(x) - x == 0       |
isint_ceil      | 3       |           |            | f(x) - x == 0       |
isint_floor     | 3       |           |            | f(x) - x == 0       |
isint_intN_inf  | 3       | constexpr | yes        | INFINITY != x &&... | Max binary80
isint_nearbyint | 3 or 6² |           | no         | f(x) - x == 0       |
isint_trunc     | 3       |           |            | f(x) - x == 0       |
isint_round     | 4       |           |            | f(x) - x == 0       |
isint_modf      | 5       |           | no         | isfinite(x) &&...   |
isint_denorm    | 7       | constexpr | yes        | f(x) - x == 0       | Any ISO/IEC 60559

¹) `std::rint()`, unexpectedly, turned out to be very inefficient for VS2022
on Win10 under Parallels on mac OS. The reasons for this are unclear;

²) `std::nearbyint()` often uses fast code, but with
save/restore FPU state, so when working on virtual machines, it can
be worse even than `std::modf()`.

If `constexpr` is not required, then the options are simplest
`std::ceil()`/`std::floor()`/`std::trunk()` is a good choice with stable
performance for any input arguments. IMHO, CPython implementation
`float.is_integer()` uses `floor()`.

GCC 14 (or pre-release version of GCC 15) has incomplete support
for `float128_t` (for budget processors, not POWER/SPARC).
Therefore, in strict compliance with the standard, only one
option is possible: `isint_denorm()'. In addition, non-standard integer
types can be used: `isint_intN<float128_t, __int128>()`
и `isint_intN_inf<float128_t, __int128, unsigned __int128>()`.

The source code of Python-compatible variants `float.is_integer()`, and the tests:
[https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp](https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp)

P.S.

If someone knows other `constexpr` ways to check floating numbers
has integral value, I would be very grateful.
