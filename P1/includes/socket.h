#ifndef SOCKET_H
#define SOCKET_H

#define maxLenMessage 4096
#define endCommunicationMessage "Connection: close"
#define defaultIP "127.0.0.1"
#define defaultPort "8081"

#define maxTries 10 /* Número máximo de intentos de conectarse por parte del cliente hasta darlo por imposible. */
#define microsecondsBetweenTries 1000000 /* Número de microsegundos que esperará el cliente entre intentos de conexión. */

/* Función que inicializa el servidor, preparado para escuchar peticiones de clientes.
   Devuelve -1 en caso de error y 0 en caso contrario */
int startServer(char* ip, char* port, int maxClients);

/* Función que cierra el servidor y libera sus recursos */
void closeServer();

/* Función para inicializar el cliente. Recibe la IP del servidor como argumento.
   Devuelve el descriptor del socket del cliente si todo funciona correctamente, y -1 en caso de error. */
int startClient(char* ip, char* port);

/* Función que cierra el cliente y libera sus recursos */
void closeClient();

/* Función para conectar el cliente al servidor. Devuelve 0 en caso de que la conexión se efectuara con éxito y -1 en caso contrario.*/
int clientConnect();

/* Función por la que el servidor recibe las conexiones de los clientes y empieza una comunicación con ellos.
   Devuelve el descriptor del socket si todo funciona correctamente, y -1 en caso de error */
int serverAccept();

/* Función por la que uno de los agentes en la comunicación envía un mensaje. Recibe como argumento el mensaje que quiere enviar
   junto con su longitud y devuelve -1 en caso de error y 0 en caso contrario */
int connectionSend(int sfd, char* msg, int len);

/* Función por la que uno de los agentes en la comunicación recibe un mensaje en el puntero a char que tiene de argumento.
   Devuelve:
      -1 en caso de error
      -2 en caso de recibir bien el mensaje de fin de la comunicación
      Número de bytes leídos en caso de recibir bien el mensaje.
*/
int connectionRecieve(int sfd, char* received);

#endif