from comandos import *
from aplicacion import *
from clienteLocal import *
import sys
import requests
import os

# NIA: 396209
token = 'dbc1fBEA529a6437'


# Obtener clave pública - /users/getPublicKey
def getUserKey(url, authHeader, userID):
    getUserKeyUrl = url + '/users/getPublicKey'

    # Argumentos para la búsqueda de la clave pública
    getUserKeyArgs = {'userID': userID}

    # Buscamos la clave del usuario
    req = requests.post(getUserKeyUrl, headers=authHeader, json=getUserKeyArgs)

    # Devuelve la clave pública del usuario buscado
    response = req.json()

    # Manejamos el error USER_ID1
    if (req.status_code == 401):
        print("ERROR: " + response['description'])
        return None

    return response['publicKey']

# Mensaje de error si no hay ninguna flag en la línea de comandos
if (len(sys.argv) < 2):
    print("-h/--help para ejecutar correctamente el cliente")
    exit()

# Obtenemos los argumentos de la línea de comandos
args = argHandle()

# Url de la que cuelgan todas las funciones de la API
url = 'https://vega.ii.uam.es:8080/api'
# Cabecera con el token requerido para las peticiones a Securebox
authHeader = {'Authorization': 'Bearer ' + token}

# Gestión de identidades
# Registro de usuarios - /users/register
if (args.create_id):
    registerUrl = url + '/users/register'
    
    # Argumentos para el registro de usuarios
    registerArgs = {'nombre': args.create_id[0], 'email': args.create_id[1]}

    # Obtenemos la clave pública del usuario a registrar
    key = getPublicKey()

    # Creamos un par de claves pública/privada si el usuario no las tiene ya
    if (key is None):
        # El usuario no quiere crear las claves
        if keysNotDefined():
            print('No se pudo registrar al usuario porque no tiene clave pública')
            exit()
        key = getPublicKey()

    # Guardamos la clave pública del usuario en los argumentos
    registerArgs['publicKey'] = key.decode('utf-8')

    # Registramos al usuario
    req = requests.post(registerUrl, headers=authHeader, json=registerArgs)

    # Devuelve el id de usuario correspondiente a su NIA y la marca de tiempo del registro
    response = req.json()

    print("OK: Usuario con id " + response['userID'] +
          " creado con éxito en el instante " + str(response['ts']))

    # En caso de que el usuario haya introducido un alias
    if (len(args.create_id) == 3):
        print("Alias: " + args.create_id[2])

# Búsqueda de usuarios - /users/search
elif (args.search_id):
    searchUserUrl = url + '/users/search'

    # Argumentos para la búsqueda de usuarios
    searchUserArgs = {'data_search': args.search_id[0]}

    # Buscamos al usuario
    req = requests.post(searchUserUrl, headers=authHeader, json=searchUserArgs)

    # Devuelve el usuario buscado: userID, nombre, email, publicKey y ts
    response = req.json()

    # Manejamos el error USER_ID2
    if (len(response) == 0):
        print("ERROR: No se ha encontrado ningún usuario con los datos proporcionados")
        exit()

    print("OK: Se han encontrado " + str(len(response)) +
          " usuario(s) que coinciden con " + args.search_id[0])
    
    errorUsers = 0

    for i in range(len(response)):
        # Comprobamos si hay algún error en los datos del usuario encontrado
        if (any(userData is None for userData in response[i].values())):
            errorUsers += 1
            continue

        print('[' + str(i+1-errorUsers) + '] ' + response[i]['nombre'] + ', ' +
              response[i]['email'] + ', ID: ' + response[i]['userID'])

    if (errorUsers != 0):
        print("Se han encontrado " + str(errorUsers) + " usuario(s) con errores en los datos")

# Borrado de usuarios - /users/delete
elif (args.delete_id):
    deleteUserUrl = url + '/users/delete'

    # Argumentos para la eliminación de usuarios
    deleteUserArgs = {'userID': args.delete_id[0]}

    # Borramos el usuario
    req = requests.post(deleteUserUrl, headers=authHeader, json=deleteUserArgs)

    # Devuelve el id del usuario borrado
    response = req.json()

    # Manejamos el error USER_ID1
    if (req.status_code == 401):
        print("ERROR: " + response['description'])
        exit()

    print("OK: Usuario con id " + response['userID'] + " borrado con éxito")

