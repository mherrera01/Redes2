#include <stdio.h>
#include <pthread.h> /* Librería para los hilos POSIX. Compilación con la flag -pthread */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <confuse.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <syslog.h>

#include "../includes/pool_thread.h"
#include "../includes/socket.h"
#include "../includes/picohttpparser.h"

#define dateLength 50 /* Tamaño del string donde se guardará la fecha en formato http */

/**
 * Estructura que define la información necesaria que los
 * hilos del servidor reaquieren saber para mandar respuestas
 * a las peticiones de los clientes
 */
typedef struct{
    char *resources_root;
    char *signature;
} server_info;

/**
 * Estructura que define la información de las peticiones
 * de los cliente
 */
typedef struct{
    char method[50]; /* El verbo de la petición */
    int version; /* Versión http de la petición */
    char path_resource[100]; /* Variable para recibir la ruta del recurso solicitado */
    char post_args[256]; /* Variable para recibir los argumentos de un método POST */
    char msg[maxLenMessage]; /* Variable para recibir el mensaje por parte del cliente */
} cliente_request;

/**
 * Función que devuelve la fecha actual en formato http en
 * la variable pasada como argumento.
 * 
 * Devuelve -1 en caso de error y 0 en caso contrario
 */
int getDateHttp(char *date){
    time_t now;
    struct tm tm;

    /* Control de errores de argumentos */
    if (date == NULL){
        printf("Hubo un error al transformar la fecha actual a formato http.\n");
        return -1;
    }

    /* Conseguimos la fecha actual */
    now = time(0);
    tm = *gmtime(&now);

    /* Transformamos la fecha al formato http y lo guardamos en date */
    strftime(date, dateLength, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return 0;
}

/**
 * Función que devuelve la última fecha de modificación en date
 * de un archivo pasado como argumento.
 * 
 * Devuelve -1 en caso de error y 0 en caso contrario
 */
int getLastModifiedDate(char *path_file, char *date){
    struct stat file_attr;

    /* Control de errores de argumentos*/
    if (path_file == NULL || date == NULL){
        printf("Ha habido un error al obtener la última fecha de modificación de un archivo.\n");
        return -1;
    }

    if (stat(path_file, &file_attr) == -1){
        printf("Ha habido un error al obtener la última fecha de modificación del recurso solicitado.\n");
        return -1;
    }
    strftime(date, dateLength, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&file_attr.st_mtime));

    return 0;
}

/**
 * Función que devuelve el tipo de un recurso script en type
 * a partir de su extensión.
 * 
 * Devuelve -1 si la extensión del script no está permitida o hay
 * algún error y 0 en caso contrario
 */
int getScriptType(char *path_resource, char *type){
    char *startExt = NULL;

    /* Control de errores de argumentos */
    if (path_resource == NULL || type == NULL){
        printf("Ha habido un error al obtener el tipo de script solicitado.\n");
        return -1;
    }

    /**
     * Comprobamos qué tipo de script se solicita
     * La función strrchr devuelve la posición de la última aparición de un caracter
     * en la string pasada como argumento
     */
    startExt = strrchr(path_resource, '.');
    if (startExt == NULL){
        printf("No hay extensión en el script solicitado.\n");
        return -1;
    }

    if (strstr(startExt, ".py")) {
        strcpy(type, "python");
    } else if (strstr(startExt, ".php")) {
        strcpy(type, "php");

    /* Tipo de script no soportado */
    } else {
        printf("El tipo de script solicitado no está soportado por el servidor.\n");
        return -1;
    }

    return 0;
}

/**
 * Función que devuelve el tipo del contenido de un recurso en type
 * a partir de su extensión.
 * 
 * Devuelve -1 si la extensión del recurso no está permitida o hay
 * algún error y 0 en caso contrario
 */
