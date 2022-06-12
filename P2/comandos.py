import argparse


# Función que comprueba si el número de argumentos está en el rango dado
def required_length(nmin, nmax):

    # Clase que hereda de Action en argparse 
    class RequiredLength(argparse.Action):
        # Función a la que se llama cuando se añade la opción action en add_argument
        def __call__(self, parser, args, values, option_string=None):
            if not nmin<=len(values)<=nmax:
                print('error: argument --' + self.dest + ' requires between ' + str(nmin) +
                      ' and ' + str(nmax) + ' arguments')
                exit()
            setattr(args, self.dest, values)
    return RequiredLength


# Función que añade los argumentos sobre la gestión de usuarios e identidades
def userArguments(parser):
    parser.add_argument('--create_id', nargs='*', type=str, metavar=('nombre', 'email'), action=required_length(2, 3),
                        help="Crea una nueva identidad (par de claves pública y privada) " +
                             "para un usuario con nombre nombre y correo email, y la " +
                             "registra en SecureBox, para que pueda ser encontrada por " +
                             "otros usuarios. alias es una cadena identificativa opcional.")

    parser.add_argument('--search_id', nargs=1, type=str, metavar=('cadena'),
                        help="Busca un usuario cuyo nombre o correo electrónico contenga " +
                             "cadena en el repositorio de identidades de SecureBox, " +
                             "y devuelve su ID.")

    parser.add_argument('--delete_id', nargs=1, type=str, metavar=('id'),
                        help="Borra la identidad con ID id registrada en el sistema. " +
                             "Obviamente, sólo se pueden borrar aquellas identidades " +
                             "creadas por el usuario que realiza la llamada.")


# Función que añade los argumentos sobre la subida y descarga de ficheros
def loadArguments(parser):
    parser.add_argument('--upload', nargs=1, type=str, metavar=('fichero'),
                        help="Envia un fichero a otro usuario, cuyo ID es especificado " +
                             "con la opción --dest_id. Por defecto, el archivo se subirá " +
                             "a SecureBox firmado y cifrado con las claves adecuadas " +
                             "para que pueda ser recuperado y verificado por el destinatario.")

    parser.add_argument('--source_id', nargs=1, type=str, metavar=('id'),
                        help="ID del emisor del fichero.")

    parser.add_argument('--dest_id', nargs=1, type=str, metavar=('id'),
                        help="ID del receptor del fichero.")

    parser.add_argument('--list_files', action='store_true',
                        help="Lista todos los ficheros pertenecientes al usuario.")

    parser.add_argument('--download', nargs=1, type=str, metavar=('id_fichero'),
                        help="Recupera un fichero con ID id_fichero del sistema (este ID " +
                             "se genera en la llamada a upload, y debe ser comunicado al " +
                             "receptor). Tras ser descargado, debe ser verificada la firma " +
                             "y, después, descifrado el contenido.")

    parser.add_argument('--delete_file', nargs=1, type=str, metavar=('id_fichero'),
                        help="Borra un fichero del sistema.")


# Función que añade los argumentos sobre el cifrado y la firma de ficheros local
def fileArguments(parser):
    parser.add_argument('--encrypt', nargs=1, type=str, metavar=('fichero'),
                        help="Cifra un fichero, de forma que puede ser descifrado por otro " +
                             "usuario, cuyo ID es especificado con la opción --dest_id.")

    parser.add_argument('--sign', nargs=1, type=str, metavar=('fichero'),
                        help="Firma un fichero.")

    parser.add_argument('--enc_sign', nargs=1, type=str, metavar=('fichero'),
                        help="Cifra y firma un fichero, combinando funcionalmente las " +
                             "dos opciones anteriores.")


def argHandle():
    # Creamos el argument parser
    parser = argparse.ArgumentParser(description='Interacción del cliente con el servidor SecureBox.')

    # Añadimos los argumentos con la funcionalidad del cliente
    userArguments(parser)
    loadArguments(parser)
    fileArguments(parser)

    # Inspeccionamos la línea de comandos y convertimos cada argumento correspondientemente
    args = parser.parse_args()

    return args
