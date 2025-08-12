// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-08-05 18:32:18 - Создан.
//

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <ostream>
#include <print>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

#include "qdpy2_cstdint.hpp"

typedef uint64_t limb_t;
typedef qdpy2::uint128_t limb2_t;
static constexpr
limb_t ipow(limb_t base, limb_t exp) {
    limb_t res = 1;
    for (limb_t i = 0; i < exp; i++) {
        res *= base;
    }
    return res;
}
static
std::ostream& operator<<(std::ostream& os, const limb2_t& value) {
    // TODO: locale/numpunct/do_grouping
    const int mod_width = 15;
    const uint64_t mod = ipow(10, mod_width);
    const auto save_width = os.width();
    const auto save_fill = os.fill();
    limb2_t v1 = value/mod;
    limb2_t v2 = v1/mod;
    if (v2) {
        os << uint64_t(v2);
        os.width(mod_width);
        os.fill('0');
    }
    if (v2 || v1) {
        os << uint64_t(v1%mod);
        os.width(mod_width);
        os.fill('0');
    }
    os << uint64_t(value%mod);
    os.width(save_width);
    os.fill(save_fill);
    return os;
}
double _count_log10_e =
    0.434294481903251827651128918916605082294397005803666566114453783165864649;
static
double l10gamma(double x) {
    return lgamma(x)*_count_log10_e;
}
static
double l10factorial(limb_t n) {
    return l10gamma(n + 1);
}
static
double m10gamma(limb_t n, double next_y = -1.) {
    auto l10g = l10factorial(n);
    if (-1. != next_y) {
        l10g = nextafter(l10g, next_y);
    }
    double i;
    return pow(10., modf(l10g, &i));
}
struct factorial_t {
    limb2_t low = 1;
    limb2_t high = 1;
    limb_t exp = 0;
    limb_t exp_len = 1;
    limb_t exp_limit = 10;
    limb_t cond_prec;
    limb_t number = 1;
    uint8_t digits;
    limb_t mod;
    factorial_t(limb_t digits_ = std::numeric_limits<limb_t>::digits10,
                limb_t cond_prec_ = std::numeric_limits<double>::digits10  // 15
               ):
        cond_prec{cond_prec_}, digits{uint8_t(digits_)}, mod{ipow(10, digits_)}
    {
        assert(digits_ <= std::numeric_limits<limb_t>::digits10);
        prepare_conditional();
    }
    factorial_t(limb_t number_, limb_t low_, limb_t high_, limb_t exp_,
                limb_t digits_, limb_t cond_prec_):
        low{low_}, high{high_}, exp{exp_}, cond_prec{cond_prec_},
        number{number_}, digits{uint8_t(digits_)}, mod{ipow(10, digits_)}
    {
        for (auto e = exp; e >= 10; e /= 10) {
            exp_len++;
            exp_limit *= 10;
        }
        prepare_conditional();
    }
    bool next(limb_t n) {
        assert(n < mod);
        prepare_conditional();
        auto i = number + 1;
        while (i <= n) {
            assert(low <= std::numeric_limits<limb2_t>::max()/mod);
            low *= i;
            limb_t c = 0;
            while (low >= mod) {
                low /= 10;
                c++;
            }
            auto f = ipow(10, c);
            assert(f < mod);
            exp += c;
            if (exp >= exp_limit) {
                exp_len += 1;
                exp_limit *= 10;
                prepare_conditional();
            }
            assert(high <= std::numeric_limits<limb2_t>::max()/mod);
            high = (high*i + f - 1)/f;
            // TODO: Насколько high может превысить mod?
            // Похоже вопрос чисто теоретический, для 64 бит (19 цифр) не
            // достичь переполнения `limb_t`.
            i++;
            if (stop_conditional()) {
                break;
            }
            if (high >= mod) {
                throw std::out_of_range("low > high");
            }
        }
        number = i - 1;
        return number == n;
    }
    double low_m10() const {
        double r = low;
        while (r >= 10.) {
            r /= 10.;
        }
        return r;
    }
    double high_m10() const {
        double r = high;
        while (r >= 10.) {
            r /= 10.;
        }
        return (high < mod ? r : r*10.);
    }
    limb2_t low_cond = 0;
    limb2_t high_cond = -1;
    void prepare_conditional() {
        // Останавливаемся при обнаружении близости log10 n! к целому, т.е.
        // modf(log10(n!)) <= eps*log10(n!) или >= 1 - eps*log10(n!)
        //
        // TODO: игнорируем условия для 0 == exp (не нормализованных) а так-же
        // в случаях явного недостатка(избытка) точности.  В пограничных
        // случаях, для сравнительно больших exp_len (относительно cond_prec),
        // непонятно, возможно может начать срабатывать на каждом числе.
        if (0 == exp) {
            low_cond = 0;
            high_cond = -1;
            return;
        }
        // Что-то здесь не так, но что? TODO
        //
        // log10(x) - floor(log10(x)) <= eps*log10(x)
        // x / 10**floor(log10(x)) <= x**eps
        //
        // d/dy 10^y = log(10)*10^y
        // 10^(log10(n!) + delta) = n!*(1 + delta*log(10))
        // delta = exp*10^-cond_prec
        //
        auto delta = pow(10., double(exp_len) - 1. - double(cond_prec));
        auto eps = delta/_count_log10_e;
        low_cond = mod*(1. + eps)/10.;
        high_cond = mod/(1. + eps);
    }
    bool stop_conditional() const {
        return low < low_cond || high_cond < high;
    }
    void set_cond_prec(limb_t cond_prec_) {
        cond_prec = cond_prec_;
        prepare_conditional();
    }
};
std::ostream& operator<<(std::ostream& os, const factorial_t& value) {
    return os << "factorial_t {low=" << value.low
              << ", high=" << value.high
              << ", exp=" << value.exp
              << ", exp_len=" << value.exp_len
              << ", number=" << value.number
              << ", digits=" << int(value.digits)
              << ", cond_prec=" << value.cond_prec
              << ", low_cond=" << value.low_cond
              << ", high_cond=" << value.high_cond
              << '}';
}
namespace fs = std::filesystem;
class out_factorial_t {
    std::string comment_;
    limb_t digits_;
    limb_t cond_prec_;
    std::string prefix_;
    std::string dir_;
    std::ofstream ofs_;
    bool header_out_;
    factorial_t last_factorial_(const fs::path& filename) {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            throw std::system_error(errno, std::generic_category(),
                                    "Файл не открывается");
        }
        ifs.seekg(-1, std::ios_base::end);
        if (ifs.peek() != '\n') {
            throw std::out_of_range("Последняя строка не завершена");
        }
        ifs.seekg(-1, std::ios_base::cur);
        for (int i = ifs.tellg(); i >= 0;
                ifs.seekg(std::max(--i, 0), std::ios_base::beg)) {
            if (ifs.get() == '\n' && ifs.peek() != '#') {
                break;
            }
        }
        limb_t number, low, high, exp;
        ifs >> number >> low >> high >> exp;
        if (low > high) {
            throw std::out_of_range("low > high");
        }
        factorial_t f(number, low, high, exp, digits_, cond_prec_);
        if (f.low >= f.mod ||
            f.high > std::numeric_limits<limb2_t>::max()/f.mod) {
            throw std::out_of_range("low >= mod || high > max()/mod");
        }
        return f;
    }
 public:
    out_factorial_t(
            const std::string& comment = "",
            limb_t digits = std::numeric_limits<limb_t>::digits10,
            limb_t cond_prec = std::numeric_limits<double>::digits10,  // 15
            const std::string& dir = "out_i64/out",
            const std::string& prefix = "i64"):
        comment_{comment},
        digits_{digits},
        cond_prec_{cond_prec},
        prefix_{prefix + "-" + std::to_string(cond_prec_) + "-" +
                std::to_string(digits_) + "-"},
        dir_{dir}
    {}
    struct last_file_t {
        int nfile;
        factorial_t factorial;
    };
    last_file_t last_file() {
        int nfile = 0;
        factorial_t fac(digits_, cond_prec_);
        for (const auto& f : fs::directory_iterator(dir_)) {
            const auto& fs = f.path().filename().string();
            if (f.is_regular_file() && fs.starts_with(prefix_)) {
                try {
                    auto lf = last_factorial_(f.path());
                    if (lf.number > fac.number) {
                        auto nf = std::stoi(fs.substr(prefix_.size()));
                        if (0 < nf) {
                            nfile = nf;
                            fac = lf;
                        } else {
                            println(std::cerr,
                                    "Пропускаем неверное имя: '{}'", fs);
                        }
                    }
                }
                catch (const std::invalid_argument& ex) {
                    println(std::cerr,
                            "Пропускаем invalid_argument({})", ex.what());
                }
                catch (const std::out_of_range& ex) {
                    println(std::cerr,
                            "Пропускаем out_of_range({})", ex.what());
                }
                catch (const std::system_error& ex) {
                    // println(std::cerr,
                    //         "Пропускаем system_error({}, {}, {})",
                    //         ex.code(), ex.code().message(), ex.what());
                    println(std::cerr,
                            "Пропускаем system_error({}, {})",
                            ex.code().message(), ex.what());
                }
            }
        }
        return {nfile, fac};
    }
    void create_file(int nfile) {
        auto fn = std::format("{}/{}{:06d}.txt", dir_, prefix_, nfile);
        ofs_.open(fn, std::ios::noreplace | std::ios::trunc | std::ios::out);
        if (ofs_.is_open()) {
            header_out_ = false;
            return;
        }
        throw std::system_error(errno, std::generic_category(),
                                "Ошибка создания: " + fn);
    }
    void write(const factorial_t& f) {
        constexpr static const char fmt[] = "{:1}{:>12} {:>20} {:>20} {:>12}";
        if (!header_out_) {
            std::println(ofs_, fmt, "#",
                         "number", "low", "high", "exp");
            std::println(ofs_,
                    "# Чтение (vim:set et fileencoding=utf8:):\n"
                    "# t = np.loadtxt(fname, dtype=np.uint64)\n"
                    "# или\n"
                    "# tn = np.genfromtxt(fname, dtype=np.uint64, names=True)\n"
                    "# assert np.all(t[:,0] == tn['number'])");
            if (comment_.size()) {
                std::println(ofs_, "# {}", comment_);
            }
            std::println(ofs_, fmt, "#",
                         "number", "low", "high", "exp");
            header_out_ = true;
        }
        assert(f.low < f.mod);
        assert(f.high <= std::numeric_limits<limb_t>::max());  // TODO: limb2_t
        std::println(ofs_, fmt, " ",
                     f.number, limb_t(f.low), limb_t(f.high), f.exp);
        ofs_.flush();
    }
    const std::string& get_dir() const {
        return dir_;
    }
};
class random_selection_t {
    // Кнут, т.2, упр. 3.4.2-8(d) TODO: непонятно. Получилась реализация
    // несоответствующая тексту ввиду `assert()`. И есть ощущение
    // неравномерности.
    std::mt19937_64 gen_;
    std::uniform_real_distribution<double> U_;
    limb_t n_;
    limb_t N_;
public:
    static constexpr
    const limb_t npos = -1;
    random_selection_t(limb_t n, limb_t N):
        gen_(std::random_device{}()), n_(n), N_(N)
    {
        assert(n_ <= N_ && n_ && N_);
    }
    limb_t skip2next() {
        if (0 >= n_) {
            return npos;
        }
        if (0 >= N_) {
            return 0;
        }
        assert(n_ <= N_);
        double p = abs(1. - pow(U_(gen_), 1./n_));
        if (p >= 1.) {
            p = nextafter(1., 0.);
        }
        assert(0. <= p && p < 1.);
        limb_t X = (N_ - n_ + 1)*p;  // TODO: что-то не то
        assert(X <= N_ - n_);
        n_--;
        N_ -= X + 1;
        assert(N_ < (N_ + (X + 1)));
        return X;
    }
};

