// Два варианта параллельного вычисления интеграла методом трапеций:
//   1) POSIX  - pthread_create / pthread_join;
//   2) std::thread.
#include "threads.h"

#include <pthread.h>

#include <chrono>
#include <cstdlib>
#include <functional>
#include <thread>
#include <vector>

#include "trapezoid.h"

// ===========================================================================
//  Вариант 1: POSIX
// ===========================================================================

namespace {

// Функция потока в POSIX принимает ровно один аргумент, поэтому параметры
// передаются указателем на структуру.
struct ThreadArgs {
    double a;         // начало отрезка
    double h;         // шаг
    StepRange range;  // участок потока
};

// Возвращаем частичную сумму через возвращаемое значение потока:
// адрес значения, выделенного в динамической памяти (адрес локальной
// переменной возвращать нельзя - её память к моменту join уже не существует).
void* worker(void* arg) {
    const ThreadArgs* p = static_cast<const ThreadArgs*>(arg);
    double* sum = static_cast<double*>(std::malloc(sizeof(double)));
    if (sum != nullptr) {
        *sum = nodeSum(p->a, p->h, p->range);
    }
    return sum;
}

}  // namespace

Result trapezoidPthreads(double a, double b, std::size_t steps, int threads) {
    const double h = (b - a) / static_cast<double>(steps);

    std::vector<ThreadArgs> args(static_cast<std::size_t>(threads));
    std::vector<pthread_t> tid(static_cast<std::size_t>(threads));

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < threads; ++i) {
        args[i].a = a;
        args[i].h = h;
        args[i].range = splitSteps(steps, threads, i);
        pthread_create(&tid[i], nullptr, worker, &args[i]);
    }

    // Именно pthread_join возвращает то, что вернул поток.
    double sum = 0.0;
    for (int i = 0; i < threads; ++i) {
        void* ret = nullptr;
        pthread_join(tid[i], &ret);
        const double* partial = static_cast<const double*>(ret);
        if (partial != nullptr) {
            sum += *partial;
            std::free(ret);
        }
    }

    const auto finish = std::chrono::steady_clock::now();

    Result result;
    result.value = trapezoid(a, b, h, sum);
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
}

// ===========================================================================
//  Вариант 2: std::thread
// ===========================================================================

namespace {

// Параметры передаются по отдельности, без объединения в структуру.
void workerStd(double a, double h, std::size_t first, std::size_t last,
               double& partialSum) {
    partialSum = nodeSum(a, h, StepRange{first, last});
}

}  // namespace

Result trapezoidStdThread(double a, double b, std::size_t steps, int threads) {
    const double h = (b - a) / static_cast<double>(steps);

    // Каждый поток пишет в свой элемент, поэтому синхронизация не нужна.
    std::vector<double> partial(static_cast<std::size_t>(threads), 0.0);
    std::vector<std::thread> pool;

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < threads; ++i) {
        const StepRange r = splitSteps(steps, threads, i);
        pool.emplace_back(workerStd, a, h, r.firstStep, r.lastStep, std::ref(partial[i]));
    }
    for (int i = 0; i < threads; ++i) {
        pool[i].join();
    }

    double sum = 0.0;
    for (int i = 0; i < threads; ++i) {
        sum += partial[i];
    }

    const auto finish = std::chrono::steady_clock::now();

    Result result;
    result.value = trapezoid(a, b, h, sum);
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
}
