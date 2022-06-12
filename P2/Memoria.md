**Autores:**

José Manuel Freire Porras y Miguel Herrera Martínez

Grupo 2391 - Pareja 11

# Aviso

Nada de esta práctica se ha cambiado desde la entrega anterior.

# Introducción

En esta wiki se explicará los aspectos relevantes del desarrollo de esta segunda práctica sobre el cliente de securebox. La práctica consistía en la creación de un cliente para el servidor securebox que nos daban, haciendo uso de su API. Nuestro cliente permite cifrar, descifrar, firmar y verificar archivos, así como gestión de identidades y de claves.

En esta memoría no se explicará cómo usar la aplicación, ya que el readme tiene esa función, ni se entrará en detalle sobre qué hace cada función del código, ya que todas ellas están debidamente comentadas.

# Librerías Implementadas
Hemos implementado varias librerías que ayudan a tener un código más modular, aunque muchas de ellas tienen dependencias de otras. Cuatro de ellas (cargaArchivo, cifradoAsimetrico, cifradoSimetrico, firmaArchivo) forman parte de cryptoBox, ya que su funcionalidad está muy relacionada y ponerlas en una carpeta ayudaba a mantener el orden entre los archivos del programa.
- cargaArchivo: Esta librería se encarga de cargar los archivos para leerlos o escribirlos.
- cifradoAsimetrico: Esta librería tiene todas las funciones de RSA para cifrar y además tiene la creación del par de claves publica/privada y la obtención de dichas claves para ser usadas en otras partes del programa.
- firmaArchivo: Esta librería se encarga de la firma digital de los archivos.
- cifradoSimetrico: Esta librería se encarga del cifrado por medio de AES.
- comandos: Esta librería se encarga de la lectura de los argumentos dados al programa mediante argparse.
- aplicacion: Esta librería se encarga de unificar las cuatro librerías cryptoBox para hacer sus funciones más opacas de cara al cliente. Contiene las funciones para firmar, verificar la firma, crear claves, cifrar y descifrar.
- clienteLocal: Esta librería se encarga de atender las peticiones del cliente para los servicios locales, como cifrar o firmar un archivo en sin subirlo al servidor. Fue creada para evitar un código demasiado largo en cliente.py.

El cliente ejecutable es cliente.py que se encuentra en la carpeta principal y es el que llama a todas las librerías. Pese al nombre de clienteLocal.py, este no es un cliente y no puede ser ejecutado directamente. La forma de ejecutarse es python3 cliente.py [argumentos] (más detalladamente explicado en el readme).

**¿Cómo son las librerías utilizadas por el programa?**

Al subir un archivo al servidor el programa primero revisará si el par de claves pública/privada existen, y en caso afirmativo, firmará el archivo con la clave privada del emisor, lo cifrará con el cifrado simétrico AES y la clave del AES la cifrará con la clave pública del receptor. Después compondrá el paquete formado por la IV del AES, la clave simétrica cifrada y el mensaje cifrado y firmado y lo subirá al servidor usando la API.

Al descargar un archivo del servidor, la clave simétrica se obtiene descifrando con la clave privada la parte correspondiente del mensaje. Junto con la IV y la clave simétrica se descifra el archivo. Por último se comprueba con la clave pública del emisor la firma digital y se informa de si el resultado ha sido el esperado o no.

Al crear o borrar un usuario, borrar un archivo, buscar un usuario o listar los archivos subidos, se usan únicamente las funciones de la API a través de la librería requests.

# Decisiones de Diseño

**Posibilidad de usar archivos con cualquier ruta**

Para que se puedan utilizar archivos en cualquier parte de la máquina desde la que se ejecuta el cliente, dejamos que sea el sistema operativo el que trate la ruta. Si el archivo que se quiere compartir se encuentra en la carpeta principal de nuestra aplicación, se puede simplemente dar su nombre como ruta, y se ha de poner la ruta completa si estuviera en otra parte del sistema.
Si el archivo estuviese fuera de la carpeta principal de la aplicación, el programa crearía una copia en la carpeta de la aplicación para trabajar con él que después borraría.

**Ocultación de los usuarios con datos erróneos**

Durante las pruebas nos dimos cuenta de que había muchos usuarios con datos malformados, como sin correo electrónico o sin nombre. En estos casos en vez de mostrar un mensaje indicando que sus datos están mal para cada uno de ellos, notificamos al usuario de cuántos usuarios hemos encontrado con sus datos incorrectamente formateados, ya que consideramos que el usuario preferiría no recibir información errónea o incompleta.

**Argumento opcional para argparse**

Al no dotarnos argparse de la posibilidad de añadir argumentos opcionales, tuvimos que ingeniárnoslas para conseguir que aceptara 2 o 3 argumentos al hacer create_id. Primero buscamos si esto se podía hacer de alguna forma directa con argparse y vimos que no (https://bugs.python.org/issue11354). Intentamos hacer soluciones por nuestra cuenta como poner el mismo comando pero con dos posibles cantidades de argumentos, pero como no funcionaba, optamos por el post processing. Encontramos una solución en stackoverflow y la adaptamos para nuestro programa (https://stackoverflow.com/questions/4194948/python-argparse-is-there-a-way-to-specify-a-range-in-nargs). Consiste en cambiar el action para que el comando que pueda recibir distinto número de argumentos y de esta forma hacer el post processing con el número de argumentos que queremos. En el comando ponemos que la cantidad de argumentos que recibe es * (cualquiera) y nos encargamos de revisar que sean 2 o 3 en la action. 

# Conclusión

Esta práctica la hicimos en dos partes: la parte sobre criptografía la terminamos un mes antes de empezar con la API de SecureBox, por lo que tuvimos una pausa en medio a causa de las otras prácticas de otras asignaturas. La primera parte la probamos mucho para asegurarnos de que no hubiera errores cuando empezásemos con la segunda y es una medida que nos ha ayudado durante toda la práctica, sabiendo que la parte de criptografía no tenía errores cada vez que teníamos un error en otro campo, como la forma de la que subíamos nuestra clave pública al servidor. Esta práctica ha sido bastante entretenida de hacer para nosotros, ya que la seguridad es un tema que nos interesa mucho y la criptografía es una parte vital de ella. También es divertido tener una práctica que se pueda usar en conjunto a las de otros compañeros y es la primera que hacemos así en todo nuestro tiempo en la universidad.