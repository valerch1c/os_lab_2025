#define _POSIX_C_SOURCE 199309L 

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <signal.h>
#include <getopt.h>
#include <errno.h> 

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработки сигналов
volatile sig_atomic_t timeout_occurred = 0;
pid_t *child_pids = NULL;

// Обработчик сигнала ALARM
void timeout_handler(int sig) {
    timeout_occurred = 1;
}

// Функция для отправки SIGKILL всем дочерним процессам
void kill_all_children(int pnum) {
    if (child_pids != NULL) {
        for (int i = 0; i < pnum; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGKILL);
                printf("Sent SIGKILL to child process %d\n", child_pids[i]);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout_sec = 0; // 0 означает отсутствие таймаута
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            timeout_sec = atoi(optarg);
            if (timeout_sec < 0) {
                printf("timeout must be a non-negative number\n");
                return 1;
            }
            break;
          case 4:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"sec\"]\n",
           argv[0]);
    return 1;
  }

  // Инициализация массива для хранения PID дочерних процессов
  child_pids = malloc(pnum * sizeof(pid_t));
  for (int i = 0; i < pnum; i++) {
      child_pids[i] = 0;
  }

  // Настройка обработчика сигнала ALARM, если задан таймаут
  if (timeout_sec > 0) {
      struct sigaction sa;
      sa.sa_handler = timeout_handler;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = 0;
      
      if (sigaction(SIGALRM, &sa, NULL) == -1) {
          perror("sigaction");
          return 1;
      }
      
      printf("Timeout set to %d seconds\n", timeout_sec);
      alarm(timeout_sec); // Устанавливаем будильник
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  // Создаем pipes для каждого процесса (если не используем файлы)
  int **pipes = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int*));
    for (int i = 0; i < pnum; i++) {
      pipes[i] = malloc(2 * sizeof(int));
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }
  }

  int active_child_processes = 0;
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // Дочерний процесс
        int start_idx = i * (array_size / pnum);
        int end_idx = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
        
        struct MinMax local_min_max = GetMinMax(array, start_idx, end_idx);
        
        if (with_files) {
          // Используем файлы для передачи данных
          char filename[20];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // Используем pipes для передачи данных
          close(pipes[i][0]); // Закрываем чтение в дочернем процессе
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          close(pipes[i][1]);
        }
        free(array);
        if (!with_files) {
          for (int j = 0; j < pnum; j++) {
            free(pipes[j]);
          }
          free(pipes);
        }
        free(child_pids);
        exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родительский процесс ждет завершения всех дочерних с таймаутом
  int completed_processes = 0;
  while (active_child_processes > 0 && !timeout_occurred) {
    int status;
    pid_t finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      // Один из дочерних процессов завершился
      active_child_processes -= 1;
      completed_processes += 1;
      
      // Находим и обнуляем PID завершенного процесса
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == finished_pid) {
          child_pids[i] = 0;
          break;
        }
      }
      
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally with status %d\n", 
               finished_pid, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d killed by signal %d\n", 
               finished_pid, WTERMSIG(status));
      }
    } else if (finished_pid == 0) {
      // Нет завершенных процессов, делаем паузу
      sleep(100000); // 100ms
    } else {
      // Ошибка waitpid
      perror("waitpid");
      break;
    }
  }

  // Если сработал таймаут, убиваем все дочерние процессы
  if (timeout_occurred) {
    printf("\nTimeout occurred! Sending SIGKILL to all child processes...\n");
    kill_all_children(pnum);
    
    // Ждем завершения убитых процессов
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  // Собираем результаты от завершившихся процессов
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  int successful_processes = 0;
  for (int i = 0; i < pnum; i++) {
    // Пропускаем процессы, которые не завершились нормально
    if (child_pids[i] != 0 && timeout_occurred) {
      continue;
    }
    
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Читаем из файлов
      char filename[20];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        if (fscanf(file, "%d %d", &min, &max) == 2) {
          successful_processes++;
          if (min < min_max.min) min_max.min = min;
          if (max > min_max.max) min_max.max = max;
        }
        fclose(file);
        remove(filename); // Удаляем временный файл
      }
    } else {
      // Читаем из pipes
      if (pipes[i] != NULL) {
        close(pipes[i][1]); // Закрываем запись в родительском процессе
        if (read(pipes[i][0], &min, sizeof(int)) == sizeof(int) &&
            read(pipes[i][0], &max, sizeof(int)) == sizeof(int)) {
          successful_processes++;
          if (min < min_max.min) min_max.min = min;
          if (max > min_max.max) min_max.max = max;
        }
        close(pipes[i][0]);
      }
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Освобождаем ресурсы
  free(array);
  free(child_pids);
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipes[i] != NULL) {
        free(pipes[i]);
      }
    }
    free(pipes);
  }

  // Выводим результаты
  printf("\n=== RESULTS ===\n");
  if (timeout_occurred) {
    printf("Execution terminated by timeout after %d seconds\n", timeout_sec);
    printf("Successfully processed by %d out of %d processes\n", 
           successful_processes, pnum);
  } else {
    printf("All processes completed successfully\n");
  }
  
  if (successful_processes > 0) {
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
  } else {
    printf("No results collected - all processes failed or were terminated\n");
  }
  
  printf("Elapsed time: %fms\n", elapsed_time);
  printf("Seed: %d\n", seed);
  printf("Array size: %d\n", array_size);
  printf("Processes: %d\n", pnum);
  printf("Synchronization: %s\n", with_files ? "files" : "pipes");
  printf("Timeout: %s\n", timeout_sec > 0 ? "enabled" : "disabled");
  
  fflush(NULL);
  return (successful_processes > 0) ? 0 : 1;
}