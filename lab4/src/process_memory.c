#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHW_ADR(ID, I) (printf("ID %s \t is at virtual address: %8p\n", ID, (void*)&I))

void showit(char *);

int main() {
    char *cptr = "Hello World\n";
    char buffer1[25];
    int i = 0;
    
    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);
    
    strcpy(buffer1, "Write to buffer1\n");
    showit(buffer1);
    
    return 0;
}

void showit(char *p) {
    char *buffer2;
    
    SHW_ADR("buffer2", buffer2);
    
    if ((buffer2 = (char *)malloc((unsigned)strlen(p) + 1)) != NULL) {
        printf("Alocated memory at %p\n", (void*)buffer2);
        strcpy(buffer2, p);
        printf("%s", buffer2);
        free(buffer2);
    } else {
        printf("Allocation error\n");
        exit(1);
    }
}