from cryptoBox.cifradoAsimetrico import *
from cryptoBox.firmaArchivo import *
from cryptoBox.cargaArchivo import *
from cryptoBox.cifradoSimetrico import *


# Esta función pregunta al usuario si quiere generar las claves pública y privada. En caso afirmativo las crea
def keysNotDefined():
    print("Las claves pública y privada no están definidas.\n¿Quieres crear unas nuevas? [y/n]")
    rcv = input()[0]

    if(rcv.capitalize() == "Y"):
        # Generamos las claves pública y privada
        generateKeyPair()
        return 0
    elif(rcv.capitalize() == "N"):
        return 1
    else:
        # Volvemos a solicitar al usuario una respuesta
        return keysNotDefined()


# Esta función firma un archivo origen en un archivo destino
def signFile(ori, dst):
    key = getPrivateKey()

    # En caso de que no haya clave privada, preguntamos al usuario si quiere generarla junto con la clave pública
    if(key == None):
        if(keysNotDefined() == 1):
            print("No hay clave privada")
            return 1
        else:
            key = getPrivateKey()

    # Leemos el archivo origen que queremos firmar
    data = readFile(ori)

    if(data == None):
        print("No se pudo abrir el archivo especificado.")
        return 1

    # Firmamos los datos leídos del archivo
    signature = signData(data, key)

    # Escribimos en el archivo destino la concatenación de la firma y los datos
    writeFile(dst, signature+data)
    return 0


# Esta función lee un archivo dado, verifica su firma con la clave dada y después sobreescribe
# el archivo sin la firma
def verifyFile(ori, key):
    # Leemos el archivo del que queremos verificar la firma
    data = readFile(ori)

    # Los primeros 256 bytes corresponden a la firma mientras que el resto a los datos del archivo
    verified = checkSignature(data[256:], key, data[:256])

    # Sobreescribimos el archivo sin la firma
    writeFile(ori, data[256:])

    #Devolvemos si el archivo ha sido verificado con éxito o no
    return verified


# Cifra un archivo simétricamente y luego cifra la clave asimétricamente
def cipherFile(ori, dst, publicKey):
    # Leemos el archivo que queremos cifrar
    data = readFile(ori)

    # Creamos una clave simétrica
    symmetricKey = Random.get_random_bytes(16)

    # Ciframos el archivo y devolvemos el contenido y el iv
    iv, ciphertext = aesEncrypt(data, symmetricKey)

    # Ciframos la clave simétrica con rsa
    encryptedKey = rsaEncrypt(symmetricKey, publicKey)

    # Guardamos el contenido cifrado junto con el IV y la clave cifrada en el archivo
    writeFile(dst, iv+encryptedKey+ciphertext)
    return 0


# Descifra un archivo que contiene su clave simétrica cifrada con RSA
def decipherFile(ori, dst, privateKey):
    # Leemos el archivo cifrado
    data = readFile(ori)

    # Separamos las distintas partes del archivo
    iv = data[:16]
    encryptedKey = data[16:272]
    ciphertext = data[272:]

    # Desencriptamos la clave simétrica, usando la clave asimétrica
    symmetricKey = rsaDecrypt(encryptedKey, privateKey)
    
    # Con la clave simétrica desciframos el contenido del archivo
    plaintext = aesDecrypt(iv, ciphertext, symmetricKey)

    # Guardamos el contenido en el archivo de destino
    writeFile(dst, plaintext)
    return 0
