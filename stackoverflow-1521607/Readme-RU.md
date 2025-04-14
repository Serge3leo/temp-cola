# Проверка целостности числа с плавающей точкой

Встретился давний вопрос:
[Check double variable if it contains an integer, and not floating point](https://stackoverflow.com/q/1521607/8585880),
17 ответов, но как-то все без системы. А вопрос-то интересный, как у нас 
говорят, с подвохом.

Наверно, самыми простыми способами следует считать: 
`std::rint(x) == x`, `std::ceil(x) == x`, как самые короткие. Однако, они 
не совсем идентичны, а так же немного отличаются от 17 других 
способов.

Основные вопросы и особенности:
1. ±∞ - целое или нет, оба этих способа считают её целой. Но, к примеру,
   метод Python `float.is_integer()` - нет. Что бы получить результат
   идентичный Python следует использовать: `std::rint(x) - x == 0.`;
2. `constexpr`, по существующим стандартам C/C++ эти выражения невозможно
   использовать при вычислении `constexpr` переменных (другие способы
   смотрите ниже);
3. `FE_INEXACT`, по стандартам C/C++ `std::rint()` всегда регистрирует
   (или вызывает) исключение `FE_INEXACT` для нецелых чисел, а для 
   `std::ceil()` это определятся реализацией. Следует отметить, что
   обслуживание состояния FPU может приводить к заметному падению
   производительности при работе на виртуальных машинах (в зависимости от
   компилятора, ОС и виртуальной машины);
4. Производительность, теоретически, поскольку `std::rint()` использует
   текущий метод округления, а `std::ceil()` округляет вниз, `std::rint()`
   может быть более эффективным (но на эффективность так же может влиять
   предыдущий пункт). 

10 способов проверки целостности плавающего числа одним выражением:
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

Сводная таблица по этим 10 методам с условным рейтингом производительности:

 Название       | Рейтинг   | constexpr | FE_INEXACT | Для Python          | Примечания
----------------|-----------|-----------|------------|---------------------|-----------------------------------
isint_int64     | 1         | constexpr | да         | -                   | -2<sup>63</sup>..2<sup>63</sup>-1
isint_rint      | 2 или 10¹ |           | обязан     | f(x) - x == 0       |
isint_ceil      | 3         |           |            | f(x) - x == 0       |
isint_floor     | 3         |           |            | f(x) - x == 0       |
isint_nearbyint | 3 или 9²  |           | нет        | f(x) - x == 0       |
isint_trunc     | 3         |           |            | f(x) - x == 0       |
isint_round     | 4         |           |            | f(x) - x == 0       |
isint_int64_inf | 5         | constexpr | да         | INFINITY != x &&... | Максимум binary80
isint_modf      | 6         |           | нет        | isfinite(x) &&...   |
isint_denorm    | 10        | constexpr | да         | f(x) - x == 0       | Любые ISO/IEC 60559

¹) `std::rint()`, неожиданно, оказался весьма неэффективен для VS2022 
на Win10 под Parallels на macOS. Причины этого неясны;

²) `std::nearbyint()`, часто, использует быстрый код, но с сохранением 
состояния FPU, поэтому при работе на виртуальных машинах он может 
проигрывать даже `std::modf()`.

Если не требуется `constexpr`, то варианты 
`std::floor()`/`std::floor()`/`std::trunc()` - хороший выбор со стабильной 
производительностью для любых входных аргументов. ЕМНИП, реализация 
`float.is_integer()` CPython использует `floor()`.

Исходный код вариантов, совместимых с Python `float.is_integer()`, и тесты:
[https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp](https://github.com/Serge3leo/temp-cola/blob/main/stackoverflow-1521607/isintT.cpp)


