#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) запущен\n", getpid());
        sleep(2);
        printf("Дочерний процесс завершается\n");
        exit(0);  // Процесс завершился, но родитель не вызвал wait()
    } else {
        // Родительский процесс
        printf("Родительский процесс (PID: %d) создал ребенка: %d\n", 
               getpid(), pid);
        printf("Родитель спит 10 секунд и не вызывает wait()...\n");
        sleep(10);  // В этот период дочерний процесс становится зомби
        
        // Теперь вызываем wait()
        int status;
        wait(&status);
        printf("Родитель вызвал wait(), зомби убран\n");
        
        sleep(10);  // Дополнительное время для наблюдения
    }
    
    return 0;
}