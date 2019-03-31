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
SOURCES = $(MAIN).c libfalonso.a
# HEADERS: archivos de cabecera de C.
HEADERS = $(MAIN).h

#----------------------------OBJETIVOS PRINCIPALES------------------------------

# Objetivos a ejecutar con el comando make.
all: $(MAIN)


# Crear el archivo de ejecución falonso.
$(MAIN): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(MAIN) $(SOURCES)

#-------------------------------OTROS OBJETIVOS---------------------------------

# Ejecutar el programa.
run: $(MAIN)
	clear
	./$(MAIN) 7 1

# Objetivo para limpieza.
clean:
	rm -rf $(MAIN) .vscode proyecto.tar

# Objetivo para comprimir.
tar: $(SOURCES) $(HEADERS) Makefile
	tar -cvf proyecto.tar $^

# Objetivo para descomprimir.
untar: proyecto.tar
	tar -xvf proyecto.tar

#-------------------------------------------------------------------------------