int getContentType(char *path_resource, char *type){
    char *startExt = NULL;

    /* Control de errores de argumentos */
    if (path_resource == NULL || type == NULL){
        printf("Ha habido un error al obtener el tipo de recurso solicitado.\n");
        return -1;
    }

    /**
     * Comprobamos qué tipo de recurso se solicita
     * La función strrchr devuelve la posición de la última aparición de un caracter
     * en la string pasada como argumento
     */
    startExt = strrchr(path_resource, '.');
    if (startExt == NULL){
        printf("No hay extensión en el recurso solicitado.\n");
        return -1;
    }

    if (strstr(startExt, ".txt")) {
        strcpy(type, "text/plain");

    } else if (strstr(startExt, ".html") || strstr(startExt, ".htm")) {
        strcpy(type, "text/html");

    } else if (strstr(startExt, ".gif")) {
        strcpy(type, "image/gif");

    } else if (strstr(startExt, ".jpeg") || strstr(startExt, ".jpg")) {
        strcpy(type, "image/jpeg");

    } else if (strstr(startExt, ".mpeg") || strstr(startExt, ".mpg")) {
        strcpy(type, "video/mpeg");

    } else if (strstr(startExt, ".doc") || strstr(startExt, ".docx")) {
        strcpy(type, "application/msword");

    } else if (strstr(startExt, ".pdf")) {
        strcpy(type, "application/pdf");

    /* Tipo de recurso no soportado */
    } else {
        printf("El tipo de recurso solicitado no está soportado por el servidor.\n");
        return -1;
    }

    return 0;
}

/**
 * Función que construye la ejecución del script con sus argumentos.
 * 
 * Devuelve el descriptor de la tubería por donde se lee el resultado del
 * script y -1 en caso de error.
 */
