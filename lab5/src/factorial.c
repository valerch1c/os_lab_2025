#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Глобальные переменные для разделения между потоками
long long result = 1;          // Результат вычисления факториала
int k = 0;                     // Число, факториал которого вычисляем
int mod = 0;                   // Модуль
int pnum = 1;                  // Количество потоков
pthread_mutex_t mutex;         // Мьютекс для синхронизации

// Структура для передачи данных в поток
typedef struct {
    int thread_id;             // ID потока (от 0 до pnum-1)
    int start;                 // Начало диапазона для этого потока
    int end;                   // Конец диапазона для этого потока
} thread_data;

// Функция, которую выполняет каждый поток
void* calculate_range(void* arg) {
    // Преобразуем void* указатель обратно в thread_data*
    thread_data* data = (thread_data*)arg;
    long long local_result = 1;  // Локальный результат для этого потока
    
    // Выводим информацию о том, какой диапазон обрабатывает поток
    printf("Поток %d: вычисляю диапазон %d-%d\n", 
           data->thread_id, data->start, data->end);
    
    // Вычисляем произведение чисел в своем диапазоне
    for (int i = data->start; i <= data->end; i++) {
        // Умножаем текущий локальный результат на следующее число и берем по модулю
        // Это предотвращает переполнение и ускоряет вычисления
        local_result = (local_result * i) % mod;
        printf("Поток %d: умножаю на %d, промежуточный результат: %lld\n", 
               data->thread_id, i, local_result);
    }
    
    // КРИТИЧЕСКАЯ СЕКЦИЯ: начинаем работу с общими данными
    // Блокируем мьютекс - только один поток может выполнять следующий блок кода
    pthread_mutex_lock(&mutex);
    printf("Поток %d: блокирую мьютекс, текущий общий результат: %lld\n", 
           data->thread_id, result);
    
    // Умножаем общий результат на локальный результат потока и берем по модулю
    // Эта операция должна быть атомарной - защищена мьютексом
    result = (result * local_result) % mod;
    
    printf("Поток %d: умножаю на свой результат %lld, новый общий: %lld\n", 
           data->thread_id, local_result, result);
    printf("Поток %d: разблокирую мьютекс\n", data->thread_id);
    // Разблокируем мьютекс - другие потоки теперь могут войти в критическую секцию
    pthread_mutex_unlock(&mutex);
    
    free(data);  // Освобождаем память, выделенную для структуры данных потока
    return NULL;  // Возвращаем NULL, так как результат передается через глобальную переменную
}

// Функция разбора аргументов командной строки
void parse_arguments(int argc, char* argv[]) {
    printf("Аргументы командной строки:\n");
    // Выводим все аргументы для отладки
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = '%s'\n", i, argv[i]);
    }
    
    // Обрабатываем аргументы начиная с 1 (0-й - имя программы)
    for (int i = 1; i < argc; i++) {
        printf("Обрабатываем аргумент %d: '%s'\n", i, argv[i]);
        
        // Обрабатываем формат: -k <число>
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);  // atoi преобразует строку в число
            printf("  Найдено k = %d\n", k);
        } 
        // Обрабатываем формат: --pnum <число>
        else if (strcmp(argv[i], "--pnum") == 0 && i + 1 < argc) {
            pnum = atoi(argv[++i]);
            printf("  Найдено pnum = %d\n", pnum);
        } 
        // Обрабатываем формат: --mod <число>
        else if (strcmp(argv[i], "--mod") == 0 && i + 1 < argc) {
            mod = atoi(argv[++i]);
            printf("  Найдено mod = %d\n", mod);
        } 
        // Обрабатываем формат: --pnum=<число>
        else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            // atoi(argv[i] + 7) - преобразуем часть строки после "="
            pnum = atoi(argv[i] + 7);
            printf("  Найдено pnum = %d (формат --pnum=value)\n", pnum);
        } 
        // Обрабатываем формат: --mod=<число>
        else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
            printf("  Найдено mod = %d (формат --mod=value)\n", mod);
        } else {
            printf("  Неизвестный аргумент: '%s'\n", argv[i]);
        }
    }
    
    printf("После парсинга: k=%d, pnum=%d, mod=%d\n", k, pnum, mod);
    
    // Проверка корректности введенных данных
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Ошибка: все параметры должны быть положительными числами\n");
        printf("Текущие значения: k=%d, pnum=%d, mod=%d\n", k, pnum, mod);
        printf("Использование: %s -k <число> --pnum <потоки> --mod <модуль>\n", argv[0]);
        printf("Или: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
        exit(1);  // Завершаем программу с кодом ошибки
    }
    
    // Если потоков больше чем чисел для вычисления - корректируем
    // Нет смысла создавать больше потоков чем чисел для обработки
    if (pnum > k) {
        printf("Предупреждение: количество потоков уменьшено с %d до %d\n", pnum, k);
        pnum = k;
    }
}

