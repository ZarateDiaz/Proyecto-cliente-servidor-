/*
 ============================================================================
 Name        : Servidor.c
 Author      : Zárate Díaz Sofía Viridiana
 Description : Servidor remoto para proyecto de ACS
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <ctype.h>
//Puertos a los que puede acceder el servidor 
#define PUERTO 8080,9090,55555
//Tamaño del buffer 
#define BUFFER_SIZE 1024
//Número máximo de conexiones pendientes 
#define QLEN 1

// Lista de los comandos interactivos bloqueados
const char *comandos_bloqueados[] = {"vi", "nano", "emacs", "less", "more", "vim"};

// Función que sirve para revisar si el comando que ingresa el cliente es un comando interactivo 
int Revisa_comando(const char *comando) {
    char buffer_copia[BUFFER_SIZE]; //buffer para copiar el comando ingresado
    strncpy(buffer_copia, comando, BUFFER_SIZE - 1);//con strncpy se copia el string 
    buffer_copia[BUFFER_SIZE - 1] = '\0'; //con esto se garantiza que sea string valido 

    char *primer_token = strtok(buffer_copia, " \t"); //con strtok se divide el string y se extrae el primer token
    //Recorre el arreglo de comandos bloqueados como vi o nano 
    //con strcmp compara las cadenas y si hay coincidencia retorna el valor de 1 
    for (int i = 0; i < sizeof(comandos_bloqueados)/sizeof(char*); i++) {
        if (primer_token && strcmp(primer_token, comandos_bloqueados[i]) == 0) {
            return 1; // Comando bloqueado
        }
    }
    return 0; // Comando permitido
}

void main(int argc, char *argv[]) {
  /*Valida los argumentos que se ingresan para el servidor
  si es diferente de 2 argumentos manda un mensaje de cómo conectarlo y un ejemplo*/ 
    if (argc !=2 ){
      fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
      fprintf(stderr, "Ejemplo: %s %d \n", argv[0], PUERTO);
      exit(EXIT_FAILURE);
    }
    //Valida que el argumento que se ingresó sea númerico 
    //en caso contrario manda mensaje de error 
    for(char *p = argv[1]; *p != '\0';p++){
      if(!isdigit(*p)){
        fprintf(stderr, "Error: el puerto debe ser un numero \n");
        exit(EXIT_FAILURE);
      }
    }
  //variable para leer el puerto 
  int puerto_ingresado = atoi(argv[1]); 
  //Valida si el puerto que se ingresó sea 8080,9090 o 55555
  int puertos_validos[]={8080, 9090,55555};
  int valido=0;
  for(int i =0; i < sizeof(puertos_validos)/sizeof(int);i++){
    if(puerto_ingresado == puertos_validos[i]){
      valido=1;
      break;
    }
  }
  //en caso contrario manda mensaje de que el puerto que se ingresó no está permitido 
  if(!valido){
    fprintf(stderr, "Error: puerto no permitido, use uno de : 8080, 9090,55555\n ");
    exit(EXIT_FAILURE);
  }
  
    int servidor, cliente; // file descriptor para el servidor y el cliente
    struct sockaddr_in servidor_addr, cliente_addr; //estructuras para los socketes 
    int longClient; //longitud para la estructura del cliente que se conecta al servidor 
    struct hostent* info_cliente;//estructura para obtener la información del cliente 
    char buffer[BUFFER_SIZE]; //buffer para los comandos 
    

    // Crear socket para el servidor en caso de error manda mensaje 
    servidor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (servidor == -1) {
        perror("Error al crear socket");
        exit(1);
    }

    //Reutiliza una dirección (puerto) que ya está en uso
    //si se quiere volver a conectar a ese puerto hace que la conexión sea más rápida 
    if (setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1)
     {
       perror("Server-setsockopt() error!");
       exit(1);
     }
     else
       printf("Server-setsockopt is OK...\n");

    // Configurar dirección, se escribe el puerto
    //limpia el socket del servidor 
    memset((char *)&servidor_addr,0,sizeof(servidor_addr));
    //especifica las direcciones ipv4 
    servidor_addr.sin_family = AF_INET;
    //el servidor se conecte de manera remota 
    servidor_addr.sin_addr.s_addr = INADDR_ANY;
    //se pasa el puerto ingresado a formato de red 
    servidor_addr.sin_port = htons(atoi(argv[1]));
    //verifica que no haya absura en los bytes 
    memset(&(servidor_addr.sin_zero),'\0',8);

    // Enlaza a un puerto local del servidor 
    if (bind(servidor, (struct sockaddr *)&servidor_addr, sizeof(servidor_addr)) < 0) {
        perror("Error en bind");
        close(servidor);
        exit(1);
    }

    // Escuchar, se usa para las conexiones entrantes 
    listen(servidor, QLEN);
    longClient=sizeof(cliente_addr);
    printf("Servidor esperando conexión en el puerto %d ...\n", puerto_ingresado);

    // Aceptar conexión
    cliente = accept(servidor, (struct sockaddr*)&cliente_addr, &longClient);
    //Muestra la información del cliente conectado
    info_cliente=gethostbyaddr((char *) &cliente_addr.sin_addr, sizeof (struct in_addr), AF_INET);
    if( info_cliente == NULL )
        {
          printf("Cliente conectado pero no puedo obtener su origen\n");
        }
        else
        {
          /* Obtiene la fecha y hora en el que el cliente se conectó se imprime en pantalla */
          time_t T = time(NULL);
          struct tm tm = *localtime(&T);
          printf("%02d/%02d/%04d %02d:%02d:%02d-Cliente conectado ", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
          printf(" desde: %s\n\n", inet_ntoa(cliente_addr.sin_addr));
        }

    // Bucle infinito para recibir mensajes del cliente 
    while (1) {
      //limpia el buffer 
    	memset(buffer, 0, BUFFER_SIZE);
      //lee los datos del socket cliente
        int bytes = read(cliente, buffer, BUFFER_SIZE - 1);
        //si no se reciben datos o hubo un error se sale del bucle 
        if (bytes <= 0)
        	break;
        //elimina el salto de linea al final del comando recibido 
        buffer[strcspn(buffer, "\n")] = '\0';
        // Verifica si el comando recibido es un comando que está bloqueado como vi,less,nano,etc
    if (Revisa_comando(buffer)) {
      //Mensaje de error al usuario en caso de que haya ingresado un comando interactivo 
        const char *mensaje = "\n[ERROR] El servidor no soporta comandos interactivos como 'vi', 'less', 'nano', etc.\n"
                               "  → Use 'cat' o 'echo' para manipular archivos.\n"
                             "  → Escriba exit para salir.\n\n";
        //Se envía el mensaje al usuario                    
        write(cliente, mensaje, strlen(mensaje));
        //continua con el siguiente comando 
        continue; 
    }
        //Se crea el buffer para almacenar una copia del comando recibido del cliente 
        char comando_usuario[BUFFER_SIZE];
        //Se copia el contenido del buffer principal al auxiliar 
        strcpy(comando_usuario, buffer);
        //Se compara el comando que ingresa el usuario con exit si coincide se cierra conexion
        //y se envía mensaje de que se cierra conexión , además se rompe el ciclo while 
         if ( strcmp(comando_usuario, "exit")==0){
          printf("Cerrando conexión...\n");
          break;
        }
        
      

        // Arreglo para almacenar los file descriptors del pipe 
        int pipefd[2];
        //se crea un pipe para la comunicación entre los procesos 
        if (pipe(pipefd) == -1) {
        perror("pipe");
        continue;
        }
        //se crea proceso hijo con fork 
        pid_t pid = fork();
        //Entra el proceso hijo para que se redirija la salida estandar y de error al pipe 
        if (pid == 0) {
        //El proceso hijo solo escribirá 
        close(pipefd[0]); // Cierra el extremo de lectura
        //Se redirige la salida estándar al extremo de escritura del pipe 
        dup2(pipefd[1], STDOUT_FILENO); 
        //Se redirige la salida de errores estándar al mismo pipe 
        dup2(pipefd[1], STDERR_FILENO);  
        //Se cierra el extremo de escritura 
        close(pipefd[1]); 
        
        //Se cierra la entrada estandar , mata comandos interactivos que pueden bloquear el servidor 
        close(STDIN_FILENO); 
       
        // Ejecutar el comando usando /bin/sh -c
        execl("/bin/sh", "sh", "-c", buffer, NULL);
         
        // Si execl falla, por ejemplo si es un comando que no existe manda error 
        perror("execl");
        //termina proceso hijo
        exit(EXIT_FAILURE);
        } 
        //si pid es mayor a 0 entonces es el proceso padre 
        else if (pid>0){
        	//El proceso padre solo lee, por lo que se cierra el extremo de escritura del pipe
        	close(pipefd[1]); 
          //espero a que el proceso hijo finalice de ejecutar el comando 
        waitpid(pid,NULL,0);
          // Leer del pipe y enviar al cliente
        	int n; //número de bytes 
          int salida=0; //variable para verificar si el comando da salida 
  
          //Se lee la salida del comando 
        	while ((n = read(pipefd[0], buffer, BUFFER_SIZE - 1)) > 0) {
          salida=1; //verifica que hubo datos 
        	buffer[n] = '\0'; //Terminación nula 
        	write(cliente, buffer, n); //se envía salida al cliente 
        	}
          //si el comando no produjo salida se notifica al cliente 
          if (!salida){
            
              strcpy(buffer, "Comando sin salida \n");
              write(cliente,buffer,strlen(buffer));//se envía el mensaje al cliente 
             
          }
          //Se cierra el extremo de lectura 
        	close(pipefd[0]);
        }
        else {
          //si fork falla imprime error 
        	perror("fork");
        }

       
    }
    //se cierra socket del cliente 
    close(cliente);
    //se cierra el socket del servidor 
    close(servidor);
    //Termina con éxito 
    exit(0);
}


