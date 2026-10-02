#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "threads.h"
#include "trapezoid.h"

namespace {

const double kA = 0.0;  // пределы интегрирования
const double kB = 1.0;

const int kMinThreads = 1;
const int kMaxThreads = 8;
const int kRuns = 7;

std::size_t readSteps(int argc, char** argv) {
    if (argc > 1) {
        if (argv[1][0] == 'h' && argv[1][1] == '=') {
            const double h = std::atof(argv[1] + 2);
            if (h > 0.0) {
                return static_cast<std::size_t>(std::ceil((kB - kA) / h));
            }
            std::printf("Шаг должен быть положительным. Взято значение по умолчанию.\n");
        } else {
            const long long n = std::atoll(argv[1]);
            if (n > 0) {
                return static_cast<std::size_t>(n);
            }
            std::printf("Число шагов должно быть положительным. Взято значение по умолчанию.\n");
        }
    }
    return 10000000;
}

// Запускает вариант kRuns раз и возвращает результат с лучшим временем:
// время «плавает» из-за планировщика и других процессов, а лучшее значение
// меньше всего от них зависит.
Result bestOf(Result (*run)(double, double, std::size_t, int), std::size_t steps, int threads) {
    Result best = run(kA, kB, steps, threads);
    for (int i = 1; i < kRuns; ++i) {
        const Result r = run(kA, kB, steps, threads);
        if (r.seconds < best.seconds) {
            best = r;
        }
    }
    return best;
}

double speedup(double base, double current) {
    return current > 0.0 ? base / current : 0.0;
}

void setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

}  

int main(int argc, char** argv) {
    setupConsole();

    const std::size_t steps = readSteps(argc, argv);
    const double h = (kB - kA) / static_cast<double>(steps);
    const double exact = exactIntegral(kA, kB);

    std::printf("Метод трапеций для f(x) = exp(-x^2) на отрезке [%g; %g]\n", kA, kB);
    std::printf("Шагов: %llu, шаг h = %.12g\n\n", static_cast<unsigned long long>(steps), h);
    std::printf("Замеров на точку: %d, в таблице показано лучшее (минимальное) время.\n",
                kRuns);
    std::printf("Ускорение = время в 1 поток / время в N потоков (идеал для N потоков: N).\n\n");

    // Прогрев: первый запуск включает разовые накладные расходы (загрузка кода,
    // выделение страниц, создание пула потоков).
    for (int i = 0; i < 2; ++i) {
        trapezoidPthreads(kA, kB, steps, kMinThreads);
        trapezoidStdThread(kA, kB, steps, kMinThreads);
    }

    double baseP = 0.0;  // время в один поток - база для ускорения
    double baseS = 0.0;
    double valueP = 0.0;  // значения интеграла (должны совпадать)
    double valueS = 0.0;
    double maxDiff = 0.0;

    std::printf("потоков |    POSIX, с | ускорение | std::thread, с | ускорение\n");
    std::printf("--------+-------------+-----------+----------------+-----------\n");

    for (int threads = kMinThreads; threads <= kMaxThreads; ++threads) {
        const Result p = bestOf(trapezoidPthreads, steps, threads);
        const Result s = bestOf(trapezoidStdThread, steps, threads);

        if (threads == kMinThreads) {
            baseP = p.seconds;
            baseS = s.seconds;
            valueP = p.value;
            valueS = s.value;
        }
        if (std::fabs(p.value - valueP) > maxDiff) {
            maxDiff = std::fabs(p.value - valueP);
        }
        if (std::fabs(s.value - valueS) > maxDiff) {
            maxDiff = std::fabs(s.value - valueS);
        }

        std::printf("%7d | %11.6f | %9.2f | %14.6f | %9.2f\n", threads, p.seconds,
                    speedup(baseP, p.seconds), s.seconds, speedup(baseS, s.seconds));
    }

    std::printf("\n");
    std::printf("Значение интеграла: POSIX %.15f, std::thread %.15f\n", valueP, valueS);
    std::printf("Эталонное значение: %.15f\n", exact);
    std::printf("Погрешность метода на %llu шагах: %.3e %%\n",
                static_cast<unsigned long long>(steps),
                std::fabs(valueP - exact) / exact * 100.0);
    std::printf("Максимальное расхождение значения между числом потоков: %.3e\n", maxDiff);

    return 0;
}
