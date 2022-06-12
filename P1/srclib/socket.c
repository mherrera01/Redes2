#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include "../includes/socket.h"

struct addrinfo *res = NULL;
int sockfd;

int startServer(char* ip, char* port, int maxClients){
    struct addrinfo hints;
    int enable = 1;

    /* Definimos los parámetros de la estructura addrinfo hints */
    memset(&hints, 0, sizeof(hints)); /* Inicializamos la estructura */
    hints.ai_family = AF_INET; /* AF_INET para IPv4, AF_INET6 para IPv6 o AF_UNSPEC para ambos */
    hints.ai_socktype = SOCK_STREAM; /* SOCK_STREAM para TCP o SOCK_DGRAM para UDP */
    hints.ai_flags = AI_PASSIVE; /* Asignamos automáticamente la IP */
    hints.ai_protocol = 0; /* Esto debe estar así a no ser que queramos algo distinto de TCP o UDP */

    /* Control de errores en caso de que ip o port sea NULL */
    if(!ip || !port){
        return -1;
    }

    /**
     * Obtenemos la información necesaria en la lista res esepecificando lo siguiente:
     * - Dirección ip que puede ser NULL en caso de que haya sido ya definida en
     * la estructura hints en el campo ai_flags (AI_PASSIVE).
     * - Número del puerto (como string, p.ej. "80") o su nombre si tiene uno como "http".
     * - La estructura addrinfo hints que contiene información del socket.
     * 
     * Devuelve un valor distinto de 0 en caso de error
     */
    if (getaddrinfo(ip, port, &hints, &res) != 0){
        printf("Ha habido un error obteniendo la información del socket para el servidor.\n");
        return -1;
    }

    /* Abrimos un socket con la información guardada en res */
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        printf("Ha habido un error abriendo el socket para el servidor.\n");
        return -1;
    }

    /* Configuramos que el socket se pueda reusar sin esperar a que el sistema operativo lo cierre */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1){
        printf("Ha habido un error al configurar que el socket se pueda reusar.\n");
        return -1;
    }

    /* Hacemos bind al puerto que pasamos para usar getaddrinfo() */
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        printf("Ha habido un error haciendo bind al puerto del socket %d.\n", sockfd);
        return -1;
    }

    /* Configuramos el número de clientes máximos que el servidor puede soportar concurrentemente */
    if (listen(sockfd, maxClients) == -1){
        printf("Ha habido un error configurando la lista de clientes máximos del servidor.\n");
        return -1;
    }

    return 0;
}

int startClient(char* ip, char* port){
    struct addrinfo hints;

    /* Definimos los parámetros de la estructura addrinfo hints */
    memset(&hints, 0, sizeof(hints)); /* Inicializamos la estructura */
    hints.ai_family = AF_INET; /* AF_INET para IPv4, AF_INET6 para IPv6 o AF_UNSPEC para ambos */
    hints.ai_socktype = SOCK_STREAM; /* SOCK_STREAM para TCP o SOCK_DGRAM para UDP */
    hints.ai_flags = AI_PASSIVE; /* Asignamos automáticamente la IP */
    hints.ai_protocol = 0; /* Esto debe estar así a no ser que queramos algo distinto de TCP o UDP */

    /* Control de errores en caso de que ip o port NULL */
    if(!ip || !port){
        return -1;
    }

    /**
     * Obtenemos la información necesaria en la lista res esepecificando lo siguiente:
     * - Dirección ip que puede ser NULL en caso de que haya sido ya definida en
     * la estructura hints en el campo ai_flags (AI_PASSIVE).
     * - Número del puerto (como string, p.ej. "80") o su nombre si tiene uno como "http".
     * - La estructura addrinfo hints que contiene información del socket.
     * 
     * Devuelve un valor distinto de 0 en caso de error
     */
    if (getaddrinfo(ip, port, &hints, &res) != 0){
        printf("Ha habido un error obteniendo la información del socket para el servidor.\n");
        return -1;
    }

    /* Abrimos un socket con la información guardada en res */
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        printf("Ha habido un error abriendo el socket para el servidor.\n");
        return -1;
    }

    return sockfd;
}

void closeServer(){
    /*Sólo liberará res si este ha sido inicializado.*/
    if(res != NULL){
        freeaddrinfo(res);
    }

    close(sockfd);
}

void closeClient(){
    /*Sólo liberará res si este ha sido inicializado.*/
    if(res != NULL){
        freeaddrinfo(res);
    }

    /*Envía el mensaje que cierra la comunicación.*/
    connectionSend(sockfd, endCommunicationMessage, strlen(endCommunicationMessage));

    /*Cerramos el socket al terminar.*/
    close(sockfd);
    return;
}

int clientConnect(){
    int tries;
    for(tries = 0; tries < maxTries; tries++){
        if(connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
            printf("Error al conectar. Reintentando.\n");
            usleep(microsecondsBetweenTries);
        } else {
            break;
        }
    }

    if(tries == maxTries){
        return -1;
    }
    return 0;
}

int serverAccept(){
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int sfd;

    addr_size = sizeof their_addr;
    printf("Buscando clientes.\n");

    sfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    if(sfd == -1) return -1;
    printf("Cliente encontrado.\n");

    return sfd;
}

int connectionSend(int sfd, char* msg, int len){
    int bytes_sent;

    if(msg == NULL) return -1;

    /**
     * Añadimos la flag MSG_NOSIGNAL para que la señal SIGPIPE no termine el programa
     * abruptamente.
     * 
     * La señal SIGPIPE se genera porque ha habido un error en el cliente y éste cierra
     * el socket, por lo que se intenta mandar bytes a un socket cerrado
     */
    bytes_sent = send(sfd, msg, len, MSG_NOSIGNAL);

    /*Si mandamos menos bytes que el mensaje que queremos mandar, ha habido un error.*/
    if(bytes_sent < len) return -1;
    return 0;
}

int connectionRecieve(int sfd, char* received){
    int rcvBytes, flags=0;
    
    if (received == NULL) return -1;

    /*Vaciamos lo que recibimos en el mensaje anterior.*/
    memset(received, 0, maxLenMessage*sizeof(char));

    rcvBytes = recv(sfd, (void*)(received), maxLenMessage, flags);
    if(rcvBytes == -1){
        return -1;
    }

    if(strcmp(received, endCommunicationMessage)==0) return -2;

    return rcvBytes;
}