int executeScript(char *script_type, char *script_path, char *script_args){
    pid_t pid;
    int pipe_in[2], pipe_out[2];
    char stdin_buf[256];
    fd_set readfds;
    struct timeval time;
    int ret;

    /* Control de errores de argumentos */
    if (script_type == NULL || script_path == NULL || script_args == NULL){
        printf("Ha habido un error al ejecutar el script.\n");
        return -1;
    }

    /**
     * Creamos las tuberías
     * 
     * Requerimos de dos tuberías ya que es una conexión bidireccional
     * entre el proceso que escribe por STDIN y el que manda de vuelta
     * el resultado del script.
     */
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1){
        printf("Ha habido un error al crear las tuberías para la ejecución del script.\n");
        return -1;
    }

    /* Creamos un proceso hijo con fork */
    pid = fork();
    if (pid < 0){
        printf("Ha habido un error al crear un proceso con fork para ejecutar el script.\n");
        return -1;

    /* El proceso padre correspondiente a la escritura por STDIN y lectura del resultado del script */
    } else if (pid > 0){
        /* Cerramos la tubería pipe_in en modo lectura ya que vamos a escribir lo que esté en STDIN */
        close(pipe_in[0]);
        /* Cerramos la tubería pipe_out en modo escritura ya que vamos a leer el resultado del script */
        close(pipe_out[1]);

        /* Configuramos que se espere n segundos para recibir datos por STDIN */
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        time.tv_sec = 5;
        time.tv_usec = 0;

        /* Esperamos n segundos por STDIN */
        ret = select(1, &readfds, NULL, NULL, &time);
        if (ret == -1){
            printf("Ha habido un error al hacer un timeout en STDIN.\n");
            return -1;

        /* Se encuentran datos en STDIN */
        } else if (ret > 0){
            /* Escribimos lo que esté en STDIN */
            if (stdin){
                fgets(stdin_buf, sizeof(stdin_buf), stdin);
                if (write(pipe_in[1], stdin_buf, strlen(stdin_buf)) == -1){
                    printf("Ha habido un error al enviar datos por STDIN.\n");
                    return -1;
                }
            }
            close(pipe_in[1]);
        }

        /* Esperamos a que el hijo termine la ejecución del script */
        wait(NULL);

        /* Devolvemos el descriptor donde se encuentra el resultado del script */
        return pipe_out[0];
    }

    /* El proceso hijo correspondiente a la lectura de STDIN y envío del resultado del script */

    /* Cerramos la tubería pipe_out en modo lectura ya que vamos a escribir el resultado del script */
    close(pipe_out[0]);
    /* Cerramos la tubería pipe_in en modo escritura ya que vamos a leer STDIN */
    close(pipe_in[1]);

    /* Esperamos hasta que el proceso padre escriba por STDIN */
    sleep(6);

    /* Asignamos la tubería pipe_in en modo lectura a la entrada STDIN */
    dup2(pipe_in[0], STDIN_FILENO);
    /* Asignamos la tubería pipe_out en modo escritura a la salida STDOUT */
    dup2(pipe_out[1], STDOUT_FILENO);

    /**
     * Ejecutamos el script con sus argumentos.
     * 
     * El primer y segundo argumento corresponde al ejecutable del script y el
     * tercero es la ruta donde se encuentra el script a ejecutar. El resto
     * de argumentos son los propios argumentos que requiera el script. La función
     * siempre debe acabar en NULL.
     * 
     * Escribe el resultado en STDOUT y por lo tanto en pipe_out[1] ya que así
     * lo asignamos con dup2
     */
    if (execlp(script_type, script_type, script_path, script_args, (char *) NULL) == -1){
        printf("Ha habido un error al ejecutar el script en el proceso hijo.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/**
 * Función que devuelve al cliente pasado como argumento
 * una respuesta 400 Bad Request a su petición.
 * 
 * Devuelve -1 en caso de error y 0 en caso contrario
 */
int serverResponse400(int sockfd, server_info *sinfo, cliente_request *req){
    char response[maxLenMessage];
    char date[dateLength];

    /* Control de error en los argumentos */
    if (sinfo == NULL || req == NULL){
        printf("Ha habido un error al responder 400 Bad Request a la petición.\n");
        return -1;
    }

    /* Hallamos la fecha actual en formato http */
    if (getDateHttp(date) == -1){
        printf("Ha habido un error al responder 400 Bad Request a la petición.\n");
        return -1;
    }

    /* Construimos y mandamos la respuesta 400 Bad Request */
    sprintf(response, "HTTP/1.%d 400 Bad Request\r\nDate: %s\r\nServer: %s\r\n"
            "Content-Length: 37\r\nContent-Type: text/html\r\n\r\n<html><b>400 Bad Request</b></html>\r\n",
            req->version, date, sinfo->signature);

    if (connectionSend(sockfd, response, strlen(response)) == -1){
        printf("No se ha podido mandar la respuesta 400 Bad Request desde el servidor.\n");
        return -1;
    }

    return 0;
}

/**
 * Función que devuelve al cliente pasado como argumento
 * una respuesta 404 Not Found a su petición.
 * 
 * Devuelve -1 en caso de error y 0 en caso contrario
 */
int serverResponse404(int sockfd, server_info *sinfo, cliente_request *req){
    char response[maxLenMessage];
    char date[dateLength];

    /* Control de error en los argumentos */
    if (sinfo == NULL || req == NULL){
        printf("Ha habido un error al responder 404 Not Found a la petición.\n");
        return -1;
    }

    /* Hallamos la fecha actual en formato http */
    if (getDateHttp(date) == -1){
        printf("Ha habido un error al responder 404 Not Found a la petición.\n");
        return -1;
    }

    /* Construimos y mandamos la respuesta 404 Not Found */
    sprintf(response, "HTTP/1.%d 404 Not Found\r\nDate: %s\r\nServer: %s\r\n"
            "Content-Length: 35\r\nContent-Type: text/html\r\n\r\n<html><b>404 Not Found</b></html>\r\n",
            req->version, date, sinfo->signature);

    if (connectionSend(sockfd, response, strlen(response)) == -1){
        printf("No se ha podido mandar la respuesta 404 Not Found desde el servidor.\n");
        return -1;
    }

    return 0;
}

/**
 * Función que devuelve al cliente pasado como argumento
 * una respuesta a su petición.
 * 
 * Devuelve -1 en caso de error y 0 en caso contrario
 */
int serverResponse(int sockfd, server_info *sinfo, cliente_request *req){
    char response[maxLenMessage], script_res[1024];
    char date[dateLength];
    char absolutePath[100], path_no_args[100];
    char final_path_resource[200];
    FILE *file = NULL;
    long content_length;
    char content_type[30], script_type[30];
    char date_lastModified[dateLength];
    char *arguments = NULL;
    int bytes_read = 0, script_fd;

    /* Control de error en los argumentos */
    if (sinfo == NULL || req == NULL){
        printf("Ha habido un error al responder la petición del cliente.\n");
        return -1;
    }

    /* Comprobamos la versión de la petición */
    if (req->version != 1){
        printf("Versión de la petición no soportada por el servidor.\n");
        serverResponse400(sockfd, sinfo, req);
        return -1;
    }

    /* Hallamos la fecha actual en formato http */
    if (getDateHttp(date) == -1){
        printf("Ha habido un error al responder la petición del cliente.\n");
        return -1;
    }

    /* Hallamos la ruta absoluta del directorio en el que el servidor está trabajando */
    if (getcwd(absolutePath, sizeof(absolutePath)) == NULL){
        printf("Ha habido un error al obtener la ruta absoluta del servidor.\n");
        return -1;
    }

    /* El verbo de la petición es GET */
    if (strstr(req->method, "GET")){
        /* Comprobamos si la petición GET contiene argumentos en la URL */
        if (strstr(req->path_resource, "?")){
            /* Obtenemos la ruta de la URL sin los argumentos */
            strcpy(path_no_args, strtok(req->path_resource, "?"));

            /* Comprobamos si el script solicitado es soportado por el servidor */
            if (getScriptType(path_no_args, script_type) == -1){
                serverResponse400(sockfd, sinfo, req);
                return -1;
            }

            /* Obtenemos la ruta completa del recurso solicitado sin los argumentos */
            sprintf(final_path_resource, "%s%s%s", absolutePath, sinfo->resources_root, path_no_args);

            /* Obtenemos los argumentos de la URL */
            arguments = strtok(NULL, "?");

            /* Vemos si el recurso script solicitado existe */
            file = fopen(final_path_resource, "rb");
            if (file == NULL){
                printf("El recurso script solicitado no se encuentra en el servidor.\n");
                serverResponse404(sockfd, sinfo, req);
                return -1;
            }
            fclose(file);

            /* Ejecutamos el script solicitado */
            script_fd = executeScript(script_type, final_path_resource, arguments);
            if (script_fd == -1){
                printf("No se ha podido ejecutar el recurso script solicitado.\n");
                return -1;
            }

            /* Obtenemos la última fecha de modificación del recurso script solicitado */
            if (getLastModifiedDate(final_path_resource, date_lastModified) == -1){
                printf("Ha habido un error al responder la petición del cliente.\n");
                return -1;
            }

            /* Inicializamos la variable script_res para escribir el resultado del script */
            memset(script_res, 0, sizeof(script_res));

            /* Leemos el resultado del script */
            if ((content_length = read(script_fd, script_res, sizeof(script_res))) == -1){
                printf("Ha habido un error al leer el resultado del script.\n");
                close(script_fd);
                return -1;
            }

            /* Comprobamos si el resultado del script es demasiado grande */
            if (content_length == sizeof(script_res)){
                printf("El resultado del script es demasiado grande.\n");
                close(script_fd);
                return -1;
            }
        
            /* Construimos y mandamos la cabecera junto con el resultado del script */
            sprintf(response, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\n"
                    "Content-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s\r\n",
                    date, sinfo->signature, date_lastModified, content_length, script_res);

            if (connectionSend(sockfd, response, strlen(response)) == -1){
                printf("No se ha podido mandar la cabecera y la respuesta del script al cliente desde el servidor.\n");
                close(script_fd);
                return -1;
            }

            close(script_fd);

        /* No hay argumentos en la URL */
        } else {
            /* Comprobamos si el recurso solicitado es soportado por el servidor */
            if (getContentType(req->path_resource, content_type) == -1){
                serverResponse400(sockfd, sinfo, req);
                return -1;
            }

            /* Obtenemos la ruta completa del recurso solicitado */
            sprintf(final_path_resource, "%s%s%s", absolutePath, sinfo->resources_root, req->path_resource);

            /* Vemos si el recurso solicitado existe */
            file = fopen(final_path_resource, "rb");
            if (file == NULL){
                printf("El recurso solicitado no se encuentra en el servidor.\n");
                serverResponse404(sockfd, sinfo, req);
                return -1;
            }

            /* Movemos el puntero al final del fichero y obtenemos la longitud del mismo */
            fseek(file, 0, SEEK_END);
            content_length = ftell(file);

            /* Movemos el puntero de nuevo al principio del fichero para leerlo */
            rewind(file);

            /* Obtenemos la última fecha de modificación del recurso solicitado */
            if (getLastModifiedDate(final_path_resource, date_lastModified) == -1){
                printf("Ha habido un error al responder la petición del cliente.\n");
                return -1;
            }

            /* Construimos y mandamos la cabecera de la respuesta */
            sprintf(response, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\n"
                    "Content-Length: %ld\r\nContent-Type: %s\r\n\r\n",
                    date, sinfo->signature, date_lastModified, content_length, content_type);

            if (connectionSend(sockfd, response, strlen(response)) == -1){
                printf("No se ha podido mandar la cabecera al cliente desde el servidor.\n");
                fclose(file);
                return -1;
            }

            /* Vaciamos la variable response para volver a utilizarla */
            memset(response, 0, maxLenMessage*sizeof(char));

            /* Mandamos el contenido del recurso solicitado */
            while ((bytes_read = fread(response, sizeof(char), maxLenMessage, file)) > 0) {
                if (connectionSend(sockfd, response, bytes_read) == -1){
                    printf("No se ha podido mandar el recurso solicitado al cliente desde el servidor.\n");
                    fclose(file);
                    return -1;
                }

                /* Vaciamos la variable response para volver a utilizarla */
                memset(response, 0, maxLenMessage*sizeof(char));  
            }

            /* Cerramos el descriptor de fichero */
            fclose(file);
        }

    /* El verbo de la petición es POST */
    } else if (strstr(req->method, "POST")){
        /* Comprobamos si la petición POST contiene argumentos en la URL */
        if (strstr(req->path_resource, "?")){
            /* Obtenemos la ruta de la URL sin los argumentos */
            strcpy(path_no_args, strtok(req->path_resource, "?"));

            /* Comprobamos si el script solicitado es soportado por el servidor */
            if (getScriptType(path_no_args, script_type) == -1){
                serverResponse400(sockfd, sinfo, req);
                return -1;
            }

            /* Obtenemos la ruta completa del recurso solicitado sin los argumentos */
            sprintf(final_path_resource, "%s%s%s", absolutePath, sinfo->resources_root, path_no_args);

        /* No hay argumentos en la URL */
        } else {
            /* Comprobamos si el script solicitado es soportado por el servidor */
            if (getScriptType(req->path_resource, script_type) == -1){
                serverResponse400(sockfd, sinfo, req);
                return -1;
            }

            /* Obtenemos la ruta completa del recurso solicitado */
            sprintf(final_path_resource, "%s%s%s", absolutePath, sinfo->resources_root, req->path_resource);
        }

        /* Vemos si el recurso script solicitado existe */
        file = fopen(final_path_resource, "rb");
        if (file == NULL){
            printf("El recurso script solicitado no se encuentra en el servidor.\n");
            serverResponse404(sockfd, sinfo, req);
            return -1;
        }
        fclose(file);

        /* Ejecutamos el script solicitado */
        script_fd = executeScript(script_type, final_path_resource, req->post_args);
        if (script_fd == -1){
            printf("No se ha podido ejecutar el recurso script solicitado.\n");
            return -1;
        }

        /* Obtenemos la última fecha de modificación del recurso script solicitado */
        if (getLastModifiedDate(final_path_resource, date_lastModified) == -1){
            printf("Ha habido un error al responder la petición del cliente.\n");
            return -1;
        }

        /* Inicializamos la variable script_res para escribir el resultado del script */
        memset(script_res, 0, sizeof(script_res));

        /* Leemos el resultado del script */
        if ((content_length = read(script_fd, script_res, sizeof(script_res))) == -1){
            printf("Ha habido un error al leer el resultado del script.\n");
            close(script_fd);
            return -1;
        }

        /* Comprobamos si el resultado del script es demasiado grande */
        if (content_length == sizeof(script_res)){
            printf("El resultado del script es demasiado grande.\n");
            close(script_fd);
            return -1;
        }
    
        /* Construimos y mandamos la cabecera junto con el resultado del script */
        sprintf(response, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\n"
                "Content-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s\r\n",
                date, sinfo->signature, date_lastModified, content_length, script_res);

        if (connectionSend(sockfd, response, strlen(response)) == -1){
            printf("No se ha podido mandar la cabecera y la respuesta del script al cliente desde el servidor.\n");
            close(script_fd);
            return -1;
        }

        close(script_fd);

    /* El verbo de la petición es OPTIONS */
    } else if (strstr(req->method, "OPTIONS")){
        /* Construimos y mandamos la cabecera de la respuesta */
        sprintf(response, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: %s\r\nContent-Length: 0\r\n"
                "Allow: GET, POST, OPTIONS\r\n", date, sinfo->signature);

        if (connectionSend(sockfd, response, strlen(response)) == -1){
            printf("No se ha podido mandar la cabecera al cliente desde el servidor.\n");
            return -1;
        }

    } else{
        /* Verbo de la petición no soportado */
        printf("Verbo de la petición no soportado por el servidor.\n");
        serverResponse400(sockfd, sinfo, req);
        return -1;
    }

    return 0;
}

/**
 * Función que parsea la petición http recibida del cliente.
 * Recibe el socket por donde la petición llega y la variable
 * en la que se quiere guardar la información.
 * 
 * Devuelve -1 en caso de error, 1 si el cliente cierra la conexión y
 * 0 si la petición ha sido parseada correctamente
 */
int parseHttpRequest(int sockfd, cliente_request *req){
    char *method, *path;
    char aux[maxLenMessage];
    int pret, minor_version;
    struct phr_header headers[100];
    size_t method_len, path_len, num_headers;
    int recv;

    /* Control de errores en argumentos */
    if (req == NULL) return -1;

    /* Leemos la petición del cliente y obtenemos la longitud del mensaje */
    recv = connectionRecieve(sockfd, req->msg);

    if (recv == -1){
        printf("No se ha leído correctamente la petición del cliente.\n");
        return -1;
    } else if (recv == 0 || recv == -2){
        /* El cliente cierra la conexión */
        return 1;
    }

    /* Parseamos la petición del cliente */
    num_headers = sizeof(headers) / sizeof(headers[0]);
    pret = phr_parse_request(req->msg, recv, (const char **) &method, &method_len,
                             (const char **) &path, &path_len, &minor_version,
                             headers, &num_headers, 0);

    if (pret == -1){
        printf("Ha habido un error al parsear la petición del cliente.\n");
        return -1;
    }

    /* Comprobamos si la petición es demasiado larga */
    if (recv == maxLenMessage){
        printf("La petición del cliente es demasiado grande.\n");
        return -1;
    }

    /* Guardamos la información de la petición en la variable req */

    /* Guardamos el verbo */
    sprintf(aux, "%.*s\n", (int)method_len, method);
    strcpy(req->method, aux);

    /* Vaciamos la variable auxiliar para volver a utilizarla */
    memset(aux, 0, maxLenMessage*sizeof(char));

    /* Si el verbo es POST guardamos sus argumentos del cuerpo de la petición */
    if (strstr(req->method, "POST")){
        sprintf(aux, "%s", req->msg + pret);
        strcpy(req->post_args, aux);
    }
    memset(aux, 0, maxLenMessage*sizeof(char));

    /* Guardamos la versión de la petición */
    req->version = minor_version;

    /* Guardamos la ruta del recurso socilitado */
    sprintf(aux, "%.*s", (int)path_len, path);
    strcpy(req->path_resource, aux);

    return 0;
}

/**
 * Rutina del hilo creado con pthread_create que recibe peticiones de cliente
 * 
 * Código que el hilo ejecuta una vez es creado. Un argumento de tipo
 * void* puede ser pasado, por lo que es posible pasar estructuras de datos
 * complejas
 */
void* threadRoutine(void* sinfo){
    server_info *server = (server_info *)sinfo;
    cliente_request req;
    int sockfd, parse;

    /**
     * Configuramos que no se pueda cancelar el hilo hasta que no
     * esté sin atender a ningún cliente
     * 
     * El segundo argumento se define a NULL ya que no nos interesa el
     * antiguo valor de cancelabilidad
     */
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) != 0){
        printf("Hubo un error al configurar que el hilo no se pueda cancelar.\n");
        /* Terminamos el hilo */
        pthread_exit(NULL);
    }

    /* Esperamos y atendemos peticiones de clientes hasta que se decida cerrar el servidor */
    while(1){
        /* El hilo ya no está atendiendo a ningún cliente, así que puede ser cancelado por el padre */
        if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0){
            printf("Hubo un error al configurar que el hilo se pueda cancelar.\n");
            /* Terminamos hilo */
            pthread_exit(NULL);
        }

        /* Esperamos hasta que llega una petición de cliente y crea un socket para éste */
        if ((sockfd = serverAccept()) == -1){
            printf("Hubo un error al hacer serverAccept en el hilo.\n");
            /* Terminamos hilo */
            pthread_exit(NULL);
        }

        /* El hilo está atendiendo a un cliente, no se puede cancelar */
        if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) != 0){
            printf("Hubo un error al configurar que el hilo no se pueda cancelar.\n");
            close(sockfd);
            /* Terminamos hilo */
            pthread_exit(NULL);
        }

        /**
         * Recibimos los mensajes del cliente y después cerramos la conexión con el servidor.
         * Parseamos el mensaje http recibido y si el mensaje es igual a 0 respondemos
         * al cliente conforme a la petición hecha.
         */
        parse = parseHttpRequest(sockfd, &req);
        if (parse == -1){
            printf("Ha habido un error al parsear el mensaje en el hilo.\n");
        } else if (parse == 0) {
            printf("RECIBIDO: %s\n", req.msg);

            /* Respondemos la petición del cliente */
            if (serverResponse(sockfd, server, &req) == -1){
                printf("Ha habido un error al responder al cliente en el hilo.\n");
            } else printf("PETICIÓN RESPONDIDA.\n");
        }

        printf("SE TERMINA CONEXIÓN.\n");
        close(sockfd);
    }
}

