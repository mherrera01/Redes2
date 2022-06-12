from aplicacion import *
import os


# Función que copia un archivo de cualquier ruta a la carpeta actual
# Devuelve true si el archivo está en otra ruta y false si está en la carpeta del cliente
# Devuelve también el nombre del archivo
def copyFileToCurrentFolder(path):
    # Revisamos si el archivo existe y en caso de que así sea, lo tratamos
    if (os.path.exists(path)):
        # Guardamos el contenido del archivo
        fileData = readFile(path)
        
        # Separamos la ruta por las /
        routeParts = path.split("/")

        # Comprobamos si el archivo está en la misma ruta que el programa cliente
        if (len(routeParts) == 1):
            filePath = False
        else:
            filePath = True

        # Nos quedamos con el nombre del archivo
        path = routeParts[len(routeParts)-1]

        # Escribimos el archivo en la carpeta local para trabajar con él con más facilidad
        writeFile(path, fileData)

        return path, filePath
    else:
        print("El archivo especificado no existe")
        exit()


# Función que cifra un archivo local
def localEncrypt(originFile, publicKey):
    # Copiamos el archivo desde su ruta a la carpeta local
    originFile, filePath = copyFileToCurrentFolder(originFile)
    
    # Ciframos el archivo con la clave pública del usuario
    cipherFile(originFile, 'cifrado_' + originFile, publicKey)

    # Borramos el archivo copiado
    if (filePath):
        os.remove(originFile)

    print("OK: Archivo " + originFile + " cifrado con éxito en cifrado_" + originFile)


# Función que firma un archivo local
def localSign(originFile):
    # Copiamos el archivo desde su ruta a la carpeta local
    originFile, filePath = copyFileToCurrentFolder(originFile)

    # Ciframos el archivo con la clave pública del usuario receptor
    signFile(originFile, 'firmado_' + originFile)

    # Borramos el archivo copiado
    if (filePath):
        os.remove(originFile)

    print("OK: Archivo " + originFile + " firmado con éxito en firmado_" + originFile)


# Función que cifra y firma un archivo local
def localEncryptSign(originFile, publicKey):
    # Primero firmamos el archivo
    localSign(originFile)

    # Obtenemos el nombre del archivo que está ya en la carpeta local
    localFile = originFile.split("/")
    originFile = localFile[len(localFile)-1]

    # Ciframos el archivo firmado
    localEncrypt("firmado_" + originFile, publicKey)

    # Borramos el archivo firmado, ya que sólo queremos el firmado + cifrado
    os.remove("firmado_" + originFile)
