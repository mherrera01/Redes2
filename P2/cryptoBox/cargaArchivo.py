# Librería para leer y escribir archivos

# Devuelve el contenido del archivo dado
def readFile(name):
    try:
        with open(name, 'rb') as src_file:
            return src_file.read()
    except FileNotFoundError:
        return None


# Escribe en el archivo dado el contenido pasado como argumento
def writeFile(name, content):
    with open(name, 'wb') as target_file:
        target_file.write(content)
