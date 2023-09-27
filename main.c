#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXINPUT 1024

/*
Se maneja recibir un input y el que hacer si es un input vacio
Se manejan los parse de las pipes y los comandos dentro de las pipes sienmpre que solo los separe un espacio
Se maneja la salida con exit y bloquea el uso de ctrl+c
Se maneja la ejecucion de comandos, hace falta testear, el comando de ejemplo que dio la profe funciona

*/

void ctrl_c(){ // Se puede ingresar exit en despues de esto, pero la instruccion es para que se vea el prompt
    printf("Si quieres salir, debes escribir exit en el prompt, presiona enter para continuar :)\n");
}

void parse_input(char *command, char **args, size_t max_tokens) {
    char *token;
    int i = 0;

    token = strtok(command, " ");// Dividir por espacios
    while (token != NULL && i < max_tokens) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    args[i] = NULL; // Marcar el final del arreglo
}

void parse_pipes(char *input, char **piped_str, size_t max_tokens){
    char *token;
    int i = 0;

    token = strtok(input, "|"); // Dividir por |
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
    signal(SIGINT,ctrl_c);
    printf("Bienvenido  \n");

    while(1){
        printf("%d@>>>",getpid()); // Se imprime el pid de la shell custom como parte del prompt
        getline(&input,&n,stdin);
        input = strtok(input,"\n");
        //Manejar el parseo del input por pipes
        parse_pipes(input,piped_str,n+1);
        
        int i = 0;
        int input_file = 0;
        while(piped_str[i]!=NULL){

            //printf("Pipe %d: %s\n",i+1,piped_str[i]);

            char *pipe_input = piped_str[i];
            char **args = malloc(sizeof(char*) * n+1);
            parse_input(pipe_input,args,n+1);

            if(args[0] == NULL) {
                // El usuario ingresó una línea vacía o solo espacios en blanco.
                free(args);
                i++;
                continue;
            }
            
            if(strcmp(args[0],"exit")==0){
                free(args);
                free(input);
                free(piped_str);
                exit(0);
            } 
            int pipe_file[2];
            pipe(pipe_file); // Crear una tubería

            pid_t child_pid = fork();

            if (child_pid == 0) {
                // Proceso hijo
                close(pipe_file[0]); // Cerrar el extremo de lectura de la tubería
                
                if (i > 0) {
                    // Redirigir la entrada estándar desde la tubería anterior
                    dup2(input_file, 0);
                    close(input_file);
                }

                if (piped_str[i+1] != NULL) {
                    // Redirigir la salida estándar hacia la tubería actual (si no es el último comando)
                    dup2(pipe_file[1], 1);
                    close(pipe_file[1]);
                }

                // Ejecutar el comando
                execvp(args[0], args);

                // Si llegamos aquí, hubo un error en execvp
                perror("execvp");
                exit(1);
            } else if (child_pid > 0) {
                // Proceso padre
                close(pipe_file[1]); // Cerrar el extremo de escritura de la tubería
                
                if (input_file != 0) {
                    close(input_file); // Cerrar la entrada anterior (no necesaria)
                }

                input_file = pipe_file[0]; // Guardar el descriptor de archivo de entrada para la próxima tubería
            } else {
                perror("fork");
                exit(1);
            }
            
            /*for (int j = 0; args[j] != NULL; j++) {
                printf("Argumento %d: %s\n", j, args[j]);
            }*/

            i++;
            free(args);
        }
        while (wait(NULL) != -1);
    }

    //Un poco redundante liberar memoria aca si al ejecutar exit se libera
    free(input);
    free(piped_str);
}