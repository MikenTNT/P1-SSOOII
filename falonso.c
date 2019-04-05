#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>  /* Funcion ftok().  */
#include <sys/sem.h>  /* Semáforos.  */
#include <sys/shm.h>
#include <sys/types.h>  /* Types pids.  */
#include <sys/msg.h>  /* Buzones */
#include <sys/wait.h>

#include <signal.h>  /* Tratamiento de señales.  */
#include <unistd.h>  /* Librería para fork().  */

#include "falonso.h"


/* Constantes de la funcion exit().  */
#define ERR_ARGS (unsigned char)151  /* Error de argumentos.  */
#define ERR_SIGN (unsigned char)152  /* Error captura de señal.  */
#define ERR_CR_MEM (unsigned char)153  /* Error crear memoria compartida.  */

/* Constantes de los semáforos.  */
#define SEM_PADRE 1
#define SEM_HIJOS 2
#define SEM_SYNC 3
#define SEM_MENS 4

/* Número máximo de hijos.  */
#define NUM_HIJOS 25


/* Funciones del hijo.  */
void hijo(int numHijo);
int puedoAvanzar(int carril, int desp);
int puedoCambiarCarril(int carril, int desp);
void cogerPosicion(int carril, int desp);
void liberarPosicion(int carril, int desp);

/* Funciones del padre.  */
void padre(int numHijos);
void abrirCruce(int dirCruce);
void cerrarCruce(int dirCruce);

/* Funciones control IPCs.  */
void inicioSemaforo(int *semaforo, int nSemaforo, int val);
void opSemaforo(int nSemaforo, int nSignal);
void enviarMensaje(long tipo);
void leerMensaje(long tipo);
int leerMensajeNW(long tipo);

/* Funciones de las manejadoras.  */
void finPadre(int seNial);
void finHijos(int seNial);


/* Definicion del tipo de mensaje, para el buzón.  */
typedef struct tipo_mensaje {
	long tipo;
	char info[20];
} mensaje;

/* Definición de la memoria compartida.  */
typedef struct memoriaCompartida {
	char cDer[137];  /* 0-136 */
	char cIzq[137];  /* 137-273 */
	char semH;  /* 274 */
	char semV;  /* 275 */
	char reservado[25];  /* 276-300 */

	int semaforo;
	int buzon;
	pid_t pid_hijos[NUM_HIJOS];

	int vueltasCoches;
} memoria;


/* Variables globales para la memoria.  */
int idMemoria = -1;  /* ID de la memoria.  */
memoria *pMemoria = NULL;  /* Puntero de la memoria.  */



