#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Два мьютекса (замка) для демонстрации взаимной блокировки
pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

// Функция для потока 1
void* thread1_function(void* arg) {
    printf("ПОТОК 1: Начинаю работу\n");
    
    // Шаг 1: Захватываем мьютекс A
    printf("ПОТОК 1: Пытаюсь захватить мьютекс A\n");
    pthread_mutex_lock(&mutex_A);
    printf("ПОТОК 1: Захватил мьютекс A\n");
    
    // Имитируем некоторую работу с ресурсом A
    printf("ПОТОК 1: Работаю с ресурсом A...\n");
    sleep(2);  // Спим 2 секунды
    
    // Шаг 2: Пытаемся захватить мьютекс B
    printf("ПОТОК 1: Пытаюсь захватить мьютекс B\n");
    pthread_mutex_lock(&mutex_B);  // ЗДЕСЬ ПРОИСХОДИТ DEADLOCK!
    printf("ПОТОК 1: Захватил мьютекс B\n");
    
    // Критическая секция (никогда не выполнится из-за deadlock)
    printf("ПОТОК 1: Работаю с обоими ресурсами A и B\n");
    
    // Освобождение ресурсов (никогда не выполнится)
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    
    printf("ПОТОК 1: Завершил работу\n");
    return NULL;
}

// Функция для потока 2
void* thread2_function(void* arg) {
    printf("ПОТОК 2: Начинаю работу\n");
    
    // Шаг 1: Захватываем мьютекс B
    printf("ПОТОК 2: Пытаюсь захватить мьютекс B\n");
    pthread_mutex_lock(&mutex_B);
    printf("ПОТОК 2: Захватил мьютекс B\n");
    
    // Имитируем некоторую работу с ресурсом B
    printf("ПОТОК 2: Работаю с ресурсом B...\n");
    sleep(2);  // Спим 2 секунды
    
    // Шаг 2: Пытаемся захватить мьютекс A
    printf("ПОТОК 2: Пытаюсь захватить мьютекс A\n");
    pthread_mutex_lock(&mutex_A);  // ЗДЕСЬ ПРОИСХОДИТ DEADLOCK!
    printf("ПОТОК 2: Захватил мьютекс A\n");
    
    // Критическая секция (никогда не выполнится из-за deadlock)
    printf("ПОТОК 2: Работаю с обоими ресурсами A и B\n");
    
    // Освобождение ресурсов (никогда не выполнится)
    pthread_mutex_unlock(&mutex_A);
    pthread_mutex_unlock(&mutex_B);
    
    printf("ПОТОК 2: Завершил работу\n");
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("========================================\n");
    printf("ДЕМОНСТРАЦИЯ DEADLOCK (ВЗАИМНОЙ БЛОКИРОВКИ)\n");
    printf("========================================\n\n");
    
    // Создаем первый поток
    printf("ГЛАВНЫЙ ПОТОК: Создаю поток 1\n");
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        printf("❌ Ошибка создания потока 1\n");
        return 1;
    }
    
    // Создаем второй поток
    printf("ГЛАВНЫЙ ПОТОК: Создаю поток 2\n");
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        printf("Ошибка создания потока 2\n");
        return 1;
    }
    
    // Даем потокам время на выполнение
    printf("\nГЛАВНЫЙ ПОТОК: Жду 10 секунд...\n");
    sleep(10);
    
    // Проверяем, завершились ли потоки
    printf("\nГЛАВНЫЙ ПОТОК: Проверяю состояние потоков...\n");
    
    // Пытаемся присоединить потоки с таймаутом
    printf("ГЛАВНЫЙ ПОТОК: Программа не завершилась - ДЕДЛОК!\n");
    printf("ОБА ПОТОКА ЗАВИСЛИ В ВЕЧНОМ ОЖИДАНИИ!\n");
    
    printf("ПОТОК 1: держит мьютекс A, ждет мьютекс B\n");
    printf("ПОТОК 2: держит мьютекс B, ждет мьютекс A\n");
    printf("РЕЗУЛЬТАТ: вечная блокировка!\n");
    
    return 0;
}