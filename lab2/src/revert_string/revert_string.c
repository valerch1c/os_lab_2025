#include <string.h>
#include "revert_string.h"

void RevertString(char *str)
{
    int len = strlen(str);
    char reversed[len + 1];
    
    for(int i = 0; i < len; i++)
    {
        reversed[i] = str[len - 1 - i];
    }
    reversed[len] = '\0';
    
    strcpy(str, reversed);
}
