#pragma once

#include <cstddef>

// Результат одного вычисления.
struct Result {
    double value;    // значение интеграла
    double seconds;  // затраченное время
};

// Вариант 1: стандарт POSIX (pthread_create / pthread_join).
// Частичная сумма возвращается потоком и принимается в главном потоке
// через второй параметр pthread_join.
Result trapezoidPthreads(double a, double b, std::size_t steps, int threads);

// Вариант 2: std::thread. Параметры функции потока передаются по отдельности.
Result trapezoidStdThread(double a, double b, std::size_t steps, int threads);
