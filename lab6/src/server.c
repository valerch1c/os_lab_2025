#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "factorial.h"  // Включаем нашу библиотеку

// УДАЛЕНО: структура FactorialArgs - теперь в factorial.h
// УДАЛЕНО: функция MultModulo - теперь в библиотеке

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t* result = malloc(sizeof(uint64_t));
    if (result == NULL) {
        pthread_exit(NULL);
    }
    // Используем функцию из библиотеки
    *result = Factorial(fargs);
    pthread_exit(result);
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (true) {
        int current_optind = optind ? optind : 1;
        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                port = atoi(optarg);
                if (port <= 0) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return 1;
                }
                break;
            case 1:
                tnum = atoi(optarg);
                if (tnum <= 0) {
                    fprintf(stderr, "Invalid thread number: %s\n", optarg);
                    return 1;
                }
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;
        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Can not create server socket!");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Can not bind to socket!");
        close(server_fd);
        return 1;
    }

    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        close(server_fd);
        return 1;
    }

    printf("Server listening at %d with %d threads\n", port, tnum);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Could not establish new connection\n");
            continue;
        }

        unsigned int buffer_size = sizeof(uint64_t) * 3;
        char from_client[buffer_size];
        int read_bytes = recv(client_fd, from_client, buffer_size, 0);

        if (read_bytes == 0) {
            close(client_fd);
            continue;
        }
        
        if (read_bytes < 0) {
            fprintf(stderr, "Client read failed\n");
            close(client_fd);
            continue;
        }
        
        if (read_bytes < buffer_size) {
            fprintf(stderr, "Client send wrong data format\n");
            close(client_fd);
            continue;
        }

        uint64_t begin = 0;
        uint64_t end = 0;
        uint64_t mod = 0;
        memcpy(&begin, from_client, sizeof(uint64_t));
        memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
        memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

        printf("Receive task: begin=%llu, end=%llu, mod=%llu\n", 
               (unsigned long long)begin, 
               (unsigned long long)end, 
               (unsigned long long)mod);

        // Создаем аргументы для потоков
        struct FactorialArgs args[tnum];
        pthread_t threads[tnum];
        
        uint64_t range = end - begin + 1;
        uint64_t numbers_per_thread = range / tnum;
        uint64_t remainder = range % tnum;
        
        uint64_t current_start = begin;
        
        for (int i = 0; i < tnum; i++) {
            args[i].begin = current_start;
            args[i].end = current_start + numbers_per_thread - 1;
            
            if (remainder > 0) {
                args[i].end++;
                remainder--;
            }
            
            if (args[i].end > end) args[i].end = end;
            
            args[i].mod = mod;
            
            printf("Thread %d: [%llu, %llu]\n", i,
                   (unsigned long long)args[i].begin,
                   (unsigned long long)args[i].end);
            
            current_start = args[i].end + 1;
            
            if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i])) {
                fprintf(stderr, "Error: pthread_create failed!\n");
                close(client_fd);
                continue;
            }
        }

        // Собираем результаты от потоков
        uint64_t total = 1;
        for (int i = 0; i < tnum; i++) {
            uint64_t *thread_result = NULL;
            pthread_join(threads[i], (void**)&thread_result);
            if (thread_result != NULL) {
                // Используем функцию из библиотеки
                total = MultModulo(total, *thread_result, mod);
                free(thread_result);
            }
        }

        printf("Calculated result: %llu\n", (unsigned long long)total);

        // Отправляем результат клиенту
        char buffer[sizeof(total)];
        memcpy(buffer, &total, sizeof(total));
        err = send(client_fd, buffer, sizeof(total), 0);
        if (err < 0) {
            fprintf(stderr, "Can't send data to client\n");
        }

        close(client_fd);
        printf("Connection closed\n");
    }

    close(server_fd);
    return 0;
}