#include <stdio.h>
#include <pthread.h> /* Librería para los hilos POSIX. Compilación con la flag -pthread */
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

#include "../includes/pool_thread.h"

int pool_th_ini(pthread_t *hilo, threadRoutine_type func, int num_threads, void *args){
    pthread_attr_t attr;
    long i;
    int error = 0;

    /* Control de error de argumentos */
    if (hilo == NULL) return -1;

    /**
     * Inicializamos la estructura attr que define atributos del hilo que queremos crear
     *
     * Esta memoria debe ser liberada después con pthread_attr_destroy
     */
    if (pthread_attr_init(&attr) != 0){
        printf("Hubo un error al inicializar la estructura de atributos del hilo.\n");
        return -1;
    }

    /**
     * Configuramos si queremos que el hilo que creamos esté vinculado con su padre o no.
     * Por defecto pthread_create crea el hilo vinculado a su padre
     *
     * - Hilo vinculado a su padre (requiere pthread_join después): PTHREAD_CREATE_JOINABLE 
     * - Hilo no vinculado a su padre (puede terminar cuando quiera ya que nadie espera su
     * resultado): PTHREAD_CREATE_DETACHED
     */
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0){
        printf("Hubo un error al configurar el atributo de vinculación a la estructura.\n");
        pthread_attr_destroy(&attr);
        return -1;
    }

    /* Creamos tantos hilos que queramos y guardamos sus identificadores en un array estático */
    for (i=0; i<num_threads; i++){
        /**
         * Para crear un hilo con pthread_create hay que especificar cuatro campos
         * 
         * - Variable donde se va a guardar el identificador del hilo
         * - Atributos adicionales del hilo (como su vinculación). Por defecto es NULL
         * - La rutina que ejecuta el hilo una vez creado
         * - Variable que se pasa a la rutina asociada al hilo. NULL no pasa argumentos
         */
        error = pthread_create(&hilo[i], &attr, func, args);
        if (error != 0){
            fprintf(stderr, "Hubo un error al crear el hilo %ld: %s\n", i+1, strerror(error));
            pthread_attr_destroy(&attr);
            return -1;
        }
    }

    /* Liberamos memoria de la estructura attr */
    if (pthread_attr_destroy(&attr) != 0){
        printf("Hubo un error al liberar la estructura de atributos del hilo.\n");
        return -1;
    }

    return 0;
}

int pool_th_destroy(pthread_t *hilo, int num_threads){
    long i;

    /* Control de error de argumentos */
    if (hilo == NULL) return -1;

    for (i=0; i<num_threads; i++){
        /**
         * Hacemos terminar a los hilos
         * 
         * Se puede definir si se puede cancelar un hilo o no con
         * la función pthread_setcancelstate (PTHREAD_CANCEL_DISABLE o
         * PTHREAD_CANCEL_ENABLE). Por defecto se pueden cancelar
         */
        if (pthread_cancel(hilo[i]) != 0){
            printf("Hubo un error al cancelar el hilo %ld.\n", i+1);
            return -1;
        }

        /**
         * Como los hilos está vinculados al padre, con pthread_join 
         * esperamos a que terminen para recoger sus resultados y así
         * éstos puedan liberar sus recursos asociados
         */
        if (pthread_join(hilo[i], NULL) != 0){
            printf("Hubo un error al esperar por el hilo %ld.\n", i+1);
            return -1;
        }
    }

    return 0;
}

/* Manejador vacío para desbloquear el hilo principal al recibir la señal */
void manejador(int sig) {
    printf("\nSe ha recibido la señal %d.\n", sig);
    fflush(stdout);
}

int thread_block(int signal, sigset_t *oset){
    sigset_t set;
    struct sigaction act;

    /* Control de error de argumentos */
    if (oset == NULL || signal < 0) return -1;

    /* Inicializamos un set de señales vacío */
    if (sigemptyset(&set) == -1){
        printf("Hubo un error al inicializar el set de señales vacío.\n");
        return -1;
    }

    /* Añadimos al set de señales la señal pasada como argumento */
    if (sigaddset(&set, signal) == -1){
        printf("Hubo un error al añadir la señal %d al set de señales.\n", signal);
        return -1;
    }

    /**
     * Con el parámetro SIG_BLOCK bloqueamos las señales de la unión de la máscara actual
     * y el conjunto pasado como argumento set
     * 
     * La máscara de señales anterior a pthread_sigmask se guarda en oset
     */
    if (pthread_sigmask(SIG_BLOCK, &set, oset) != 0){
        printf("Hubo un error al bloquear la señal %d en los hilos.\n", signal);
        return -1;
    }

    /* Definimos el manejador para la señal */
    act.sa_handler = manejador;
    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;

    /* Armamos la señal con nuestra propia rutina */
    if (sigaction(signal, &act, NULL) < 0) {
        printf("Hubo un error al armar la señal %d con nuestra propia rutina.\n", signal);
        return -1;
    }

    return 0;
}

void daemon_init() {
    pid_t pid, pid2;
    int fd;

    /* Creamos un hijo */
    pid = fork();

    /* Proceso hijo */
    if (pid == 0){
        /* Creamos una nueva sesión con el hijo como líder */
        if (setsid() < 0){
            printf("Ha habido un error creando una nueva sesión para el hijo.\n");
            exit(EXIT_FAILURE);
        }

        /* Hacemos fork de nuevo */
        pid2 = fork();

        /* Proceso del segundo hijo */
        if (pid2 == 0){
            /* Cambiamos los permisos de los ficheros creados por el demonio */
            umask(0);

            /**
             * Cerramos todos los descriptores de ficheros abiertos
             * 
             * Con la función sysconf(_SC_OPEN_MAX) se puede obtener
             * el número máximo de archivos que un proceso puede tener abiertos.
             * Sin embargo, al ser dependiente del sistema y complejo de determinar,
             * hemos decidido fijar 64 descriptores que cerrar como en UNIX 
             * Network Programming (W. Richard Stevens et al.), Sección 13.4.
             */
            for (fd=0; fd<MAX_FD; fd++){
                close(fd);
            }

            /* Redirigimos stdin, stdout y stderr a /dev/null */
            open("/dev/null", O_RDONLY);
            open("/dev/null", O_RDWR);
            open("/dev/null", O_RDWR);

            /* Abrimos un fichero log para manejar los errores del demonio */
            openlog("serverDaemon", LOG_PID, LOG_DAEMON);
            syslog(LOG_INFO, "Proceso demonio creado correctamente.");
            syslog(LOG_INFO, "Traceback de información en el log.txt creado por el programa.");

        /* Error al crear el segundo hijo */
        } else if (pid2 < 0){
            printf("Ha habido un error al crear el segundo hijo.\n");
            exit(EXIT_FAILURE);

        /* Proceso del segundo padre */
        } else if (pid2 > 0){
            /* El proceso del segundo padre termina para que su hijo siga ejecutándose en segundo plano */
            exit(EXIT_SUCCESS);
        }

    /* Error al crear el hijo */
    } else if (pid < 0){
        printf("Ha habido un error al crear el hijo.\n");
        exit(EXIT_FAILURE);

    /* Proceso padre */
    } else if (pid > 0){
        /* El proceso padre termina para que su hijo siga ejecutándose en segundo plano */
        exit(EXIT_SUCCESS);
    }
}
