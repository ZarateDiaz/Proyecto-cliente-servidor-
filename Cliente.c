/*
 ============================================================================
 Name        : Cliente.c
 Author      : Zárate Díaz Sofía Viridiana 
 Description : programa para el cliente 
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
#include <ctype.h>
//Se define el tamaño del buffer para los comandos 
#define BUFFER_SIZE 1024


/*Función creada para guiar al cliente en la conexión al servidor
se imprime en la terminal el mensaje; muestra los puertos donde puede estar conectado el servidor
un ejemplo de como conectarse , y algunos errores comunes que hacen que la conexión no se realice*/

void mensaje_conexion (const char *ejecutable) {
    fprintf(stderr,
        "Uso correcto: %s <IP> <Puerto>\n"
        "Puertos donde puede estar conectado el servidor: 8080,9090,55555\n"
        "  Ejemplo: %s 127.0.0.1 8080\n\n"
        "Errores comunes:\n"
        "  - Usar letras en el puerto: %s 192.168.1.1 abc\n"
        "  - Invertir orden: %s 8080 127.0.0.1\n",
        ejecutable, ejecutable, ejecutable, ejecutable);
    exit(EXIT_FAILURE);
}
/*Función creada para verificar que el puerto ingresado es un número y no una letra 
con la función isdigit se verifica si lo que se ingresó el cliente es un número. 
En caso de que no sea un número regresa un 0 y en caso de que si sea un número retorna un 1*/
int puerto_num (const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;  // No es número
        }
    }
    return 1;  // Es número
}
//Función main 
void main(int argc, char *argv[]) {
    /*Aquí se valida que los argumentos ingresados del cliente sean 2 argumentos (IP y puerto). 
    En caso de que sea diferente de 2 argumentos se enviarán las instrucciones para conectarse 
    al servidor con la función mensaje_conexion ()*/
    if (argc != 3) {
        fprintf(stderr, "Error: Se requieren 2 argumentos.\n");
        mensaje_conexion (argv[0]);
    }

    /*Valida que el cliente haya ingresado un puerto numérico con ayuda de la función "puerto_num ". 
    En caso de que el puerto no sea númerico muestra los puertos donde puede estar conectado el 
    servidor, un mensaje de error , los datos que ingresó el cliente 
    y la manera en que se debe de conectar al servidor */
    if (!puerto_num (argv[2])) {
        fprintf(stderr,
            "Error: El puerto debe ser un número.\n"
            "Puertos donde puede estar conectado el servidor: 8080,9090,55555\n"
            "  Ingresaste: '%s'\n", argv[2]);
        mensaje_conexion (argv[0]);
    }

    /*Valida que el primer argumento que ingrese el cliente no sea un número, 
    en caso de que el cliente escriba primero el puerto y después la IP saldrá un mensaje de error*/
    if (puerto_num (argv[1])) {
        fprintf(stderr,
            "Error: El primer argumento debe ser la IP, no el puerto.\n"
            "  Orden correcto: %s <IP> <Puerto>\n", argv[0]);
        mensaje_conexion (argv[0]);
    }
   
	
    int cliente; // file descriptor para el socket cliente

    struct sockaddr_in socket_cliente; //estructura para el socket cliente
    struct hostent *h; //esctructura para la dirección IP 

    char buffer[BUFFER_SIZE]; //buffer para los comandos y las respuestas 
    

    // Se crea el socket cliente en caso de fallar manda mensaje de error 
    cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (cliente == -1) {
        perror("Error al crear socket");
        exit(1);
    }

    // Limpia el socket del cliente
    memset((char *)&socket_cliente,0,sizeof(socket_cliente));
    //Especifica la familia de direcciones IPv4 
    socket_cliente.sin_family = AF_INET;
    //Se convierte el puerto ingresado a formato de red 
    socket_cliente.sin_port = htons((u_short)atoi(argv[2]));
    //Se pasa la IP que ingrese el cliente con argv[1] y se convierte la IP 
    h=gethostbyname(argv[1]);
    //Prepara la dirección IP para la conexión 
   memcpy(&socket_cliente.sin_addr, h->h_addr,h->h_length);

    // Se conecta al servidor en caso de error devuelve -1 y un mensaje de error 
    if (connect(cliente, (struct sockaddr *)&socket_cliente, sizeof(socket_cliente)) < 0) {
        perror("Error al conectar con el servidor");
        close(cliente);
        exit(1);
    }
    //mensaje de conectado al cliente y en caso de querer salir escriba exit 
    printf("Conectado al servidor. Escribe 'exit' para desconectarte.\n");

    // Ciclo while para enviar mensajes al servidor
    // cuando el usuario ingrese exit se rompe el cliclo while
    while (1) {
        
        printf("\nComando> ");

         //Lee la entrada del usuario y se almacena en el buffer   
         fgets(buffer, BUFFER_SIZE, stdin);
          // Quitar salto de línea
        buffer[strcspn(buffer, "\n")] = '\0';
        //En caso de que el usuario ingrese exit se cierra conexion 
        //con strcmp se compara lo que ingrese el cliente con exit si resulta igual a 0 se desconecta 
        if (strcmp(buffer, "exit") == 0) {
            write(cliente, buffer, strlen(buffer)); //enviar mensaje de exit al servidor 
            printf("Desconectando...\n"); // se imprime mensaje de "Desconectando" al cliente
            close(cliente); //se cierra conexion del cliente 
            exit(0); //termina el programa
        }
        /*Se envía el comando que ingrese el cliente al servidor 
        en caso de ser -1 manda mensaje de error*/

        if(write(cliente, buffer, strlen(buffer)) < 0){
            perror("Error al enviar comando");
            break;
        }
        
        // Leer respuesta del servidor
       
        printf("Salida del comando:\n");
        //variable para el numero de bytes 
        int n;
        //ciclo while para leer la respuesta del servidor 
        while ((n = read(cliente, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[n] = '\0'; //marca el final de la cadena 
        printf("%s", buffer); //imprime respuesta del servidor 
        if (n < BUFFER_SIZE - 1)      
        	break; // Asume que no hay mas datos por leer 
        
        }
        
        
    }

    close(cliente); //cierra el socket del cliente
    exit(0);//termina con éxito 
}