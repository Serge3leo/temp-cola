// vim:set sw=4 ts=8 et fileencoding=utf8::Кодировка:UTF-8[АБЁЪЯабёъя]
// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Сергей Леонтьев (leo@sai.msu.ru)
// История:
// 2025-04-29 05:32:12 - Создан.
//

#include <cassert>
#include <cfloat>

#include <Eigen/Dense>
#include <Eigen/QR>

using namespace Eigen;

Vector2d fixed_point(Vector2d& rm, Matrix<double, 4, 2>& cr) {
    // Находит неподвижную точку аффинного преобразования:
    // (0,0, rm[0],0, rm[0],rm[1], 0,rm[1]) -> cr
    //
    // rm - размеры переговорной
    // cr - координаты углов плана переговорной внутри переговорной
    Matrix<double, 4, 3> rt{{0.,    0.,    1.},
                            {rm[0], 0.,    1.},
                            {rm[0], rm[1], 1.},
                            {0.,    rm[1], 1.}};
    Matrix<double, 4, 3> ct;
    ct.block<4, 2>(0, 0) = cr;
    ct.col(2).setOnes();
    Matrix3d a;  // Матрица аффинного преобразования
    a = rt.block<3,3>(0,0).householderQr().solve(ct.block<3,3>(0,0))
                          .transpose();
    Matrix3d ai{a};                                 // (A - I)*f == 0
    ai.topLeftCorner(2,2) -= Matrix2d::Identity();  // f[2] == 1
    Vector3d f = ai.householderQr().solve(Vector3d::UnitZ());
    assert(ct.transpose().isApprox(a*rt.transpose(), 1e-4) && "Нехороший план");
    assert(Vector3d::UnitZ().isApprox(ai*f, 20*DBL_EPSILON)
           && "Нехороший план");
    assert(f.isApprox(a*f, 20*DBL_EPSILON) && "Нехороший план");
    return f.head<2>();
}

// Тесты
#include <cmath>
#include <iostream>

struct test_case {
    Vector2d rm;
    Matrix<double, 4, 2> cr;
    Vector2d fpt;
};
test_case test_cases[] = {
    {{10, 5},
     Matrix<double, 4, 2>{{4.5, 2.0}, {4.5, 4.0}, {5.5, 4.0}, {5.5, 2.0}},
     {245./48., 145./48.}},
    {{10, 5},
     Matrix<double, 4, 2>{{4.5, 4.0}, {4.5, 2.0}, {5.5, 2.0}, {5.5, 4.0}},
     {265./52., 155./52.}},
    {{10, 5},
     Matrix<double, 4, 2>{{5.5, 4.0}, {5.5, 2.0}, {4.5, 2.0}, {4.5, 4.0}},
     {235./48., 145./48.}},
    {{10, 5},
     Matrix<double, 4, 2>{{5.5, 2.0}, {5.5, 4.0}, {4.5, 4.0}, {4.5, 2.0}},
     {255./52., 155./52.}},

    {{10, 5},
     Matrix<double, 4, 2>{{1.0, 1.5}, {3.0, 1.5}, {3.0, 2.5}, {1.0, 2.5}},
     {1.25, 1.875}},
    {{10, 5},
     Matrix<double, 4, 2>{{3.0, 1.5}, {1.0, 1.5}, {1.0, 2.5}, {3.0, 2.5}},
     {2.5, 1.875}},
    {{10, 5},
     Matrix<double, 4, 2>{{3.0, 2.5}, {1.0, 2.5}, {1.0, 1.5}, {3.0, 1.5}},
     {2.5, 12.5 / 6}},
    {{10, 5},
     Matrix<double, 4, 2>{{1.0, 2.5}, {3.0, 2.5}, {3.0, 1.5}, {1.0, 1.5}},
     {1.25, 12.5 / 6}},

    {{10, 5},
     Matrix<double, 4, 2>{{1.5, 2.5}, {3.5, 4.5}, {4.5, 3.5}, {2.5, 1.5}},
     {2.5, 2.5}},
    {{10, 5},
     Matrix<double, 4, 2>{{1.5, 2.5}, {4.5, 4.5}, {5.5, 3.5}, {2.5, 1.5}},
     {2.875, 2.5625}}
};
template <class T>
T ulp_diff(T a, T b) {
    if (a == b) {
        return T(0);
    }
    if (std::abs(a) >= std::abs(b)) {
        return (a - b)/(std::nextafter(b, a) - b);
    }
    return (b - a)/(std::nextafter(a, b) - a);
}
int main() {
    for (auto& t : test_cases) {
        // t.cr += 1e-5*Matrix<double, 4, 2>::Random();  // Точность 5 знаков
        Vector2d fpt = fixed_point(t.rm, t.cr);
        std::cout << ulp_diff(t.fpt[0], fpt[0]) << ", "
                  << ulp_diff(t.fpt[1], fpt[1]) << " ULP\n";
        assert(ulp_diff(t.fpt[0], fpt[0]) <= 5.);
        assert(ulp_diff(t.fpt[1], fpt[1]) <= 5.);
    }
    return 0;
}