/* Función main del programa que crea un servidor que recibe peticiones http */
int main(int argc, char *argv[]){
    int server, maxClients, daemon, fd;
    sigset_t oset;
    pthread_t *hilo = NULL; /* Array con los identificadores de n hilos */
    cfg_t *cfg = NULL;
    server_info sinfo;
    FILE *log = NULL;

    /* Definimos los valores por defecto de las key en el server.conf */
    cfg_opt_t opts[] =
    {
        CFG_STR("server_root", "/htmlfiles", CFGF_NONE),
        CFG_STR("max_clients", "10", CFGF_NONE),
        CFG_STR("listen_port", "8080", CFGF_NONE),
        CFG_STR("server_IP", "127.0.0.1", CFGF_NONE),
        CFG_STR("server_signature", "MyCoolServer/1.1", CFGF_NONE),
        CFG_STR("server_daemon", "0", CFGF_NONE),
        CFG_END()
    };

    /**
     * Inicializamos la estructura que lee el fichero de configuración del servidor
     * CFG_NONE significa que no se utilizan flags
     */
    cfg = cfg_init(opts, CFGF_NONE);
    if (cfg_parse(cfg, "server.conf") != CFG_SUCCESS){
        printf("Ha habido un error leyendo el archivo de configuración del servidor.\n");
        if (cfg != NULL) cfg_free(cfg);
        return -1;
    }

    /* Obtenemos el máximo número de clientes que el servidor puede soportar */
    maxClients = strtol(cfg_getstr(cfg, "max_clients"), NULL, 10);

    /* Obtenemos si queremos que el servidor sea un proceso demonio o no */
    daemon = strtol(cfg_getstr(cfg, "server_daemon"), NULL, 10);

    /* Guardamos en una estructura la ruta de recursos y el nombre del servidor */
    sinfo.resources_root = cfg_getstr(cfg, "server_root");
    sinfo.signature = cfg_getstr(cfg, "server_signature");

    /* Demonizamos el proceso servidor conforme lo pedido en server.conf */
    if (daemon){
        printf("Creando al servidor como proceso demonio...\nInformación en el syslog del sistema.\n");
        daemon_init();
    }

    /* Creamos un archivo para redirigir todos los mensajes de error e información */
    log = fopen("log.txt", "w");
    if (log == NULL){
        printf("Ha habido un error al abrir el archivo log del programa.\n");
        cfg_free(cfg);
        return -1;
    }

    /* Obtenemos el descriptor de fichero del log */
    fd = fileno(log);
    if (fd == -1){
        printf("Ha habido un error al obtener el descriptor del archivo log.\n");
        cfg_free(cfg);
        fclose(log);
        return -1;
    }

    printf("Creando el servidor...\nTraceback de información en el log.txt creado por el programa.\n");
    printf("Pulsa Ctrl + C para cerrar el servidor.\n\n");

    /* Redirigimos el stdout al archivo log creado anteriormente */
    dup2(fd, STDOUT_FILENO);

    /* Inicializamos el servidor, que ya empieza a escuchar peticiones de cliente */
    server = startServer(cfg_getstr(cfg, "server_IP"), cfg_getstr(cfg, "listen_port"), maxClients);
    if (server == -1){
        printf("Ha habido un error abriendo el servidor.\n");
        closeServer();
        cfg_free(cfg);
        fclose(log);
        return -1;
    }
    printf("Se espera recibir peticiones en el puerto %s...\n\n", cfg_getstr(cfg, "listen_port"));

    /* Bloqueamos la señal SIGINT y guardamos la antigua máscara de señales en oset */
    if (thread_block(SIGINT, &oset) == -1){
        printf("Ha habido un error al bloquear la señal SIGINT.\n");
        closeServer();
        cfg_free(cfg);
        fclose(log);
        return -1;
    }

    /* Alocamos memoria para los identificadores de n hilos */
    hilo = (pthread_t *)malloc(maxClients * sizeof(pthread_t));
    if (hilo == NULL){
        printf("Ha habido un error al crear la memoria para el array de hilos.\n");
        closeServer();
        cfg_free(cfg);
        fclose(log);
        return -1;
    }

    /* Creamos tantos hilos como hemos definido al alocar memoria para el array de identificadores,
       los cuales ejecutarán nuestra propia rutina */
    if (pool_th_ini(hilo, threadRoutine, maxClients, (void *)&sinfo) == -1){
        printf("Hubo un error al crear la piscina de hilos.\n");
        closeServer();
        cfg_free(cfg);
        fclose(log);
        free(hilo);
        return -1;
    }

    /* Esperamos a recibir la señal SIGINT pero con la máscara que no la bloquea */
    sigsuspend(&oset);

    /* Cancelamos y liberamos los hilos que habíamos creado */
    if (pool_th_destroy(hilo, maxClients) == -1){
        printf("Ha habido un error al cancelar y liberar los hilos creados.\n");
        closeServer();
        cfg_free(cfg);
        fclose(log);
        free(hilo);
        return -1;
    }

    /* Liberamos la memoria previamente alocada */
    free(hilo);
    cfg_free(cfg);
    fclose(log);

    /* Cerramos el servidor y terminamos el proceso padre */
    closeServer();

    printf("\nEl servidor se ha cerrado correctamente.\n");
    pthread_exit(NULL);
}
