#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "factorial.h"  // Включаем нашу библиотеку

struct Server {
    char ip[255];
    int port;
};

struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

// УДАЛЕНО: функция MultModulo - теперь в библиотеке
// УДАЛЕНО: функция ConvertStringToUI64 - теперь в библиотеке

// Функция для потока, который подключается к серверу
void* ProcessServer(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    
    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
        data->result = 0;
        pthread_exit(NULL);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(data->server.port);
    server.sin_addr.s_addr = *((unsigned long*)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        data->result = 0;
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", 
                data->server.ip, data->server.port);
        close(sck);
        data->result = 0;
        pthread_exit(NULL);
    }

    // Подготавливаем данные для отправки
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    // Отправляем задание серверу
    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", 
                data->server.ip, data->server.port);
        close(sck);
        data->result = 0;
        pthread_exit(NULL);
    }

    // Получаем результат от сервера
    char response[sizeof(uint64_t)];
    ssize_t bytes_received = recv(sck, response, sizeof(response), 0);
    if (bytes_received < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", 
                data->server.ip, data->server.port);
        close(sck);
        data->result = 0;
        pthread_exit(NULL);
    }

    if (bytes_received != sizeof(uint64_t)) {
        fprintf(stderr, "Invalid response size from %s:%d\n", 
                data->server.ip, data->server.port);
        close(sck);
        data->result = 0;
        pthread_exit(NULL);
    }

    memcpy(&data->result, response, sizeof(uint64_t));
    close(sck);
    
    printf("Server %s:%d returned result: %llu for range [%llu, %llu]\n",
           data->server.ip, data->server.port, 
           (unsigned long long)data->result,
           (unsigned long long)data->begin,
           (unsigned long long)data->end);
    
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                // Используем функцию из библиотеки
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid value for k: %s\n", optarg);
                    return 1;
                }
                break;
            case 1:
                // Используем функцию из библиотеки
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid value for mod: %s\n", optarg);
                    return 1;
                }
                break;
            case 2:
                strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;
        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == -1 || mod == -1 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    // Читаем список серверов из файла
    FILE* file = fopen(servers_file, "r");
    if (!file) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
        return 1;
    }

    // Считаем количество серверов
    int servers_num = 0;
    char line[255];
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) > 1) servers_num++;
    }
    
    if (servers_num == 0) {
        fprintf(stderr, "No servers found in file\n");
        fclose(file);
        return 1;
    }

    // Выделяем память и читаем серверы
    struct Server* servers = malloc(sizeof(struct Server) * servers_num);
    rewind(file);
    
    int idx = 0;
    while (fgets(line, sizeof(line), file) && idx < servers_num) {
        if (strlen(line) <= 1) continue;
        
        line[strcspn(line, "\n")] = 0;
        
        char* colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid server format: %s. Expected ip:port\n", line);
            free(servers);
            fclose(file);
            return 1;
        }
        
        *colon = '\0';
        strncpy(servers[idx].ip, line, sizeof(servers[idx].ip) - 1);
        servers[idx].port = atoi(colon + 1);
        
        if (servers[idx].port <= 0) {
            fprintf(stderr, "Invalid port for server: %s\n", line);
            free(servers);
            fclose(file);
            return 1;
        }
        
        idx++;
    }
    fclose(file);

    // Разделяем работу между серверами
    struct ThreadData* thread_data = malloc(sizeof(struct ThreadData) * servers_num);
    pthread_t* threads = malloc(sizeof(pthread_t) * servers_num);
    
    uint64_t numbers_per_server = k / servers_num;
    uint64_t remainder = k % servers_num;
    
    uint64_t current_start = 1;
    for (int i = 0; i < servers_num; i++) {
        thread_data[i].server = servers[i];
        thread_data[i].begin = current_start;
        thread_data[i].end = current_start + numbers_per_server - 1;
        
        if (remainder > 0) {
            thread_data[i].end++;
            remainder--;
        }
        
        thread_data[i].mod = mod;
        thread_data[i].result = 0;
        
        if (thread_data[i].end > k) thread_data[i].end = k;
        
        printf("Server %d (%s:%d) will process [%llu, %llu]\n", 
               i, servers[i].ip, servers[i].port,
               (unsigned long long)thread_data[i].begin,
               (unsigned long long)thread_data[i].end);
        
        current_start = thread_data[i].end + 1;
        
        if (pthread_create(&threads[i], NULL, ProcessServer, &thread_data[i])) {
            fprintf(stderr, "Error creating thread for server %d\n", i);
        }
    }

    // Ждем завершения всех потоков
    uint64_t total_result = 1;
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        if (thread_data[i].result != 0) {
            // Используем функцию из библиотеки
            total_result = MultModulo(total_result, thread_data[i].result, mod);
        }
    }

    printf("\n==============================\n");
    printf("Final result: %llu! mod %llu = %llu\n", 
           (unsigned long long)k,
           (unsigned long long)mod,
           (unsigned long long)total_result);
    printf("==============================\n");

    free(threads);
    free(thread_data);
    free(servers);

    return 0;
}