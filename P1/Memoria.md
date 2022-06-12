**Autores:**

José Manuel Freire Porras y Miguel Herrera Martínez

Grupo 2391 - Pareja 11

# Aviso

Nada de esta práctica se ha cambiado desde la entrega anterior.

# Introducción

En esta memoria se van a exponer las librerías que hemos decidido implementar para hacer más modular el código, al igual que el funcionamiento del programa servidor. También se abordará la razón por la que nos hemos decantado por un servidor concurrente con pool de hilos y cómo está integrado a alto nivel con los otros módulos.

El objetivo no es explicar cada función en el código ya que están todas detalladamente documentadas. Aunque sí que se incluirán las decisiones de diseño y los problemas que tuvimos durante la realización de la práctica y se terminará con las conclusiones que hemos sacado de la misma.

# Tipo de Servidor

El tipo de servidor elegido tiene que ser concurrente para así aumentar la eficiencia del servidor, ya que puede responder varias peticiones de clientes a la vez. El número de peticiones concurrentes que puede soportar el servidor se especifica en la variable *max_clients* del archivo de configuración server.conf. Descartamos un servidor asíncrono ya que la base de las peticiones HTTP es que se mande una solictud que requiere de una respuesta.

Entonces la discusión sobre el tipo de servidor que se debe escoger recae en si es mejor hacer un pool de hilos o que cada vez que llegue una petición se cree un hilo. Hemos decidido implementar el servidor con un pool porque es más eficaz. Esto se debe a que los hilos se crean todos al principio, por lo que solo va a repercutir cuando se ejecute el servidor. La idea es que los servidores que mantienen una web no se cierran casi nunca, ya que tienen que estar trabajando todo el día siempre a la espera de un cliente que quiera acceder. Crear un hilo por petición supondría que durante toda la ejecución del servidor se tienen que manejar los hilos.

En cuanto a por qué hilos y no procesos (con fork), la creación de procesos es más lenta en comparación con los hilos. Esto es lógico ya que el sistema no inicializa una zona de memoria ni un entorno para los hilos, siendo éstos procesos ligeros. En nuestro caso los *pthreads* nos vienen perfecto para la implementación del servidor.

# Librerías Implementadas

Hemos implementado dos librerías, una de sockets y otra de creación y manejo de hilos. Lo que contienen ambas más especificamente es lo siguiente:
- socket: Aunque se incluyen funciones para manejar un socket cliente éstas no nos interesan y se desarrollaron en fases tempranas de la práctica cuando aún no usábamos el navegador. A parte, esta librería permite crear un servidor dados una ip, puerto y número de peticiones concurrentes que aceptar. También se puede mandar mensajes y recibirlos abriendo nuevos sockets cuando se acepta una conexión con un cliente. Cerrar el servidor libera todos los recursos alocados y no requiere del descriptor del socket a cerrar porque lo maneja la librería globalmente.

- pool_thread: Esta librería crea los hilos que se quieran y los cancela apropiadamente liberando sus recursos. También incluye una función que bloquea un proceso dada una señal (en nuestro caso Interrupt from Keyboard), aunque tiene que ser el servidor quien se encarge de hacer *sigsuspend*. Por último, se ha  implementado una función que crea un proceso demonio, que a pesar de no estar complementamente relacionada con esta librería, creemos que debía estar aquí.

**¿Cómo son las librerías utilizadas por el programa servidor?**

Una vez que el programa lee el archivo de configuración server.conf, crea un proceso demonio si es necesario e inicializa el servidor haciendo uso de la librería de sockets. Se aloca memoria para los hilos y se crean pasando la rutina (que implementa el servidor) que va a ejecutar cada hilo. Esta rutina tendrá el accept, que es bloqueante ya que espera una petición del cliente. Finalmente, bloqueamos la señal que queramos y con el sigsuspend nos quedamos a la espera de que se cierre el servidor con dicha señal. Una vez cerrado, se liberan los hilos usando la librería pool_thread y todos los recursos que se hubieran alocado en el main.

En cuanto al receive de los sockets éste se utiliza en la función parseHttpRequest que es llamada por la rutina del hilo para guardar en la estructura *cliente_request* la petición http que el cliente ha mandado. Después, la rutina llama a la función serverResponse que se encarga de procesar la petición y utiliza el send de los sockets para responder de vuelta al cliente. En caso de que la petición fuera un script, la función serverResponse llama a executeScript, que manejará los argumentos del script y devolverá la tubería de dónde leer el resultado.

