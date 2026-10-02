#include "trapezoid.h"

#include <cmath>

double f(double x) {
    return std::exp(-x * x);
}

double exactIntegral(double a, double b) {
    // Int exp(-x^2) dx = sqrt(pi)/2 * erf(x)
    return std::sqrt(std::acos(-1.0)) / 2.0 * (std::erf(b) - std::erf(a));
}

StepRange splitSteps(std::size_t steps, int threads, int index) {
    const std::size_t n = static_cast<std::size_t>(threads);
    const std::size_t i = static_cast<std::size_t>(index);
    const std::size_t base = steps / n;    // отрезков на поток
    const std::size_t extra = steps % n;   // первые extra потоков получают +1

    StepRange r;
    r.firstStep = i * base + (i < extra ? i : extra);
    r.lastStep = r.firstStep + base + (i < extra ? 1 : 0);
    return r;
}

double nodeSum(double a, double h, const StepRange& r) {
    double sum = 0.0;
    for (std::size_t k = r.firstStep + 1; k <= r.lastStep; ++k) {
        sum += f(a + static_cast<double>(k) * h);
    }
    return sum;
}

double trapezoid(double a, double b, double h, double sum) {
    return h * (0.5 * f(a) + sum + 0.5 * f(b));
}