int main(int argc, char const *argv[]) {

	/* Comprobación de los argumentos.  */
	if (argc != 3) {
		puts("Modo de uso, \"./falonso (numero de coches) (velocidad 0/1)\".");
		exit(ERR_ARGS);
	}
	else if ((atoi(argv[2]) != 0) && (atoi(argv[2]) != 1)) {
		puts("Velocidad de coches 1(normal) o 0(rápida).");
		exit(ERR_ARGS);
	}
	else if (atoi(argv[1]) <= 0) {
		puts("Número de coches erróneo.");
		exit(ERR_ARGS);
	}


	/* Variables de la función main().  */
	struct sigaction parada, antiguo;  /* Variables para sigaction.  */
	int i;  /* Variable de iteración.  */
	pid_t pidHijo;
	pid_t pidPadre = getpid();  /* PID del padre.  */


	/* Valores para la estructura sigaction.  */
	parada.sa_handler = &finPadre;
	parada.sa_flags = SA_RESTART;
	sigfillset(&parada.sa_mask);

	/* Comprobación de la captura de la señal SIGINT.  */
	if (-1 == sigaction(SIGINT, &parada, &antiguo)) {
		perror("No se ha podido capturar CTRL+C");
		exit(ERR_SIGN);
	}


	/* Inicia y comprueba la creacíon de memoria compartida.  */
	if (-1 == (idMemoria = shmget(IPC_PRIVATE, sizeof(memoria), 0666 | IPC_CREAT))) {
		perror("No se ha podido crear la memoria");
		exit(ERR_CR_MEM);
	}

	/* Asociación del puntero a la memoria compartida.  */
	if ((void *)-1 == (pMemoria = (memoria *)shmat(idMemoria, NULL, 0))) {
		perror("No se ha podido asignar el puntero");
		raise(SIGINT);
	}

	/* Inicializamos los valores de la memoria.  */
	pMemoria->semaforo = -1;
	pMemoria->buzon = -1;
	for (i = 0; i < NUM_HIJOS; i++)
		pMemoria->pid_hijos[i] = -1;

	/* Iniciamos el contador de vueltas.  */
	pMemoria->vueltasCoches = 0;


	/* Inicia y comprueba la creación del array de semáforos.  */
	if (-1 == (pMemoria->semaforo = semget(IPC_PRIVATE, 5, 0600 | IPC_CREAT))) {
		perror("No se pudo crear el semaforo");
		raise(SIGINT);
	}

	/* Creación del buzón.  */
	if (-1 == (pMemoria->buzon = msgget(IPC_PRIVATE, 0600 | IPC_CREAT))) {
		perror("No se ha podido crear el buzón");
		raise(SIGINT);
	}

	/* Damos valor a los semáforos.  */
	inicioSemaforo(&(pMemoria->semaforo), SEM_PADRE, 0);
	inicioSemaforo(&(pMemoria->semaforo), SEM_HIJOS, 0);
	inicioSemaforo(&(pMemoria->semaforo), SEM_SYNC, 1);
	inicioSemaforo(&(pMemoria->semaforo), SEM_MENS, 1);


	/* Enviar mensaje a todas las posiciones, sumando 1 para no enviar al 0.  */
	for (i = 0; i < 274; i++)  enviarMensaje(i);


	/* Iniciamos el circuito.  */
	if (-1 == inicio_falonso(atoi(argv[2]), pMemoria->semaforo, (char *)pMemoria)) {
		perror("Error inicio falonso");
		raise(SIGINT);
	}


	/* Creamos a los hijos/coches.  */
	for (i = 0; i < atoi(argv[1]); i++) {
		switch (pidHijo = fork()) {
			case -1:
				perror("Error al crear a los hijos");
				raise(SIGINT);
			case 0:
				/* Valores para la estructura sigaction.  */
				parada.sa_handler = &finHijos;
				parada.sa_flags = SA_RESTART;
				sigemptyset(&parada.sa_mask);  /* Reinicio de la máscara.  */
				sigfillset(&parada.sa_mask);

				/* Reinicio de la señal SIGINT.  */
				if (-1 == sigaction(SIGINT, &antiguo, NULL)) {
					perror("No se ha podido capturar CTRL+C");
					exit(ERR_SIGN);
				}

				/* Comprobación de la captura de la señal SIGUSR1.  */
				if (-1 == sigaction(SIGUSR1, &parada, NULL)) {
					perror("No se ha podido capturar CTRL+C");
					exit(ERR_SIGN);
				}

				/* Acciones del hijo.  */
				hijo(i);
			default:
				pMemoria->pid_hijos[i] = pidHijo;
		}
	}


	/* Acciones del padre.  */
	if (getpid() == pidPadre)  padre(atoi(argv[1]));


	exit(0);

}


