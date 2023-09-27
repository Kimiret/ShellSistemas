#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXINPUT 1024

int main(){
    size_t n = MAXINPUT;
    char *input = malloc(sizeof(char) * n);
    while(1){
        printf(">>>");
        getline(&input,&n,stdin);
        if(strlen(input) == 1) continue;
        printf("A continuar programando\n");
        //Manejar el parseo del input
    }
}