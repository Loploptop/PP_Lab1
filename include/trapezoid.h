#pragma once

#include <cstddef>

// Подынтегральная функция: f(x) = exp(-x^2) на [0; 1].
// Точное значение: (sqrt(pi)/2) * erf(1) = 0.746824132812427.
double f(double x);

// Точное значение интеграла (для проверки результата).
double exactIntegral(double a, double b);

// Диапазон элементарных отрезков (firstStep; lastStep], закреплённый за потоком.
// Поток считает узлы x_(firstStep+1) ... x_(lastStep) включительно: узел на стыке
// двух участков попадает ровно в один из них, поэтому в общей сумме он
// учитывается один раз с полным весом h.
struct StepRange {
    std::size_t firstStep;
    std::size_t lastStep;
};

// Делит steps отрезков между threads потоками: первые (steps % threads) потоков
// получают на один отрезок больше.
StepRange splitSteps(std::size_t steps, int threads, int index);

// Сумма f(x) в узлах участка (без деления концов на 2).
double nodeSum(double a, double h, const StepRange& r);

// Формула трапеций: I = h * (f(a)/2 + sum + f(b)/2).
double trapezoid(double a, double b, double h, double sum);
