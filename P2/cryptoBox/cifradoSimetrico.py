# Librería para cifrar archivos por medio de una clave simétrica

from Cryptodome.Cipher import AES
from Cryptodome import Random
from Cryptodome.Util.Padding import *
from base64 import b64encode
from cryptoBox.cargaArchivo import *
    
    
# Esta función desencripta un ciphertext con la clave k dada
def aesEncrypt(plaintext, k):
    # Hacemos padding para que el tamaño del plaintext sea múltiplo de 16 bytes 
    plaintext = pad(plaintext, 16)
    # Generamos un IV
    iv = Random.get_random_bytes(16)
    # Creamos un objeto del cifrado
    cipher = AES.new(k, AES.MODE_CBC, iv)
    # Encriptamos el plaintext
    ciphertext = cipher.encrypt(plaintext)
    
    return iv, ciphertext


# Esta función desencripta un ciphertext con la clave k dada
def aesDecrypt(iv, ciphertext, k):
    # Creamos un objeto del cifrado
    cipher = AES.new(k, AES.MODE_CBC, iv)
    # Encriptamos el ciphertext
    plaintext = cipher.decrypt(ciphertext)

    return unpad(plaintext, 16)


# Esta función genera una clave de 32 bytes para el cifrado
def generateRandomKey():
    return Random.get_random_bytes(32)
