# practica1

Práctica 1 de redes de comunicaciones 2.
El objetivo de la primera práctica del curso es diseñar e implementar en lenguaje C un servidor Web, con algunas limitaciones, pero plenamente funcional.

**Prerequisitos:**

Se debe tener instalada la librería confuse.h encargada de leer el archivo de configuración del servidor.

$> sudo apt-get install -y libconfuse-dev

También se debe tener instalado python y php.
Python no soporta tildes, a no ser que la versión por defecto sea Python3 (esto es cuando el comando python llama a python3)

**Compilación y ejecución:**

El programa trae un Makefile que automáticamente elimina el ejecutable, los .o y .a que estén ya generados, y los vuelve a crear en sus respectivos directorios.
Solo hace falta ejecutar el siguiente comando:

$> make

Se crean dos carpetas /lib y /obj las cuales contienen los siguientes archivos generados por el Makefile:
- /lib: Librerías estáticas socketlib.a y picolib.a
- /obj: .o necesarios para la compilación del servidor

Se crea también el ejecutable server.
Ejecución sin valgrind:

$> make run_s

Ejecución con valgrind:

$> make runv_s

El comando "make" ya elimina los archivos anteriormente generados para crearlos de nuevo, pero si se quiere se puede hacer manualmente:

$> make clean

**Funcionamiento del programa:**

Al ejecutar el servidor se crea el archivo log.txt que guarda el traceback de lo que ocurre durante la ejecución (errores, información...).

Para conectarse al servidor, se debe entrar como <ip:puerto/index.html> donde ip y puerto están configurados en server.conf.

Para crear el servidor como proceso demonio se debe cambiar la variable server_daemon en el server.conf:
- 0: NO es un proceso demonio.
- 1: SÍ es un proceso demonio.

Por defecto está en 0, ya que así se puede cerrar fácilmente el servidor con Ctrl + C y se pueden introducir datos por STDIN.
