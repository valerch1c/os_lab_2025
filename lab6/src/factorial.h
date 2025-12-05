#ifndef FACTORIAL_H
#define FACTORIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

// Структура для передачи аргументов факториала
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Умножение по модулю (защита от переполнения)
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

// Вычисление факториала в диапазоне по модулю
uint64_t Factorial(const struct FactorialArgs *args);

// Преобразование строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif // FACTORIAL_H