#ifndef CATCH2_SINGLE_INCLUDE
    #include <catch2/catch_all.hpp>
#else
    #undef CATCH_CONFIG_MAIN
    #include "catch2/catch.hpp"
#endif

#include <iomanip>
#include <iostream>
#include <locale>
#include <string>
#include <sstream>

void limb2_test(limb2_t n, const std::string& sn, int grouping = 0) {
    CAPTURE(n);
    struct grouping3 : std::numpunct<char> {
        char do_thousands_sep()   const { return '\''; } // separate with spaces
        std::string do_grouping() const { return "\3"; } // groups of 1 digit
    };
    struct grouping5 : std::numpunct<char> {
        char do_thousands_sep()   const { return '\''; } // separate with spaces
        std::string do_grouping() const { return "\5"; } // groups of 1 digit
    };
    std::ostringstream os;
    if (3 == grouping) {
        os.imbue(std::locale(os.getloc(), new grouping3));
    } if (5 == grouping) {
        os.imbue(std::locale(os.getloc(), new grouping5));
    } else {
        assert(0 == grouping);
    }
    os << n;
    CHECK(os.str() == sn);
}
TEST_CASE("limb2_t") {
    limb2_test(1'000, "1000");
    limb2_test(1'000'000'000'000'000'000, "1000000000000000000");
    limb2_test(1'000'000'000'000'003'000, "1000000000000003000");
    limb2_test((limb2_t(9'000'000'000'000'000'000)*
                limb2_t(9'000'000'000'000'000'000)),
               "81000000000000000000000000000000000000");
    limb2_test((limb2_t(9'000'000'000'000'000'000)*
                limb2_t(9'000'000'000'000'000'000)) + 500'000,
               "81000000000000000000000000000000500000");
    limb2_test((limb2_t(9'000'000'000'000'000'000)*
                limb2_t(9'000'000'000'000'000'000)) + 15'000'000'000'000'000,
               "81000000000000000000015000000000000000");
#if 0  // TODO
    limb2_test((limb2_t(9'000'000'000'000'000'000)*
                limb2_t(9'000'000'000'000'000'000)),
               "81'000'000'000'000'000'000'000'000'000'000'000'000", 3);
    limb2_test((limb2_t(9'000'000'000'000'000'000)*
                limb2_t(9'000'000'000'000'000'000)) + 15'000'000'000'000'000,
               "810'00000'00000'00000'00015'00000'00000'00000", 5);
#endif
}
TEST_CASE("ipow") {
    CHECK(ipow(10, 0) == 1);
    CHECK(ipow(10, 3) == 1'000);
    CHECK(ipow(10, 16) == 10'000'000'000'000'000ull);
    CHECK(ipow(10, 19) == 10'000'000'000'000'000'000ull);
}
#include <numeric>
#include <vector>
#include <utility>
TEST_CASE("random_selection_t") {
    const limb_t iters = 100'000;
    const limb_t st = 2;
    for(auto [m0, N] : (std::pair<limb_t, limb_t>[]){
                        {10, 10},
                        {10, 100},
                        {10, 1000} }) {
        auto m = std::min(m0, N - st);
        std::vector<limb_t> h(N);
        for (limb_t i = 0; i < iters; i++) {
            random_selection_t rs{m, N - st};
            limb_t n = st;
            limb_t x;
            while (rs.npos != (x = rs.skip2next())) {
                n += x;
                assert(n < N);
                h[n++]++;
            }
        }
        CAPTURE(m0, m, N, h[N-6], h[N-5], h[N-4], h[N-3], h[N-2], h[N-1]);
        CHECK(m*iters == std::reduce(h.begin() + st, h.end()));
    }
}
void factorial_test(const factorial_t &f,
                    limb2_t low,
                    limb2_t high,
                    limb_t exp,
                    limb_t exp_len = 0,
                    limb_t exp_limit = 0) {
    CAPTURE(f);
    CHECK(f.low == low);
    CHECK(f.high == high);
    CHECK(f.exp == exp);
    if (exp_len) {
        CHECK(f.exp_len == exp_len);
    }
    if (exp_limit) {
        CHECK(f.exp_limit == exp_limit);
    }
}
void cond_stop_test(const factorial_t &f, double max_lff[2]) {
    CAPTURE(f);
    CHECK(f.stop_conditional());
    if (f.high < f.mod) {
        double lfi;
        auto lf = l10factorial(f.number);
        auto lff = modf(lf, &lfi);
        int i = 0;
        if (lff > 0.5) {
            lff = 1. - lff;
            i = 1;
        }
        CAPTURE(lff, lf, pow(10., -double(f.cond_prec)));
        CHECK(lff/lf <= pow(10., -double(f.cond_prec)));
        max_lff[i] = std::max(max_lff[i], lff/lf);
    }
}
void lgamma_test(const factorial_t &f) {
    auto m10u = m10gamma(f.number, INFINITY);
    auto m10d = m10gamma(f.number, -INFINITY);
    CAPTURE(f);
    CHECK(f.low_m10() <= m10u);
    CHECK(m10d <= f.high_m10());
}
void lgamma_loss(const factorial_t &f) {
    CAPTURE(f);
    double lgi;
    auto lg = l10gamma(f.number + 1);
    auto lgf = modf(lg, &lgi);
    CHECK(lgf == 0.);
}
TEST_CASE("factorial simple") {
    factorial_t f;
    f.next(0);
    factorial_test(f, 1, 1, 0, 1, 10);
    lgamma_test(f);
    f.next(1);
    factorial_test(f, 1, 1, 0, 1, 10);
    lgamma_test(f);
    f.next(2);
    factorial_test(f, 2, 2, 0, 1, 10);
    lgamma_test(f);
    f.next(3);
    factorial_test(f, 6, 6, 0, 1, 10);
    lgamma_test(f);
    factorial_t f1;
    f1.next(3);
    factorial_test(f1, 6, 6, 0, 1, 10);
    lgamma_test(f1);
}
TEST_CASE("factorial medium") {
    factorial_t f(10);
    f.next(12);
    factorial_test(f, 479001600, 479001600, 0, 1, 10);
    lgamma_test(f);
    f.next(13);
    factorial_test(f, 6227020800, 6227020800, 0, 1, 10);
    lgamma_test(f);
    f.next(14);    // 87178291200
    factorial_test(f, 8717829120, 8717829120, 1, 1, 10);
    lgamma_test(f);
    f.next(15);    // 1307674368000
    factorial_test(f, 1307674368, 1307674368, 3, 1, 10);
    lgamma_test(f);
    f.next(16);    // 20922789888000
    factorial_test(f, 2092278988, 2092278989, 4, 1, 10);
    lgamma_test(f);
    f.next(30);    // 265252859812191058636308480000000
    factorial_test(f, 2652528590, 2652528606, 23, 2, 100);
    lgamma_test(f);
    factorial_t f1(10);
    f1.next(30);
    factorial_test(f1, 2652528590, 2652528606, 23, 2, 100);
    lgamma_test(f1);
}
TEST_CASE("factorial cond") {
    factorial_t f;
    const limb_t cond_prec = 15;
    f.set_cond_prec(cond_prec);
    double max_lff[2] = {0., 0.};
    int cnt = 0;
    while (!f.next(50'000'000)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(14 == cnt);
    factorial_test(f, 2345380369906790482, 2345380369952621778,
                   363233762, 9, 1'000'000'000);
    lgamma_test(f);
    CHECK(max_lff[0] >= pow(10., -double(cond_prec + 1)));
    CHECK(max_lff[1] >= pow(10., -double(cond_prec + 1)));
}
TEST_CASE("factorial complex", "[.][slow][slowest]") {
    factorial_t f(10);
    f.set_cond_prec(15);
    int cnt = 0;
    while (!f.next(5'000'000)) {
        cnt++;
    }
    CHECK(2177 == cnt);
    factorial_test(f, 2292527031, 2297008862, 31323372, 8, 100'000'000);
    lgamma_test(f);
    while (!f.next(50'000'000)) {
        cnt++;
    }
    CHECK(210653 == cnt);
    factorial_test(f, 2322570410, 2368192477, 363233771, 9, 1'000'000'000);
    lgamma_test(f);
}
TEST_CASE("factorial lgamma", "[.][slow][slowest]") {
    factorial_t f;
    const limb_t cond_prec = 15;
    f.set_cond_prec(cond_prec);
    double max_lff[2] = {0., 0.};
    int cnt = 0;
    while (!f.next(44'315'509)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(9 == cnt);
    factorial_test(f, 1000000098605330261ull, 1000000098622648976ull,
                   319615015, 9, 1'000'000'000);
    lgamma_test(f);
    CHECK(f.stop_conditional());
    while (!f.next(46'464'264)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(9 == cnt);
    factorial_test(f, 9999999489593805517ull, 9999999489775394465ull,
                   336067856, 9, 1'000'000'000);
    // Не хватает точности: lgamma_test(f);
    lgamma_loss(f);
    CHECK(f.stop_conditional());
    while (!f.next(48'655'817)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(10 == cnt);
    factorial_test(f, 1000000035367005029ull, 1000000035386020899ull,
                   352892856, 9, 1'000'000'000);
    // Не хватает точности: lgamma_test(f);
    lgamma_loss(f);
    CAPTURE(f);
    CHECK(f.stop_conditional());
    // CPython имеет собственная реализация гамма-функции и она немного хуже,
    // чем BSD/macOS/Microsoft и даже glibc.
    CHECK(0x1.508b7c9ffffffp+28 != lgamma(48655818)*_count_log10_e);
    CHECK(352892874.0 == lgamma(48655818)*_count_log10_e);
    while (!f.next(54'336'595)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(12 == cnt);
    factorial_test(f, 9999999716133277224ull, 9999999716345636016ull,
                   396700486, 9, 1'000'000'000);
    // Не хватает точности: lgamma_test(f);
    lgamma_loss(f);
    CHECK(f.stop_conditional());
    CHECK(max_lff[0] >= pow(10., -double(cond_prec + 1)));
    CHECK(max_lff[1] >= pow(10., -double(cond_prec + 1)));
}
TEST_CASE("factorial sum log10") {
    factorial_t f;
    const limb_t cond_prec = 13;
    f.set_cond_prec(cond_prec);
    double max_lff[2] = {0., 0.};
    int cnt = 0;
    while (!f.next(3'121'515)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(3 == cnt);
    factorial_test(f, 1000000695128040535ull, 1000000695129260268ull,
                   18916588);
    lgamma_test(f);
    while (!f.next(8'783'619)) {
        cond_stop_test(f, max_lff);
        cnt++;
    }
    CHECK(15 == cnt);
    factorial_test(f, 9999992763427618606ull, 9999992763461943734ull,
                   57175890, 8, 100'000'000);
    lgamma_test(f);
    CHECK(max_lff[0] >= pow(10., -double(cond_prec + 1)));
    CHECK(max_lff[1] >= pow(10., -double(cond_prec + 1)));
    factorial_t f1;
    f1.set_cond_prec(cond_prec);
    cnt = 0;
    while (!f1.next(8'783'619)) {
        cnt++;
    }
    CHECK(16 == cnt);
    factorial_test(f1, 9999992763427618606ull, 9999992763461943734ull,
                   57175890, 8, 100'000'000);
    lgamma_test(f1);
}
TEST_CASE("factorial slow-19-15", "[.][slowest]") {
    const limb_t cond_prec = 15;
    factorial_t f(19, cond_prec);
    int cnt = 0;
    int ccnt = 0;
    auto dn = 50'000'000ull;
    for (auto n = dn; n <= 500'000'000ull; n += dn) {
        int lcnt = 0;
        int lccnt = 0;
        double max_lff[2] = {0., 0.};
        while (!f.next(n)) {
            cond_stop_test(f, max_lff);
            cnt++;
            lcnt++;
            if (f.high >= f.mod) {
                ccnt++;
                lccnt++;
            }
        }
        INFO(n << ": " << lcnt << " (" << cnt << "): "
             << lccnt << " (" << ccnt << ')');
        CHECK(max_lff[0] >= pow(10., -double(cond_prec + 1)));
        CHECK(max_lff[1] >= pow(10., -double(cond_prec + 1)));
    }
    CHECK(749 == cnt);  // 14 - 7668
    CHECK(0 == ccnt);
    factorial_test(f, 9224831135243578245ull, 9224831137046357630ull,
                   4132337747, 10, 10'000'000'000);
    lgamma_test(f);
}
TEST_CASE("factorial overflow-10-15") {
    const limb_t cond_prec = 15;
    factorial_t f(10, cond_prec);
    factorial_t lf;
    int cnt = 0;
    int ccnt = 0;
    auto dn = 5'000'000ull;
    for (auto n = dn; n <= 25'000'000ull; n += dn) {
        int lcnt = 0;
        int lccnt = 0;
        // TODO: double max_lff[2] = {0., 0.};
        while (!f.next(n)) {
            // TODO: cond_stop_test(f, max_lff);
            cnt++;
            lcnt++;
            if (f.high >= f.mod) {
                ccnt++;
                lccnt++;
                if (f.high >= lf.high) {
                    lf = f;
                }
            }
        }
        INFO(n << ": " << lcnt << " (" << cnt << "): "
             << lccnt << " (" << ccnt << ')');
        // TODO: CHECK(max_lff[0] >= pow(10., -double(cond_prec + 1)));
        // TODO: CHECK(max_lff[1] >= pow(10., -double(cond_prec + 1)));
    }
    INFO("lf=" << lf);  // high=12051608284 number=499792292
    CHECK(52682 == cnt);
    CHECK(52679 == ccnt);
    factorial_test(f, 1841437453, 1859481248, 174091133,
                   9, 1'000'000'000);
    lgamma_test(f);
}
TEST_CASE("out factorial base") {
    const char *path = "out_i64/test/base/.";
    const limb_t cond_prec = 14;
    const limb_t digits = 10;
    const limb_t number = 1'000'000;
    std::filesystem::create_directories(path);
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    out_factorial_t of(std::format("Randomness seeded to: {}",
                                   Catch::rngSeed()),
                       digits, cond_prec, path);
    factorial_t f(digits, cond_prec);
    of.create_file(1);
    int cnt = 0;
    while (!f.next(number)) {
        of.write(f);
        cnt++;
    }
    REQUIRE(number == f.number);
    REQUIRE_FALSE(f.stop_conditional());
    of.write(f);
    cnt++;
    CHECK(92 == cnt);
    auto lf = of.last_file();
    CHECK(1 == lf.nfile);
    CHECK(number == lf.factorial.number);
    CHECK(lf.factorial.low == f.low);
    CHECK(lf.factorial.high == f.high);
    CHECK(lf.factorial.exp == f.exp);
    CHECK(lf.factorial.exp_len == f.exp_len);
    CHECK(lf.factorial.digits == f.digits);
    CHECK(lf.factorial.mod == f.mod);
    CHECK(lf.factorial.cond_prec == f.cond_prec);
    CHECK(lf.factorial.low_cond == f.low_cond);
    CHECK(lf.factorial.high_cond == f.high_cond);
}
TEST_CASE("out factorial iter") {
    const char *path = "out_i64/test/iter/.";
    const limb_t cond_prec = 14;
    const limb_t digits = 10;
    const limb_t in = 999'382;
    const int im = 100;
    std::filesystem::create_directories(path);
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    limb_t number = 1;
    for (int i = 0; i < 3; i++) {
        out_factorial_t of(std::format("Randomness seeded to: {}",
                                       Catch::rngSeed()),
                           digits, cond_prec, path);
        auto lf = of.last_file();
        CHECK(number == lf.factorial.number);
        CHECK(i == lf.nfile);
        of.create_file(i + 1);
        int cnt = 0;

        random_selection_t rs{im, (i + 1)*in - number};
        limb_t x;
        while (rs.npos != (x = rs.skip2next())) {
            number += x + 1;
            while (!lf.factorial.next(number)) {
                of.write(lf.factorial);
                cnt++;
            }
            of.write(lf.factorial);
            cnt++;
        }
        CHECK(number <= (i + 1)*in);
        if (number < (i + 1)*in) {
            number = (i + 1)*in;
            while (!lf.factorial.next(number)) {
                of.write(lf.factorial);
                cnt++;
            }
            of.write(lf.factorial);
            cnt++;
        }
        REQUIRE(number == lf.factorial.number);
        if (i) {
            CHECK_FALSE(lf.factorial.stop_conditional());
        } else {
            REQUIRE(lf.factorial.stop_conditional());
        }
        CAPTURE(cnt);
        CHECK(im*(i + 1) < cnt);
    }
}
TEST_CASE("work-15-500m", "[.][work]") {
    const limb_t cond_prec = 15;
    const limb_t digits = 19;
    const limb_t in = 500'000'000;
    const int im = 1000;
    out_factorial_t of(std::format("Randomness seeded to: {}",
                                   Catch::rngSeed()),
                       digits, cond_prec);
    std::filesystem::create_directories(of.get_dir());
    auto lf = of.last_file();
    auto number = lf.factorial.number;
    auto last = number + in;
    last -= last%in;
    of.create_file(lf.nfile + 1);
    random_selection_t rs{im, last - number};
    limb_t x;
    while (rs.npos != (x = rs.skip2next())) {
        number += x + 1;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(number <= last);
    if (number < last) {
        number = last;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(last == lf.factorial.number);
}
TEST_CASE("work-16-1g", "[.][work]") {
    const limb_t cond_prec = 16;
    const limb_t digits = 19;
    const limb_t in = 1'000'000'000;
    const int im = 2000;
    out_factorial_t of(std::format("Randomness seeded to: {}",
                                   Catch::rngSeed()),
                       digits, cond_prec);
    std::filesystem::create_directories(of.get_dir());
    auto lf = of.last_file();
    auto number = lf.factorial.number;
    auto last = number + in;
    last -= last%in;
    of.create_file(lf.nfile + 1);
    random_selection_t rs{im, last - number};
    limb_t x;
    while (rs.npos != (x = rs.skip2next())) {
        number += x + 1;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(number <= last);
    if (number < last) {
        number = last;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(last == lf.factorial.number);
}
TEST_CASE("work-17-1g", "[.][work]") {
    const limb_t cond_prec = 17;
    const limb_t digits = 19;
    const limb_t in = 1'000'000'000;
    const int im = 2000;
    out_factorial_t of(std::format("Randomness seeded to: {}",
                                   Catch::rngSeed()),
                       digits, cond_prec);
    std::filesystem::create_directories(of.get_dir());
    auto lf = of.last_file();
    auto number = lf.factorial.number;
    auto last = number + in;
    last -= last%in;
    of.create_file(lf.nfile + 1);
    random_selection_t rs{im, last - number};
    limb_t x;
    while (rs.npos != (x = rs.skip2next())) {
        number += x + 1;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(number <= last);
    if (number < last) {
        number = last;
        while (!lf.factorial.next(number)) {
            of.write(lf.factorial);
        }
        of.write(lf.factorial);
    }
    CHECK(last == lf.factorial.number);
}
