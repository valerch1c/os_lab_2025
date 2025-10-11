#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Аргументы для sequential_min_max
    char *seq_args[] = {"./sequential_min_max", "123", "10", NULL};
    
    printf("Родительский процесс: PID=%d\n", getpid());
    printf("Запускаю sequential_min_max...\n\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // ДОЧЕРНИЙ ПРОЦЕСС
        printf("Дочерний процесс: PID=%d\n", getpid());
        printf("Запускаю: %s %s %s\n\n", seq_args[0], seq_args[1], seq_args[2]);
        
        // Заменяем текущую программу на sequential_min_max
        execvp(seq_args[0], seq_args);
        
        // Если сюда попали - ошибка!
        perror("execvp failed");
        exit(1);
        
    } else if (pid > 0) {
        // РОДИТЕЛЬСКИЙ ПРОЦЕСС
        int status;
        wait(&status);  // Ждем завершения дочернего процесса
        
        printf("\nРодительский процесс: дочерний завершился\n");
        
        if (WIFEXITED(status)) {
            printf("Код возврата: %d\n", WEXITSTATUS(status));
        }
        
    } else {
        // ОШИБКА fork
        perror("fork failed");
        return 1;
    }
    
    return 0;
}