# Gestión de ficheros
# Subida de ficheros - /files/upload
elif (args.upload):
    # Debe estar especificado en la flag --dest_id el usuario al que mandar el fichero
    if (args.dest_id == None):
        print('Para subir un fichero debes especificar a qué usuario se lo mandas con --dest_id <id>')
        exit()

    uploadUrl = url + '/files/upload'

    # Obtenemos la clave pública del usuario receptor
    publicKey = getUserKey(url, authHeader, args.dest_id[0])
    if (publicKey is None):
        print('No se ha podido obtener la clave pública del usuario con id ' + args.dest_id[0])
        exit()

    # Copiamos el archivo desde su ruta a la carpeta local
    # filePath devuelve True si el archivo a subir está en una ruta diferente al programa cliente, False en caso contrario
    args.upload[0], filePath = copyFileToCurrentFolder(args.upload[0])

    # Firmamos el archivo con la clave privada del usuario emisor
    if signFile(args.upload[0], 'firmado_' + args.upload[0]):
        print('No se ha podido firmar el archivo ' + args.upload[0])

        # Borramos el archivo copiado
        if (filePath):
            os.remove(args.upload[0])
        exit()

    # Ciframos el archivo con la clave pública del usuario receptor
    cipherFile('firmado_' + args.upload[0], 'cifrado_firmado_' + args.upload[0], publicKey)

    # Leemos el archivo cifrado y firmado
    file = readFile('cifrado_firmado_' + args.upload[0])
    if (file == None):
        print('No se ha podido leer el archivo cifrado y firmado')

        # Borramos el archivo copiado
        if (filePath):
            os.remove(args.upload[0])

        # Borramos el archivo firmado
        if os.path.exists('firmado_' + args.upload[0]):
            os.remove('firmado_' + args.upload[0])
        else:
            print("El archivo firmado_" + args.upload[0] + " no existe")
        exit()

    # Argumentos del fichero a subir
    fileArgs = {'ufile': (args.upload[0], file)}

    # Subimos el archivo al servidor
    req = requests.post(uploadUrl, headers=authHeader, files=fileArgs)

    # Devuelve el identificador del archivo subido y su tamaño
    response = req.json()

    # Borramos el archivo copiado
    if (filePath):
        os.remove(args.upload[0])

    # Borramos el archivo firmado
    if os.path.exists('firmado_' + args.upload[0]):
        os.remove('firmado_' + args.upload[0])
    else:
        print("El archivo firmado_" + args.upload[0] + " no existe")
        exit()

    # Borramos el archivo cifrado
    if os.path.exists('cifrado_firmado_' + args.upload[0]):
        os.remove('cifrado_firmado_' + args.upload[0])
    else:
        print("El archivo cifrado_firmado_" + args.upload[0] + " no existe")
        exit()

    # Manejamos los errores FILE1, FILE2, FILE3
    if (req.status_code == 401 or req.status_code == 403):
        print("ERROR: " + response['description'])
        exit()

    print("OK: Archivo " + args.upload[0] + " con id " + response['file_id'] +
          " y tamaño " + str(response['file_size']) + " bytes subido con éxito")

