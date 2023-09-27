#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXINPUT 1024

/*
Se maneja recibir un input y el que hacer si es un input vacio
Se manejan los parse de las pipes y los comandos dentro de las pipes
Se maneja la salida con exit y bloquea el uso de ctrl+c

*/

void ctrl_c(){
    printf("Si quieres salir, debes escribir exit en el prompt, presiona enter para continuar :)\n");
}

void parse_input(char *command, char **args, size_t max_tokens) {
    char *token;
    int i = 0;

    token = strtok(command, " \t\n"); // Dividir por espacios, tabuladores y saltos de línea
    while (token != NULL && i < max_tokens) {
        args[i] = token;
        token = strtok(NULL, " \t\n");
        i++;
    }

    args[i] = NULL; // Marcar el final del arreglo
}

void parse_pipes(char *input, char **piped_str, size_t max_tokens){
    char *token;
    int i = 0;

    token = strtok(input, "|");
    while (token != NULL && i < max_tokens) {
        piped_str[i] = token;
        token = strtok(NULL, "|");
        i++;
    }

    piped_str[i] = NULL; // Marcar el final del arreglo
}

int main(){
    size_t n = MAXINPUT;
    char *input = malloc(sizeof(char) * n);
    char **piped_str = malloc(sizeof(char*) * n+1);
    char **commands_str = malloc(sizeof(char*) * n+1);
    signal(SIGINT,ctrl_c);
    printf("Bienvenido \n");

    while(1){
        printf(">>>");
        getline(&input,&n,stdin);
        if(strlen(input) == 1) continue;
        printf("A continuar programando\n");
        //Manejar el parseo del input por pipes
        parse_pipes(input,piped_str,n+1);
        
        int i = 0;
        while(piped_str[i]!=NULL){

            printf("Pipe %d: %s\n",i+1,piped_str[i]);

            char *pipe_input = piped_str[i];
            char **args = malloc(sizeof(char*) * n+1);
            parse_input(pipe_input,args,n+1);
            if(strcmp(args[0],"exit")==0) exit(0);

            i++;
        }
    }
    free(input);
    free(piped_str);
    free(commands_str);
}