/* Control del padre.  */
void padre(int numHijos)
{
	/* Esperamos a que todos los hijos estén creados.  */
	opSemaforo(SEM_PADRE, -numHijos);

	/* Control semáforo (V=1  H=0).  */
	cerrarCruce(HORIZONTAL);

	if (-1 == luz_semAforo(HORIZONTAL, ROJO)) {
		perror("Error luz semaforo");
		raise(SIGINT);
	}
	if (-1 == luz_semAforo(VERTICAL, VERDE)) {
		perror("Error luz semaforo");
		raise(SIGINT);
	}

	/* Pistoletazo hijos.  */
	opSemaforo(SEM_HIJOS, numHijos);


	/* Sincronizacion de los semáforos y cambio de color de estos.  */
	while(1) {
		/* Control semáforo (V=0  H=1).  */
		if (-1 == luz_semAforo(VERTICAL, AMARILLO)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}

		cerrarCruce(VERTICAL);

		if (-1 == luz_semAforo(VERTICAL, ROJO)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}
		if (-1 == luz_semAforo(HORIZONTAL, VERDE)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}

		abrirCruce(HORIZONTAL);

		sleep(1);


		/* Control semáforo (V=1  H=0).  */
		if (-1 == luz_semAforo(HORIZONTAL, AMARILLO)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}

		cerrarCruce(HORIZONTAL);

		if (-1 == luz_semAforo(HORIZONTAL, ROJO)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}
		if (-1 == luz_semAforo(VERTICAL, VERDE)) {
			perror("Error luz semaforo");
			raise(SIGINT);
		}

		abrirCruce(VERTICAL);

		sleep(1);
	}
}


/* Control del hijo.  */
void hijo(int numHijo)
{
	/* Inicio de la semilla.  */
	srand(getpid());

	/* Variables del hijo.  */
	int carril = numHijo % 2;  /* Carril del coche.  */
	int desp = numHijo / 2;  /* Posición del coche.  */
	int color = rand() % 8;  /* Color del coche.  */
	int velo = 1 + (rand() % 99);  /* Velocidad del coche.  */
	int tempC, tempD;  /* Variables temporales para el cambio de posición.  */

	/* Si calcula un color azul lo cambia a otro.  */
	if (4 == color)  color ++;


	/* Configuramos el mensaje de la posición.  */
	if (carril == CARRIL_DERECHO) {
		if (!leerMensajeNW(desp)) {
			perror("Error de reserva de posición");
			raise(SIGINT);
		}
	}
	else if (carril == CARRIL_IZQUIERDO) {
		if (!leerMensajeNW(137 + desp)) {
			perror("Error de reserva de posición");
			raise(SIGINT);
		}
	}

	/* Iniciamos el coche.  */
	if (-1 == inicio_coche(&carril, &desp, color)) {
		perror("Error de creacion del coche");
		raise(SIGINT);
	}


	/* Esperamos a que todos los hijos estén creados.  */
	opSemaforo(SEM_PADRE, 1);
	opSemaforo(SEM_HIJOS, -1);

	/* Bucle de ejecución.  */
	while (1) {
		velocidad(velo, carril, desp);
		if (puedoAvanzar(carril, desp)) {
			/* Inicio parte atómica.  */
			opSemaforo(SEM_SYNC, -1);
			/*----------------------------------------------------------------*/
			tempC = carril;
			tempD = desp;
			avance_coche(&carril, &desp, color);
			liberarPosicion(tempC, tempD);

			/* Incremento del contador si es necesario.  */
			if (((carril == 0) && (desp == 133)) || ((carril == 1) && (desp == 131)))
				(pMemoria->vueltasCoches)++;
			/*----------------------------------------------------------------*/
			opSemaforo(SEM_SYNC, 1);
			/* Fin parte atómica.  */

			continue;
		}
		else if (puedoCambiarCarril(carril, desp)) {
			/* Inicio parte atómica.  */
			opSemaforo(SEM_SYNC, -1);
			/*----------------------------------------------------------------*/
			tempC = carril;
			tempD = desp;
			cambio_carril(&carril, &desp, color);
			liberarPosicion(tempC, tempD);
			/*----------------------------------------------------------------*/
			opSemaforo(SEM_SYNC, 1);
			/* Fin parte atómica.  */

			continue;
		}


		cogerPosicion(carril, desp);
		/* Inicio parte atómica.  */
		opSemaforo(SEM_SYNC, -1);
		/*--------------------------------------------------------------------*/
		tempC = carril;
		tempD = desp;
		avance_coche(&carril, &desp, color);
		liberarPosicion(tempC, tempD);

		/* Incremento del contador si es necesario.  */
		if (((carril == 0) && (desp == 133)) || ((carril == 1) && (desp == 131)))
			(pMemoria->vueltasCoches)++;
		/*--------------------------------------------------------------------*/
		opSemaforo(SEM_SYNC, 1);
		/* Fin parte atómica.  */
	}
}