int main(int argc, char* argv[]) {
    // Разбор аргументов командной строки
    parse_arguments(argc, argv);
    
    printf("\nВычисление %d! mod %d в %d потоках\n", k, mod, pnum);
    
    // Инициализация мьютекса
    // Мьютекс необходим для синхронизации доступа к общей переменной result
    pthread_mutex_init(&mutex, NULL);
    
    // Создание массивов для потоков и их данных
    pthread_t threads[pnum];  // Массив идентификаторов потоков
    int numbers_per_thread = k / pnum;  // Базовое количество чисел на поток
    int remainder = k % pnum;           // Остаток чисел для распределения
    int current_start = 1;              // Начинаем с 1 (факториал начинается с 1)
    
    // Создание и запуск потоков
    for (int i = 0; i < pnum; i++) {
        // Вычисление диапазона для текущего потока
        int range_size = numbers_per_thread;
        // Распределяем остаток по первым потокам для равномерной нагрузки
        if (i < remainder) {
            range_size++;  // Этот поток получит на одно число больше
        }
        
        int current_end = current_start + range_size - 1;
        
        // Создание структуры данных для потока
        // Выделяем память под структуру, которая будет передана в поток
        thread_data* data = (thread_data*)malloc(sizeof(thread_data));
        data->thread_id = i;        // Номер потока
        data->start = current_start; // Начало диапазона
        data->end = current_end;     // Конец диапазона
        
        printf("Создаю поток %d для диапазона %d-%d\n", i, current_start, current_end);
        
        // Создание потока
        // pthread_create создает новый поток который начинает выполнять calculate_range
        if (pthread_create(&threads[i], NULL, calculate_range, data) != 0) {
            perror("Ошибка создания потока");
            exit(1);
        }
        
        // Сдвигаем начало для следующего потока
        current_start = current_end + 1;
    }
    
    // Ожидание завершения всех потоков
    // pthread_join блокирует выполнение main до завершения указанного потока
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка ожидания потока");
            exit(1);
        }
    }
    
    // Уничтожение мьютекса - освобождение ресурсов
    pthread_mutex_destroy(&mutex);
    
    // Вывод результата
    printf("\nРезультат: %d! mod %d = %lld\n", k, mod, result);
    
    // Проверка: последовательное вычисление для верификации
    // Это демонстрация того, что параллельный расчет дает правильный результат
    long long sequential_result = 1;
    for (int i = 1; i <= k; i++) {
        sequential_result = (sequential_result * i) % mod;
    }
    printf("Проверка (последовательно): %d! mod %d = %lld\n", k, mod, sequential_result);
    
    // Сравниваем результаты параллельного и последовательного вычисления
    if (result == sequential_result) {
        printf("✓ Результаты совпадают!\n");
    } else {
        printf("✗ Ошибка: результаты не совпадают!\n");
    }
    
    return 0;  // Успешное завершение программы
}