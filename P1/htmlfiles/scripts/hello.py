'''
 * Script de prueba que recibe un nombre de usuario y responde
 * con la cadena "Hola <<nombre>>!"
 * 
 * Autores: Jose Manuel Freire y Miguel Herrera
 *
'''

import sys

if len(sys.argv) > 1:
    # Cogemos el numero del argumento pasado
    name = sys.argv[1].split("=")

    print("Hola " + name[1] + "!")
else:
    print("No hay usuario al que decirle hola.\n")
