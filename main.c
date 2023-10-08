#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAXINPUT 1024

/*
Se maneja recibir un input y el que hacer si es un input vacio.
Se manejan los parse de las pipes y los comandos dentro de las pipes sienmpre que solo los separe un espacio.
Se maneja la salida con exit y bloquea el uso de ctrl+c.
Se maneja la ejecucion de comandos, hace falta testear, el comando de ejemplo que dio la profe funciona.
Se maneja la creacion de un daemon, no sabemos como restringir la ejecucion de otro comando de daemon mientras el anterior se sigue ejecutando
asi que de momento solo se puede uno por cada instancia de la consola, se podria arreglar con la implementacion de una señal.
*/

void ctrl_c(){ //Se puede ingresar exit despues de esto, pero la instruccion es para que se vea el prompt
    printf("Si quieres salir, debes escribir exit en el prompt, presiona enter para continuar :)\n");
}


//Funciones parser
void parse_input(char *command, char **args, size_t max_tokens) {
    char *token;
    int i = 0;

    token = strtok(command, " ");//Dividir por espacios
    while (token != NULL && i < max_tokens) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    args[i] = NULL; //Marcar el final del arreglo
}

void parse_pipes(char *input, char **piped_str, size_t max_tokens){
    char *token;
    int i = 0;

    token = strtok(input, "|"); //Dividir por |
    while (token != NULL && i < max_tokens) {
        piped_str[i] = token;
        token = strtok(NULL, "|");
        i++;
    }

    piped_str[i] = NULL;
}

//Funcion que maneja el daemon
void daemon_exe(int t, int p){
    //Codigo tipico de crear un fork y revisar, pero esta vez el proceso padre sale de la funcion y el hijo ejecuta el resto del codigo
    pid_t pid,sid;

    pid = fork();

    if(pid < 0){
        perror("fork");
        exit(-1);
    }else if(pid > 0){
        exit(0);
    }

    //Se cambia el umask y se crea una sesion aparte para el daemon
    umask(0);

    sid = setsid();

    if(sid < 0){
        perror("setsid");
        exit(-1);
    }

    //Se prepara el ambiente para empezar a hacer registros en el log del sistema
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog("nuevo_daemon", LOG_PID, LOG_DAEMON);

    time_t start_time = time(NULL);
    time_t end_time = start_time + p;

    while(time(NULL) < end_time){
        //Se abre /proc/stat para sacar la info a añadir en el log del sistema
        FILE *stat_file = fopen("/proc/stat", "r");
        if (stat_file == NULL) {
            perror("No se pudo abrir /proc/stat");
            exit(-1);
        }

        //Se procesa la informacion para despues ser añadida al log del sistema
        char line[MAXINPUT];

        double processes = 0, procs_running = 0, procs_blocked = 0;
        while (fgets(line, sizeof(line), stat_file)) {
            if (strstr(line, "processes ")) {
                sscanf(line, "processes %lf", &processes);
            } else if (strstr(line, "procs_running ")) {
                sscanf(line, "procs_running %lf", &procs_running);
            } else if (strstr(line, "procs_blocked ")) {
                sscanf(line, "procs_blocked %lf", &procs_blocked);
            }
        }

        char log[MAXINPUT];
        snprintf(log, sizeof(log), "processes: %lf, procs_running: %lf, procs_blocked: %lf", processes, procs_running, procs_blocked);
        syslog(LOG_INFO, "%s",log);

        //Espera t segundos antes de la próxima medición
        sleep(t);
    }

    //Cierra el log y sale del proceso hijo
    closelog();

    exit(0);

}

int main(){
    size_t n = MAXINPUT;
    char *input = malloc(sizeof(char) * n);
    char **piped_str = malloc(sizeof(char*) * n+1);
    system("clear");
    signal(SIGINT,ctrl_c);
    printf("Bienvenido  \n");
    int daemon_run = 0;

    while(1){
        printf("%d@>>>",getpid()); //Se imprime el pid de la shell custom como parte del prompt, probar a ejecutar a.out varias veces dentro de esta
        getline(&input,&n,stdin);
        input = strtok(input,"\n");
        //Manejar el parseo del input por cada pipe
        parse_pipes(input,piped_str,n+1);
        
        int i = 0;
        int input_file = 0;
        while(piped_str[i]!=NULL){

            //printf("Pipe %d: %s\n",i+1,piped_str[i]);

            char *pipe_input = piped_str[i];
            char **args = malloc(sizeof(char*) * n+1);
            parse_input(pipe_input,args,n+1);

            if(args[0] == NULL) {
                //El usuario ingresó una línea vacía o solo espacios en blanco.
                free(args);
                i++;
                continue;
            }
            
            if(strcmp(args[0],"exit")==0){//Si el usuario usa el comando exit
                free(args);
                free(input);
                free(piped_str);
                if(daemon_run){
                    //Si el daemon está en ejecución, mata al daemon antes de salir
                    kill(daemon_run, SIGTERM);
                }
                exit(0);
            }else if(strcmp(args[0],"daemon")==0){//Si el usuario usa el comando daemon
                int t,p;
                if(args[1]==NULL || args[2]==NULL){
                    printf("Faltan argumentos para la ejecucion del daemon\n");
                    free(args);
                    i++;
                    continue;
                }else{
                    t = atoi(args[1]);
                    p = atoi(args[2]);
                } 
                if(t < 1 || p < t){
                    printf("Los argumentos para el daemon no son validos\n");
                    free(args);
                    i++;
                    continue;
                }
                if(daemon_run > 0){
                    printf("El daemon ya esta en ejecucion\n");
                }else{
                    daemon_run = fork();
                    if(daemon_run == 0){
                        daemon_exe(t,p);
                        exit(0);
                    }
                }
                
                free(args);
                i++;
                continue;
            }
            int pipe_file[2];
            pipe(pipe_file); //Crear una tubería

            pid_t child_pid = fork();

            if (child_pid == 0) {
                //Proceso hijo
                close(pipe_file[0]); //Cerrar el extremo de lectura de la tubería
                
                if (i > 0) {
                    //Redirigir la entrada estándar desde la tubería anterior
                    dup2(input_file, 0);
                    close(input_file);
                }

                if (piped_str[i+1] != NULL) {
                    //Redirigir la salida estándar hacia la tubería actual (si no es el último comando)
                    dup2(pipe_file[1], 1);
                    close(pipe_file[1]);
                }

                //Ejecutar el comando
                execvp(args[0], args);

                //Esto solo se ejecuta sihubo un error en execvp
                perror("execvp");
                exit(-1);
            } else if (child_pid > 0) {//Estamos en proceso padre
                close(pipe_file[1]); //Cerrar el extremo de escritura de la tubería
                
                if (input_file != 0) {
                    close(input_file); //Cerrar la entrada anterior (no necesaria)
                }

                input_file = pipe_file[0]; //Guardar el descriptor de archivo de entrada para la próxima tubería
            } else {
                perror("fork");
                exit(-1);
            }

            /* 
            for (int j = 0; args[j] != NULL; j++) {
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