/* Comprueba si puede avanzar.  */
int puedoAvanzar(int carril, int desp)
{
	if (carril == CARRIL_DERECHO) {
		if (desp != 136)
			return leerMensajeNW(desp + 1);
		else
			return leerMensajeNW(0);
	}
	else if (carril == CARRIL_IZQUIERDO) {
		if (desp != 136)
			return leerMensajeNW(137 + desp + 1);
		else
			return leerMensajeNW(137);
	}

	return 1;
}


/* Comprueba si puede cambiar de carril.  */
int puedoCambiarCarril(int carril, int desp)
{
	if (carril == CARRIL_DERECHO) {
		if (desp >= 14 && desp <= 28)
			return leerMensajeNW(137 + desp + 1);
		else if ((desp >= 0 && desp <= 13) || (desp >= 29 && desp <= 60))
			return leerMensajeNW(137 + desp);
		else if ((desp >= 61 && desp <= 62) || (desp >= 135 && desp <= 136))
			return leerMensajeNW(137 + desp - 1);
		else if ((desp >= 63 && desp <= 65) || (desp >= 131 && desp <= 134))
			return leerMensajeNW(137 + desp - 2);
		else if ((desp >= 66 && desp <= 67) || (desp == 130))
			return leerMensajeNW(137 + desp - 3);
		else if (desp == 68)
			return leerMensajeNW(137 + desp - 4);
		else if (desp >= 69 && desp <= 129)
			return leerMensajeNW(137 + desp - 5);
	}
	else if (carril == CARRIL_IZQUIERDO) {
		if (desp >= 16 && desp <= 28)
			return leerMensajeNW(desp - 1);
		else if ((desp >= 0 && desp <= 15) || (desp >= 29 && desp <= 58))
			return leerMensajeNW(desp);
		else if (desp >= 59 && desp <= 60)
			return leerMensajeNW(desp + 1);
		else if ((desp >= 61 && desp <= 62) || (desp >= 129 && desp <= 133))
			return leerMensajeNW(desp + 2);
		else if (desp >= 127 && desp <= 128)
			return leerMensajeNW(desp + 3);
		else if ((desp >= 63 && desp <= 64) || (desp == 126))
			return leerMensajeNW(desp + 4);
		else if (desp >= 65 && desp <= 125)
			return leerMensajeNW(desp + 5);
		else if (desp >= 134 && desp <= 136)
			return leerMensajeNW(136);
	}

	return 1;
}


/* Mira la posición que tiene delante y lee el mensaje cuando haya.  */
void cogerPosicion(int carril, int desp)
{
	if (carril == CARRIL_DERECHO) {
		if (desp != 136)
			leerMensaje(desp + 1);
		else
			leerMensaje(0);
	}
	else if (carril == CARRIL_IZQUIERDO) {
		if (desp != 136)
			leerMensaje(137 + desp + 1);
		else
			leerMensaje(137);
	}
}


/* Envía un mensaje a la posición anterior.  */
void liberarPosicion(int carril, int desp)
{
	if (carril == CARRIL_DERECHO)
		enviarMensaje(desp);
	else if (carril == CARRIL_IZQUIERDO)
		enviarMensaje(137 + desp);
}


/*
 * Manda un mensaje al buzón con las posiciones del cruce
 *  para permitir el paso.
 */
void abrirCruce(int dirCruce)
{
	if (dirCruce == VERTICAL) {
		enviarMensaje(21);
		enviarMensaje(137 + 23);
		enviarMensaje(22);
		enviarMensaje(137 + 24);
		enviarMensaje(23);
		enviarMensaje(137 + 25);
	} else if (dirCruce == HORIZONTAL) {
		enviarMensaje(106);
		enviarMensaje(137 + 99);
		enviarMensaje(107);
		enviarMensaje(137 + 100);
		enviarMensaje(108);
		enviarMensaje(137 + 101);
	}
}


/*
 * Manda un mensaje al buzón con las posiciones del cruce
 *  para bloquear el paso.
 */
