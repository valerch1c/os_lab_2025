#include "factorial.h"

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    uint64_t start = args->begin;
    
    // Если начало 0, начинаем с 1 (0! = 1 по определению)
    if (start == 0) start = 1;
    
    for (uint64_t i = start; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    
    return ans;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    
    if (errno == ERANGE) {
        return false;
    }
    
    if (errno != 0 || *end != '\0') {
        return false;
    }
    
    *val = i;
    return true;
}