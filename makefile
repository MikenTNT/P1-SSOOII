# MAKEFILE DE PROGRAMA C
# COPYRIGHT Miguel Sanchez Gonzalez JAN-2019.

#----------------------------------VARIABLES------------------------------------

# Compilador de C 'gcc'.
CC = gcc
# CFLAGS: depurador '-g', matematicas '-lm'. -m32
CFLAGS = -m32 -Wall

# SOURCES: archivos fuentes de C.
SOURCES = falonso.c libfalonso.a
# HEADERS: archivos de cabecera de C.
HEADERS = falonso.h
# OBJECTS: archivos compilados a objeto para el linker.
OBJETCS = falonso.o

#----------------------------OBJETIVOS PRINCIPALES------------------------------

# Objetivos a ejecutar con el comando make.
all: falonso


# Crear el archivo de ejecución falonso.
falonso: $(SOURCES)
	$(CC) $(CFLAGS) -o falonso $(SOURCES)

#-------------------------------OTROS OBJETIVOS---------------------------------

# Ejecutar el programa.
run: falonso
	clear
	./falonso

# Objetivo para limpieza.
clean:
	rm -rf $(OBJETCS) falonso

# Objetivo para comprimir.
tar: $(SOURCES) $(HEADERS) Makefile
	tar -cvf proyecto.tar $^

#-------------------------------------------------------------------------------
