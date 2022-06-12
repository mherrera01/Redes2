#ifndef POOL_THREAD_H
#define POOL_THREAD_H

#define MAX_FD 64 /* Número máximo de archivos que un proceso puede tener abiertos */

typedef void (*(*threadRoutine_type)(void*));

/**
 * Función que crea num_threads hilos con la rutina y argumentos dados.
 * Guarda los identificadores de los hilos creados en el puntero de
 * tipo pthread_t pasado como argumento
 * 
 * Devuelve 0 en caso de éxito y -1 en caso de error
 */
int pool_th_ini(pthread_t *hilo, threadRoutine_type func, int num_threads, void *args);

/**
 * Función que cancela y libera los recursos (al estar vinculados al padre)
 * de los hilos en la variable de tipo pthread_t
 * 
 * Devuelve 0 en caso de éxito y -1 en caso de error
 */
int pool_th_destroy(pthread_t *hilo, int num_threads);

/**
 * Función que bloquea una señal dada y guarda en la variable
 * oset la máscara de señales anterior al bloqueo. También arma
 * para la señal un manejador vacío, el cual es privado
 * 
 * Devuelve 0 en caso de éxito y -1 en caso de error
 */
int thread_block(int signal, sigset_t *oset);

/* Función que crea un proceso demonio, haciendo que no esté asociado a ninguna terminal */
void daemon_init();

#endif