void cerrarCruce(int dirCruce)
{
	if (dirCruce == VERTICAL) {
		leerMensaje(21);
		leerMensaje(137 + 23);
		leerMensaje(22);
		leerMensaje(137 + 24);
		leerMensaje(23);
		leerMensaje(137 + 25);
	} else if (dirCruce == HORIZONTAL) {
		leerMensaje(106);
		leerMensaje(137 + 99);
		leerMensaje(107);
		leerMensaje(137 + 100);
		leerMensaje(108);
		leerMensaje(137 + 101);
	}
}


/* Función para iniciar semaforo.  */
void inicioSemaforo(int *semaforo, int nSemaforo, int val)
{
	/* Declaración de la union para semaforos solaris.  */
	union semun {
		int val;
		struct semid_ds *buf;
		ushort_t *array;
	} semf;

	semf.val = val;
	if (-1 == semctl(*semaforo, nSemaforo, SETVAL, semf)) {
		perror("No se ha podido establecer el valor del semáforo");
		raise(SIGINT);
	}
}


/* Función para hacer signal y wait.  */
void opSemaforo(int nSemaforo, int nSignal)
{
	/* Declaramos la estructura de control del semáforo.  */
	struct sembuf opSem;

	/* Establecemos los valores de control.  */
	opSem.sem_num = nSemaforo;
	opSem.sem_op = nSignal;
	opSem.sem_flg = 0;

	/* Ejecutamos la operación del semáforo.  */
	if (-1 == semop(pMemoria->semaforo, &opSem, 1)) {
		perror("Error al hacer Signal");
		raise(SIGINT);
	}
}


/* Función para enviar un mensaje del buzón.  */
void enviarMensaje(long tipo)
{
	mensaje men;
	sprintf(men.info, "Carta Recibida");

	opSemaforo(SEM_MENS, -1);
	men.tipo = tipo + 1;  /* +1 por la lógica de los buzones.  */
	if (-1 == msgsnd(pMemoria->buzon, &men, sizeof(men.info), 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}
	opSemaforo(SEM_MENS, 1);
}


/* Función para leer un mensaje del buzón con flag 0.  */
void leerMensaje(long tipo)
{
	mensaje men;

	if (-1 == msgrcv(pMemoria->buzon, &men, sizeof(men.info), tipo + 1, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
}


/* Función para leer un mensaje del buzón con flag IPC_NOWAIT.  */
int leerMensajeNW(long tipo)
{
	mensaje men;

	if (-1 == msgrcv(pMemoria->buzon, &men, sizeof(men.info), tipo + 1, IPC_NOWAIT))
		return 0;
	else
		return 1;
}


/* Función de la manejadora de la señal CTRL + C.  */
void finPadre(int seNial)
{
	/* Eliminamos los hijos y esperamos a que terminen.  */
	int i;
	for (i = 0; i < NUM_HIJOS; i++) {
		if (pMemoria->pid_hijos[i] != -1) {
			kill(pMemoria->pid_hijos[i], SIGUSR1);
			wait(0);
		}
	}


	/* Pasamos las vueltas a la función fin_falonso().  */
	fin_falonso(&(pMemoria->vueltasCoches));


	/* Eliminamos los recursos IPC.  */
	if (pMemoria->semaforo != -1)
		if (semctl(pMemoria->semaforo, 0, IPC_RMID) == -1)
			perror("Error en el borrado de los semáforos");

	if (pMemoria->buzon != -1)
		if (msgctl(pMemoria->buzon, IPC_RMID, NULL) == -1)
			perror("Error en borrado de buzón");

	if ((pMemoria != NULL) || (pMemoria != (void *)-1))
		if (shmdt((void *)pMemoria) == -1)
			perror("No se ha podido eliminar el puntero de memoria");

	if (idMemoria != -1)
		if (shmctl(idMemoria, IPC_RMID, NULL) == -1)
			perror("No se ha podido liberar la memoria");


	exit(0);
}


/* Función de la manejadora para los hijos.  */
void finHijos(int seNial)
{
	exit(0);
}
