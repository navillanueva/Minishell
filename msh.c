//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// ficheros por si hay redirección
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Saliendo del MSH **** \n");
	//signal(SIGINT, siginthandler);
        exit(0);
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first
    for(int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0; 
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];
    //Creamos la variable de entorno si no está creada
    if(getenv("Acc")==NULL){
    	if(setenv("Acc", "0", 1)<0){
    		perror("Error creating variable");
    		exit(-1);
    	}
    }
    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;


	while (1) 
	{
		int status = 0;
	        int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt 
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
                //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
                executed_cmd_lines++;
                if( end != 0 && executed_cmd_lines < end) {
                    command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
                }else if( end != 0 && executed_cmd_lines == end)
                    return 0;
                else
                    command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
                //************************************************************************************************
		if(command_counter>0&&command_counter<9){//Filtramos que haya entre 1 y 8 comandos

			if(command_counter==1){ //Casos en los que es un comando solamente
				if(strcmp(argvv[0][0],"mycalc")==0){ //Comprobamos si hemos recibido el mandato interno mycalc
					if(argvv[0][1]==NULL||argvv[0][2]==NULL||argvv[0][3]==NULL){
						printf("[ERROR] La estructura del comando es <operando 1> <add/mod> <operando 2>\n");
						continue;
					}
					int a=atoi(argvv[0][1]); //Extraemos los operandos
					int b=atoi(argvv[0][3]);
					if(strcmp(argvv[0][2],"add")!=0 && strcmp(argvv[0][2],"mod")!=0){ //Comprobamos que las operaciones
													  //son las indicadas
						printf("[ERROR] La estructura del comando es <operando 1> <add/mod> <operando 2>\n");
						continue;
					}
					if(strcmp(argvv[0][2],"add")==0){
						int result=a+b; //operamos
						int env=atoi(getenv("Acc")); //cargamos la variable de entorno
						env=env+result; //calculamos su nuevo valor
						char nEV[50];
						sprintf(nEV, "%d", env); //pasamos su valor a String
						setenv("Acc", nEV, 1); // Actualizamos su valor
						fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", a, b, result, env);//Imprimimos el resultado
						continue; //volvemos al bucle
					}
					if(strcmp(argvv[0][2],"mod")==0){
						int coc=a/b;//operamos
						int rem=a%b;
						char sim ='%';
						fprintf(stderr, "[OK] %d %c %d = %d * %d + %d\n", a, sim, b, b, coc, rem);//imprimimos el
															  //resultado
						continue;//volvemos al bucle
					}
				}
					
					
								
				if(strcmp(argvv[0][0], "mycp")==0){ //comprobamos si nos han pasado el mandato interno mycp
					if(argvv[0][1]==NULL || argvv[0][2]==NULL){ //comprobamos si tenemos dos ficheros en los argumentos
						printf("[ERROR] La estructura del comando es mycp <fichero origen> <fichero destino>\n");
						continue;
					}
					int a=open(argvv[0][1], O_RDONLY); //cargamos el descriptor del primer fichero
					if(a==-1){ //controlamos posibles errores de apertura, como que no exista el fichero origen
						printf("[ERROR] Error al abrir el fichero origen\n");
						continue;
					}
					int b=open(argvv[0][2], O_CREAT|O_WRONLY|O_TRUNC, 00700); //cargamos el descriptor del segundo
												  //fichero
					if(b==-1){ //controlamos posibles errores de apertura
						printf("[ERROR] Error al abrir el fichero destino\n");
						continue;
					}
					char buffer[1024]; //creamos un buffer para almacenar contenido
					int r; //contador de caracteres leídos
					while((r=read(a, buffer, 1024))>0){ //vamos escribiendo en el fichero de destino lo que
						write(b, buffer, r);	    //vamos leyendo del fichero orígen
					}
					close(a);//Cerramos descriptores
					close(b);
					printf("[OK] Copiado con exito el fichero %s a %s\n", argvv[0][1], argvv[0][2]);//Imprimimos el
					continue;									//resultado
					}
				else{ //En este caso, el comando se trata de cualquier otro que no es uno interno
					pid_t pid; //creamos un proceso hijo
					pid=fork();
					if(pid==-1){ //controlamos errores de creación del hijo
						perror ("Error en el fork");
						exit(-1);
					}
					if(pid==0){ //Ejecución del hijo
						if((strcmp(filev[0], "0")!=0)){ //Comprobamos si tiene o no redirección de entrada
							int in=open(filev[0], O_RDONLY);
							if(in==-1){
								perror ("Error al abrir el fichero");
								exit(-1);
							}
							close(STDIN_FILENO);
							dup(in);
							close(in);
						}
						if((strcmp(filev[1], "0")!=0)){ //Comprobamos si tiene o no redirección de salida
							int out=open(filev[1], O_CREAT|O_WRONLY|O_TRUNC, 00700);
							if(out==-1){
								perror ("Error al abrir el fichero");
								exit(-1);
							}
							close(STDOUT_FILENO);
							dup(out);
							close(out);
						}
						if((strcmp(filev[2], "0")!=0)){ //Comprobamos si tiene o no redirección de errores
							int err=open(filev[2], O_CREAT|O_WRONLY|O_TRUNC, 00700);
							if(err==-1){
								perror ("Error al abrir el fichero");
								exit(-1);
							}
							close(STDERR_FILENO);
							dup(err);
							close(err);
						}
						execvp(argvv[0][0], argvv[0]); //Ejecutamos el comando
						perror("Error en exec. Esto no debería ejecutarse");//Controlamos posibles errores de
												    //ejecución
					}
					else{
						if(in_background==0){ //Controlamos si debe ejecutarse o no en background
							while(wait(&status)!=pid); //Caso negativo, el padre espera a que termine el hijo
						}
						else{
							printf("[%d]\n",pid); //Caso afirmativo, imprimimos su pid y no espera
						}
					}
				}
			}
				
			else{ //Aquí implementamos las secuencias de más de un comando
				pid_t pid[command_counter]; //Creamos tantos pid como procesos tengan que ejecutarse
				int p[command_counter-1][2]; //Creamos tantos pipes como sean necesarios, es decir, uno menos que el
							     //número de procesos a ejecutar
				for(int i=0; i<command_counter-1; i++){
					pipe(p[i]);
				}
				for(int i=0;i<command_counter; i++){ //Comenzamos a crear y ejecutar procesos
					if(i>0) close(p[i-1][1]); //Aquí cerramos los accesos a escritura del padre para cada pipe según
								  //los hijos se vayan ejecutando y así mandar el End of File al siguiente
								  //hijo que vaya a coger datos de la tubería
					pid[i]=fork(); //Creamos el proceso hijo
					if(pid[i]<0){ //Controlamos los errores que puedan salir en la creación
						perror("Error en el fork");
						exit(-1);
					}
					if(pid[i]==0){ //proceso hijo
						if(i==0){ //Caso en el que es el primero proceso
							close(STDOUT_FILENO); //cerramos su descriptor de salida estándar
							dup(p[i][1]); 	     //y la cambiamos por la escritura en la tubería
							for(int j=0; j<command_counter-1; j++){ //Cerramos los descriptores que no se
								close(p[j][0]);			//van a usar
								close(p[j][1]);	
							}
							if((strcmp(filev[0], "0")!=0)){//Como sería el primer proceso, puede tener
										       //redirección de entrada, por tanto, evaluamos
										       //si la tiene o no
								int in=open(filev[0], O_RDONLY);
								if(in==-1){
									perror ("Error al abrir el fichero");
									exit(-1);
								}
								close(STDIN_FILENO);
								dup(in);
								close(in);
							}
							if((strcmp(filev[2], "0")!=0)){//Verificamos si tiene redirección de error o no
								int err=open(filev[2], O_CREAT|O_WRONLY|O_TRUNC, 00700);
								if(err==-1){
									perror ("Error al abrir el fichero");
									exit(-1);
								}
								close(STDERR_FILENO);
								dup(err);
								close(err);
							}
							execvp(argvv[i][0], argvv[i]); //Ejecutamos el proceso
							perror ("Error en exec. Esto no debería ejecutarse"); //controlamos errores
							exit(-1);					      //de ejecución
						}
						else if(i==command_counter-1){//Caso en el que tengamos al último proceso
							close(STDIN_FILENO); //Cerramos el descriptor de entrada estándar 
							dup(p[i-1][0]);	     //para cambiarlo por entrada de datos desde la tubería
							for(int j=0; j<command_counter-1; j++){//Cerramos los descriptores no usados
								close(p[j][0]);	
								close(p[j][1]);	
							}
							if((strcmp(filev[1], "0")!=0)){//Como es el último proceso, puede tener
										       //redirección de salida, por lo que evaluamos
										       //esa posibilidad
								int out=open(filev[1], O_CREAT|O_WRONLY|O_TRUNC, 00700);
								if(out==-1){
									perror ("Error al abrir el fichero");
									exit(-1);
								}
								close(STDOUT_FILENO);
								dup(out);
								close(out);
							}
							if((strcmp(filev[2], "0")!=0)){//Verificamos si tiene redirección de errores
								int err=open(filev[2], O_CREAT|O_WRONLY|O_TRUNC, 00700);
								if(err==-1){
									perror ("Error al abrir el fichero");
									exit(-1);
								}
									close(STDERR_FILENO);
								dup(err);
								close(err);
							}
							execvp(argvv[i][0],argvv[i]); //ejecutamos el comando
							perror("Error en exec. Esto no debería ejecutarse");//controlamos posibles
							exit(-1); 					    //errores en la ejecución
						}
						else{ //Caso en el que no sea ni el primer proceso ni el último
							close(STDIN_FILENO); //cerramos el descriptor de la entrada estándar
							close(STDOUT_FILENO);//cerramos el descriptor de la salida estándar
							dup(p[i-1][0]);//habilitamos la entrada de datos desde la tubería a la que 
								       //envió datos el proceso anterior
							dup(p[i][1]);//habilitamos la salida de datos a la tubería que empleará
								     //para coger los datos el siguiente proceso
							if((strcmp(filev[2], "0")!=0)){//Al ser un proceso intermedio, solamente puede 
										       //tener redirección de errores, por lo que evaluamos
										       //el caso
								int err=open(filev[2], O_CREAT|O_WRONLY|O_TRUNC, 00700);
								if(err==-1){
									perror ("Error al abrir el fichero");
									exit(-1);
								}
									close(STDERR_FILENO);
								dup(err);
								close(err);
							}
							execvp(argvv[i][0],argvv[i]); //Ejecutamos el comando
							perror("Error en exec. Esto no debería ejecutarse"); //controlamos posibles
							exit(-1);					     //errores de ejecución
						}
					}
					else{//Evaluamos si la cadena de procesos debe ejecutarse o no en background
						if(in_background==0){ //En caso negativo, el padre espera a la terminación de cada hijo
							while(wait(&status)!=pid[i]);
						}
						else{
							printf("[%d]\n",pid[i]); //En caso afirmativo, el padre no espera e imprimimos el
										//pid del hijo en ejecución
						}
					}
				}
			}
		}
              /************************ STUDENTS CODE ********************************/
	      if (command_counter > 0) {
                if (command_counter > MAX_COMMANDS)
                      printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
                /*else {
            	   // Print command
		   print_command(argvv, filev, in_background);
                }*/
              }
        }
	return 0;
}
