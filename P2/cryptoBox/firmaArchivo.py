from Cryptodome.Hash import SHA256
from Cryptodome.Signature import pkcs1_15
from Cryptodome.PublicKey import RSA


# Firma los datos que recibe con la clave privada.
def signData(data, private_key):
    # Hacemos el hash de los datos.
    hashed = SHA256.new(data)

    # Importamos la clave dada.
    key = RSA.import_key(private_key)    

    # Creamos un objeto que firme con la clave privada.
    signer = pkcs1_15.new(key)

    # Firmamos los datos y devolvemos la firma.
    return signer.sign(hashed)


# Revisa que los datos recibidos estén firmados con la clave privada pareja a la clave pública dada.
def checkSignature(data, public_key, signature):
    hashed = SHA256.new(data)

    # Importamos la clave dada.
    key = RSA.import_key(public_key)

    # Creamos un objeto para verificar la firma.
    signer = pkcs1_15.new(key)

    # Verificamos que la firma sea la correcta y devolvemos True en caso de que lo sea.
    try:
        signer.verify(hashed, signature)
    except ValueError:
        return False
    return True
