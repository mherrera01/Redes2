# practica2

Práctica 2 de redes de comunicaciones 2.

El objetivo de la segunda práctica de redes de comunicaciones 2 es crear un cliente para el servicio
securebox en el servidor de la universidad. En éste se tomará en cuenta el correcto cifrado y firma
de los archivos que se suben y descargan.

**Prerequisitos:**  

Se debe tener instalado un intérprete de python.

También se requiere tener instalada la librería pycryptodomex.

También es necesario tener credenciales del servidor vega.ii.uam.es.
Para su posible uso en la corrección facilitamos nuestras credenciales:
- NIA: 396209 | Token: dbc1fBEA529a6437
- NIA: 400916 | Token: 8Ca6F134A975DbdE

**Uso:**

Para usar el programa basta con escribir python3 cliente.py [comando] desde la carpeta en la que se encuentra cliente.py. Los comandos que se pueden usar con nuestra aplicación son:

|Comando| Funcionamiento |
|--|--|
| -h/-\-help | Muestra la ayuda sobre cómo usar el programa. |
| -\-create_id | Recibe como argumentos nombre y mail y un tercer argumento opcional alias, el cual no tiene ninguna utilidad. Crea un usuario para el servidor de SecureBox.|
| -\-search_id | Recibe una cadena y busca todos los usuarios en el servidor con los que coincida esa cadena en el nombre o en el email. No muestra los usuarios con errores. |
| -\-delete_id | Borra la cuenta con la id asociada. Sólo se pueden borrar cuentas creadas por el usuario que las borra.|
| -\-upload | Recibe el nombre de un archivo que se quiere enviar. Cifra y firma el archivo y lo sube al servidor. Necesita un destinatario.|
| -\-dest_id | Designa al destinatario de un archivo. Requiere como argumento su NIA. |
| -\-download | Recibe el identificador de fichero que se quiere descargar. Descarga un archivo del servidor, lo descifra y comprueba la firma. Necesita un emisor. |
| -\-source_id | Designa al emisor de un archivo. Requiere como argumento su NIA. |
| -\-list_files | Lista todos los archivos en el servidor subidos por el usuario. No requiere argumentos. |
| -\-delete_file | Borra el archivo con el identificador dado. Requiere el identificador como argumento. |
| -\-encrypt | Cifra el archivo dado como argumento. |
| -\-sign | Firma el archivo dado como argumento. |
| -\-enc_sign | Firma y cifra el archivo dado como argumento. |

El token se puede cambiar en la línea 9 de cliente.py. En la línea 8 hay un comentario indicando a que NIA pertenece el token puesto, que se puede usar como ayuda para recordar el par NIA/token introducido.

**Ejemplos de uso:**

Para crear un usuario podemos escribir:

$> python3 cliente.py -\-create_id nombre e@mail.com alias

Esto creará un usuario de nombre *nombre*, con email *e@mail.com* y alias *alias*.

Para subir un archivo y que se lo descargue un usuario con NIA 123456:

$> python3 cliente.py -\-upload file.txt -\-dest_id 123456

También se puede escribir la ruta del archivo a subir si está en otra carpeta diferente.

Para descargar un archivo con identificador 3a4cD091 de un usuario con NIA 123456:

$> python3 cliente.py -\-download 3a4cD091 -\-source_id 123456

El archivo se descargará en la carpeta /descargas y se imprimirá en pantalla si la firma ha podido ser verificada.
