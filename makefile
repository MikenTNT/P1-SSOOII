# MAKEFILE DE PROYECTO C
# Autor: MikenTNT  31-MAR-2019.

#----------------------------------VARIABLES------------------------------------

# Compilador de C 'gcc'.
CC = gcc
# CFLAGS: depurador '-g', matematicas '-lm'.
CFLAGS = -m32 -Wall

# Nombre del archivo a principal.
MAIN = falonso
# SOURCES: archivos fuentes de C.
SOURCES = $(main).c libfalonso.a
# HEADERS: archivos de cabecera de C.
HEADERS = $(main).h

#----------------------------OBJETIVOS PRINCIPALES------------------------------

# Objetivos a ejecutar con el comando make.
all: $(main)


# Crear el archivo de ejecución falonso.
$(main): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(main) $(SOURCES)

#-------------------------------OTROS OBJETIVOS---------------------------------

# Ejecutar el programa.
run: $(main)
	clear
	./$(main) 7 1

# Objetivo para limpieza.
clean:
	rm -rf $(main) .vscode proyecto.tar

# Objetivo para comprimir.
tar: $(SOURCES) $(HEADERS) Makefile
	tar -cvf proyecto.tar $^

# Objetivo para descomprimir.
untar: proyecto.tar
	tar -xvf proyecto.tar

#-------------------------------------------------------------------------------
