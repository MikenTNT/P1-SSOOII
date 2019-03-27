# MAKEFILE DE PROGRAMA C
# COPYRIGHT Miguel Sanchez Gonzalez JAN-2019.

#----------------------------------VARIABLES------------------------------------

# Compilador de C 'gcc'.
CC = gcc
# CFLAGS: depurador '-g', matematicas '-lm'.
CFLAGS = -m32 -Wall

# SOURCES: archivos fuentes de C.
SOURCES = falonso.c libfalonso.a
# HEADERS: archivos de cabecera de C.
HEADERS = falonso.h

#----------------------------OBJETIVOS PRINCIPALES------------------------------

# Objetivos a ejecutar con el comando make.
all: falonso


# Crear el archivo de ejecución falonso.
falonso: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o falonso $(SOURCES)

#-------------------------------OTROS OBJETIVOS---------------------------------

# Ejecutar el programa.
run: falonso
	clear
	./falonso 7 1

# Objetivo para limpieza.
clean:
	rm -rf falonso .vscode proyecto.tar

# Objetivo para comprimir.
tar: $(SOURCES) $(HEADERS) Makefile
	tar -cvf proyecto.tar $^

# Objetivo para descomprimir.
untar: proyecto.tar
	tar -xvf proyecto.tar

#-------------------------------------------------------------------------------
