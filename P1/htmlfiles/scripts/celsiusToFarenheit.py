'''
 * Script de prueba que recibe una temperatura en Celsius y
 * devuelve su equivalente en Farenheit
 * 
 * Autores: Jose Manuel Freire y Miguel Herrera
 *
'''

import sys

if len(sys.argv) > 1:
    # Cogemos el numero del argumento pasado
    cl = sys.argv[1].split("=")

    # Convertimos la temperatura a Farenheit
    fh = float(cl[1]) * 1.8 + 32
    print(cl[1] + " Celsius equivale a " + str(fh) + " Farenheit")
else:
    print("No hay temperatura en Celsius que transformar a Farenheit.\n")