# Descarga de ficheros - /files/download
elif (args.download):
    # Debe estar especificado en la flag --source_id el usuario del que descargar el fichero
    if (args.source_id == None):
        print('Para descargar un fichero debes especificar de qué usuario lo recibes con --source_id <id>')
        exit()

    downloadUrl = url + '/files/download'

    # Argumentos para la descarga de ficheros
    downloadArgs = {'file_id': args.download[0]}

    # Descargamos el fichero
    req = requests.post(downloadUrl, headers=authHeader, json=downloadArgs)

    # Manejamos los errores FILE2
    if (req.status_code == 401):
        print("ERROR: " + req.json()['description'])
        exit()

    # Obtenemos el nombre del fichero descargado
    fileName = req.headers['Content-Disposition'].split('"')[1]

    # Devuelve el contenido del fichero
    writeFile("cifrado_firmado_recibido", req.content)

    # Creamos una carpeta donde guardar el archivo descargado
    if not os.path.exists('descargas'):
        os.makedirs('descargas')
    fileName = 'descargas/' + fileName

    # Descifra el archivo recibido con la clave privada del usuario receptor
    decipherFile("cifrado_firmado_recibido", fileName, getPrivateKey())

    # Obtenemos la clave pública del usuario emisor
    publicKey = getUserKey(url, authHeader, args.source_id[0])
    if (publicKey is None):
        print('No se ha podido obtener la clave pública del usuario con id ' + args.source_id[0])

        # Borramos el archivo original recibido
        if os.path.exists('cifrado_firmado_recibido'):
            os.remove('cifrado_firmado_recibido')
        else:
            print("El archivo cifrado_firmado_recibido no existe")
        exit()

    # Comprobamos la firma del archivo recibido ya descifrado
    if (verifyFile(fileName, publicKey)):
        print("El archivo ha sido verificado satisfactoriamente")
    else:
        print("WARNING: No se ha podido verificar la firma, el archivo puede haber sido manipulado")

    # Borramos el archivo original recibido
    if os.path.exists('cifrado_firmado_recibido'):
        os.remove('cifrado_firmado_recibido')
    else:
        print("El archivo cifrado_firmado_recibido no existe")
        exit()
    
    print("OK: Archivo " + fileName.split('/')[1] + " descargado con éxito en la carpeta descargas")

# Listado de ficheros - /files/list
elif (args.list_files):
    listFilesUrl = url + '/files/list'

    # Obtenemos la lista de ficheros pertenecientes al usuario
    req = requests.post(listFilesUrl, headers=authHeader)

    # Devuelve la lista de los id de los ficheros y el número total de ficheros
    response = req.json()

    # Comprobamos si el usuario posee algún fichero
    if (response['num_files'] == 0):
        print('El usuario no tiene asociado ningún fichero')
        exit()

    print("OK: El usuario posee " + str(response['num_files']) + " fichero(s)")

    for i in range(response['num_files']):
        print('[' + str(i+1) + '] ' + response['files_list'][i]['fileName'] +
              ', ID: ' + response['files_list'][i]['fileID'])

# Borrado de ficheros - /files/delete
elif (args.delete_file):
    deleteFileUrl = url + '/files/delete'

    # Argumentos para la descarga de ficheros
    deleteFileArgs = {'file_id': args.delete_file[0]}

    # Borramos el fichero
    req = requests.post(deleteFileUrl, headers=authHeader, json=deleteFileArgs)

    # Manejamos los errores FILE2
    if (req.status_code == 401):
        print("ERROR: " + req.json()['description'])
        exit()

    # Devuelve el id del fichero borrado
    print("OK: Fichero con id " + req.json()['file_id'] + " borrado con éxito")

# Cifrado de fichero local
elif (args.encrypt):
    # Debe estar especificado en la flag --dest_id el usuario con el que cifrar el fichero
    if (args.dest_id == None):
        print('Para cifrar un fichero debes especificar el usuario con --dest_id <id>')
        exit()

    # Obtenemos la clave pública del usuario con el que vamos a cifrar el fichero
    publicKey = getUserKey(url, authHeader, args.dest_id[0])
    if (publicKey is None):
        print('No se ha podido obtener la clave pública del usuario con id ' + args.dest_id[0])
        exit()

    # Ciframos el fichero local
    localEncrypt(args.encrypt[0], publicKey)

# Firma de fichero local
elif (args.sign):
    # Firmamos el fichero local
    localSign(args.sign[0])

# Cifrado y firma de fichero local
elif (args.enc_sign):
    # Debe estar especificado en la flag --dest_id el usuario con el que cifrar el fichero
    if (args.dest_id == None):
        print('Para cifrar un fichero debes especificar el usuario con --dest_id <id>')
        exit()

    # Obtenemos la clave pública del usuario con el que vamos a cifrar el fichero
    publicKey = getUserKey(url, authHeader, args.dest_id[0])
    if (publicKey is None):
        print('No se ha podido obtener la clave pública del usuario con id ' + args.dest_id[0])
        exit()

    # Ciframos y firmamos el fichero local
    localEncryptSign(args.enc_sign[0], publicKey)
