from Cryptodome.PublicKey import RSA
from Cryptodome.Cipher import PKCS1_OAEP
from Cryptodome.Hash import SHA256
from cryptoBox.cargaArchivo import *
import os


# Encripta el argumento plaintext con la cláve public_key
def rsaEncrypt(plaintext, public_key):
    # Importamos la clave dada
    key = RSA.import_key(public_key)

    # Creamos el objeto del cifrado
    cipher = PKCS1_OAEP.new(key)

    # Encriptamos el plaintext y lo devolvemos
    return cipher.encrypt(plaintext)


# Desencripta el argumento ciphertext con la cláve private_key
def rsaDecrypt(ciphertext, private_key):
    # Importamos la clave dada
    key = RSA.import_key(private_key)

    # Creamos el objeto del cifrado
    cipher = PKCS1_OAEP.new(key)

    # Desencriptamos el ciphertext y lo devolvemos 
    return cipher.decrypt(ciphertext)


# Genera un par de claves: pública y privada
def generateKeyPair():
    # Genera la clave de 2048 bits
    key = RSA.generate(2048)

    # Las exporta
    private_key = key.exportKey('PEM')
    public_key = key.publickey().exportKey('PEM')

    # Creamos una carpeta donde guardar el par de claves
    if not os.path.exists('keys'):
        os.makedirs('keys')

    # Las guarda en dos archivos
    writeFile("keys/private_key.PEM", private_key)
    writeFile("keys/public_key.PEM", public_key)


# Devuelve la clave privada
def getPrivateKey():
    return readFile("keys/private_key.PEM")


# Devuelve la clave pública
def getPublicKey():
    return readFile("keys/public_key.PEM")