# Decisiones de Diseño

**Mandar la respuesta al cliente**

En la librería de los sockets cuando mandamos con la función send la respuesta a la petición del cliente, tenemos que hacer uso de la flag *MSG_NOSIGNAL* para evitar que la señal SIGPIPE termine el programa abruptamente. Esto se debe a un bug que está descrito en el libro "Unix Network Programming" by Richard Stevens en la sección 5.13.

> When a process writes to a socket that has received an RST, the SIGPIPE signal is sent to the process. The default action of this signal is to terminate the process, so the process must catch the signal to avoid being involuntarily terminated.

Se supone que el socket ya está cerrando cuando se envía la respuesta y por lo tanto surge el mensaje de tubería rota. Aunque este error nos apareció en muy pocas ocasiones al probar el programa, hemos decidido manejarlo correctamente.

**Ejecución de los scripts**

Al ejecutar los scripts utilizamos dos tuberías que actúan como una conexión bidireccional entre dos procesos, el padre y el hijo creado con fork. Esto supone un problema, y es que estamos haciendo un fork dentro de un thread. Las implicaciones que esto conlleva las podemos encontrar [aquí](https://stackoverflow.com/questions/6078712/is-it-safe-to-fork-from-within-a-thread). Entre lo que nos interesa, no podemos usar semáforos para hacer que el hijo (el que ejecuta el script), espere primero a que el padre detecte datos en STDIN. Por ello, la solución a la que llegamos es utilizar un *sleep* en el proceso hijo. En total cuando se ejecuta un script se deben esperar 6 segundos, siendo 5 segundos el tiempo que se tiene para escribir algo en la terminal y que el script lo detecte como STDIN.

Es importante remarcar que si se ejecuta el servidor como un proceso demonio, al no tener una terminal asociada, no se pueden mandar datos por STDIN al script.

**Proceso demonio**

El proceso demonio debe estar al principio del programa servidor, justo después de leer el archivo de configuración server.conf. Esto se debe a que si abrimos el servidor antes, el descriptor del socket no se configura correctamente y el navegador no puede acceder al servidor.

En cuanto a la creación del proceso demonio hemos decidido no incluir el siguiente código:
```
/* Cambiamos el directorio de trabajo a la localización '/' */
if (chdir("/") == -1){
    printf("Ha habido un error al cambiar el directorio de trabajo para el hijo demonio.\n");
    exit(EXIT_FAILURE);
}
```

No podemos cambiar el directorio del proceso demonio porque no tendría acceso a los recursos de la web en la carpeta /htmlfiles.

A pesar de que en el syslog del sistema encontraremos que el proceso demonio se ha creado correctamente, hemos decidido que todos los mensajes de error e información generados durante la ejecución del programa se impriman en un archivo llamado log.txt. Cada vez que se recibe una petición, ésta se muestra en el archivo ya que imprimirlo en el syslog sería erroneo. Por lo tanto,  sea un proceso demonio o no, favorecemos la lectura del traceback de la ejecución del servidor.

Para acabar con el proceso demonio correctamente tendríamos que hacer uso de los siguientes comandos:
```
# Vemos el pid del proceso que está ejecutando ./server
$> ps -aux

# Mandamos la señal 2 (Interrupt from Keyboard) para cerrar correctamente el servidor.
$> kill -2 <pid>
```

# Conclusión

Como conclusión, a pesar de que nos llevó varias horas implementar la práctica, fueron bastante llevaderas. Esto seguramente se debe a que personalmente encontramos el lenguaje de programación C agradable de usar, a parte de muy adecuado para esta práctica ya que necesitamos programar a relativamente bajo nivel. Sin embargo, al estar involucrado el manejo de procesos e hilos, se hacía bastante costoso el debug ya que es más complejo de comprender lo que está pasando en un momento concreto de la ejecución. Esto nos dio problemas al principio ya que teníamos conflictos con los descriptores de los sockets entre los hilos del servidor. Aún así el uso de valgrind nos ayudó bastante y creemos interesante el objetivo de la práctica.
