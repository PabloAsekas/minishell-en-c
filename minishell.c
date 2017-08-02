#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"

/*
*
* Pablo Bermejo y Blanca Romero
*
* https://github.com/PabloAsekas
*
*/

int main(void) {

	char buf[1024];
	tline * line;
	int i, j, k;
	int **pipes; //Matriz de pipes
	int pid;
	int *pidHijos;
	int fichero;

	printf("msh > ");

	while (fgets(buf, 1024, stdin)){

		line = tokenize(buf);
		if(line == NULL){
			continue;
		}

		/*------------------------------COMANDO CD------------------------------*/

		if (strcmp(line->commands[0].argv[0],"cd") == 0) {//si el comando es cd

			if (line->commands[0].argv[1] == NULL) { //y no tiene argumentos
				chdir(getenv("HOME")); //nos lleva a HOME
			}

			else {
				int error = chdir(line->commands[0].argv[1]); //si tiene argumentos nos lleva al destino solicitado

				if (error == -1) { //a menos que no exista
					printf("Lo siento pero el directorio actual no existe: %s \n", line->commands[0].argv[1]);
				}
			}
		}

		/*------------------------------COMANDO CD------------------------------*/

		/*------------------------------1 COMANDO------------------------------*/

		else if(line->ncommands == 1){ //Si hay un solo comando
			
			pid = fork();

			if(pid < 0){
				fprintf(stderr, "Fallo el fork() %s/n" , strerror(errno));
				exit(-1);
			} else if(pid == 0){ //si es el hijo
				if(line->redirect_input != NULL){ //si hay redirección de entrada
						int fichero;
						printf("redirección de entrada: %s\n", line->redirect_input);
						fichero = open(line->redirect_input, O_RDONLY);
						if (fichero == -1) {
							fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", fichero);
							exit(1);
						} else {
							dup2(fichero,0); //redirijimos la entrada
						}
					if (line->redirect_output != NULL) { //si hay redirección de salida
						int fichero2;
						fichero2 = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600);
						if (fichero2 == -1) {
							fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero2);
							exit(1);
						} else {
							dup2(fichero2,1); //redirigimos la salida
						}
						execv(line->commands[0].filename, line->commands[0].argv); //ejecutamos el comando
						fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
					}else if (line->redirect_error != NULL) { //si hay redirección de error
						int fichero2;
						fichero2 = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600);
						if (fichero2 == -1){
							fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de error\n", fichero2);
							exit(1);
						}
						dup2(fichero2,2); //redirigimos la salida de error
						execv(line->commands[0].filename, line->commands[0].argv); //ejecutamos el comando
						fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
					} else { //si solo hay redirección de entrada
						execv(line->commands[0].filename, line->commands[0].argv); //ehecutamos el comando
						fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
					}
				} else if (line->redirect_output != NULL) { //si solo hay redirección de salida
					int fichero;
					fichero = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600);
					if (fichero == -1) {
						fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero);
						exit(1);
					} else {
						dup2(fichero,1); //redirigimos la salida
					}
					execv(line->commands[0].filename, line->commands[0].argv); //ejecutamos el comando
					fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
				} else if (line->redirect_error != NULL) { //si solo hay redirección de error
					int fichero;
					fichero = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600);
					if (fichero == -1) {
						fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", fichero);
						exit(1);
					} else {
						dup2(fichero,2); //redirigimos la salida de error
					}
					execv(line->commands[0].filename, line->commands[0].argv); //ejecutamos el comando
					fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
				} else { //si no hay redirecciones
					execv(line->commands[0].filename, line->commands[0].argv); //ejecutamos el comando
					fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
				}

			}else{ //el padre
				waitpid(pid, NULL, 0); //espera por su hijo
			}
		}

		/*------------------------------1 COMANDO------------------------------*/


		/*------------------------------2 O MAS COMANDOS-----------------------*/
		if(line->ncommands >= 2){ 
			pidHijos = malloc(line->ncommands * sizeof(int)); 
			pipes = (int **) malloc ((line->ncommands-1) * sizeof(int *)); //Reservamos memoria para la matriz de pipes

			for(i=0; i<line->ncommands-1; i++){ //Creo tantos pipes como comandos-1
					pipes[i] = (int *) malloc (2*sizeof(int)); 
					if(pipe(pipes[i]) < 0)
						fprintf(stderr, "Fallo al crear el pipe %s/n" , strerror(errno));
			}
			for(i=0; i < line->ncommands; i++){ //Por cada comando hago un HIJO	
				pid = fork();

				if(pid < 0){
					fprintf(stderr, "Fallo el fork() %s\n" , strerror(errno));
					exit(-1);
				} else if(pid == 0){ //Soy el HIJO
					if(i == 0){ //Si soy el primer comando
						if(line->redirect_input != NULL){ //Si hay redirección de entrada
							fichero = open(line->redirect_input, O_RDONLY);
							if (fichero == -1) {
								fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", fichero);
								exit(1);
							} else {
								dup2(fichero,0); //Redirijimos la entrada
							}
						}
						for(j=1; j<line->ncommands-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
							close(pipes[j][1]);
							close(pipes[j][0]);
						}
						close(pipes[0][0]);
						dup2(pipes[0][1],1);
					}
					if(i>0 && i<line->ncommands-1){ //Si soy un comando intermedio	
						if(i==1 && line->ncommands != 3){ //Si soy el segundo comando
							for(j=i+1; j<line->ncommands-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
								close(pipes[j][1]);
								close(pipes[j][0]);
							}						
						}
						if(i==line->ncommands-2 && line->ncommands != 3){ //Si soy el penultimo comando
							for(j=0; j<i-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
								close(pipes[j][1]);
								close(pipes[j][0]);	
							}
						}
						if(i!=1 && i!=line->ncommands-2 && line->ncommands != 3){ //Si no soy ni el segundo ni el penultimo
							for(j=0; j<i-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
								close(pipes[j][1]);
								close(pipes[j][0]);
							}
							for(j=i+1; j<line->ncommands-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
								close(pipes[j][1]);
								close(pipes[j][0]);
							}
						}
						close(pipes[i-1][1]);
						dup2(pipes[i-1][0],0);
						close(pipes[i][0]);
						dup2(pipes[i][1],1);
					}
					if(i == line->ncommands-1){ //Si soy el ultimo comando
						if (line->redirect_output != NULL) { //Si hay redirección de salida
							fichero = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC , 0600);
							if (fichero == -1) {
								fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero);
								exit(1);
							} else {
								dup2(fichero,1); //Redirigimos la salida
							}
						}
						if (line->redirect_error != NULL) { //Si hay redirección de error
							fichero = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC , 0600);
							if (fichero == -1) {
								fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de error\n", fichero);
								exit(1);
							} else {
								dup2(fichero,2); //Redirigimos la salida de error
							}
						}
						for(j=0; j<line->ncommands-2; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0]
							close(pipes[j][1]);
							close(pipes[j][0]);
						}
						close(pipes[i-1][1]);
						dup2(pipes[i-1][0],0);
					}
					execv(line->commands[i].filename, line->commands[i].argv); //Ejecutamos el comando
					fprintf(stderr,"%s: No se encuentra el mandato\n",line->commands[i].filename);
				} else { //Soy el PADRE
					pidHijos[i] = pid; //Guardamos el PID del HIJO
				}
			}
			for(k=0; k<line->ncommands-1; k++){ //Cerramos todos los pipes
				close(pipes[k][1]);
				close(pipes[k][0]);
			}
			for(k=0; k<line->ncommands; k++){ 
				waitpid(pidHijos[k],NULL,0);
			}
			for(i=0; i<line->ncommands-1; i++){ //Liberamos memoria
				free(pipes[i]);
			}
			free(pipes);
			free(pidHijos);
		}
		/*------------------------------2 O MAS COMANDOS-----------------------*/
		
		printf("msh > ");			
	}
	return 0;
}