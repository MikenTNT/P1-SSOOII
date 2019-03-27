#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>  /* Funcion ftok().  */
#include <sys/sem.h>  /* Semáforos.  */
#include <sys/shm.h>
#include <sys/types.h>  /* Types pids.  */
#include <sys/msg.h>  /* Buzones */

#include <signal.h>  /* Tratamiento de señales.  */
#include <unistd.h>  /* Librería para fork().  */

#include "falonso.h"


/* Constantes de la funcion exit().  */
/* Error genérico.  */
#define ERR_GEN (unsigned char)150
/* Error de argumentos.  */
#define ERR_ARGS (unsigned char)151
/* Error captura de señal.  */
#define ERR_CAP_SIGN (unsigned char)152

#define NUM_HIJOS 10
#define TAM 350


/* Prototipos de función.  */
void opSemaforo(int semaforos, int nSemaforo, int nSignal);
void perrorExit(int code, char *msg);
void salir(int senial);

void leerMensaje(int carril, int desp);
void enviarMensaje(int carril, int desp);
int puedoAvanzar(int carril, int desp);
void mandarMensajePosicion(int carril, int desp);
void mandarMensajeCambio(int carril, int desp);
int puedoCambioCarril(int carril, int desp);


/* Definicion de tipos de datos y variables globales.  */
typedef struct tipo_mensaje {
	long tipo;
	char info[20];
} mensaje;

struct recursosIPC {
	int semaforos;
	int buzon;
	int memoriaID;
	void *pMemoria;
	pid_t pid_hijos[NUM_HIJOS];
	mensaje men;
} rIPC;


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


	/* Comprobación de la captura de la señal CTLR + C.  */
	if (signal(SIGINT, salir) == SIG_ERR)
		perrorExit(ERR_CAP_SIGN, "No se puede capturar la señal.");


	/* Variables de la función main().  */
	int i;
	int desp = 0;
	int tempC, tempD;
	int color = ROJO;
	int carril = CARRIL_DERECHO;
	int velo = 40;
	pid_t pidPadre = getpid();
	pid_t pidHijo;
	rIPC.semaforos = -1;
	rIPC.buzon = -1;
	rIPC.memoriaID = -1;
	rIPC.pMemoria = NULL;
	sprintf(rIPC.men.info, "Carta Recibida");
	for (i = 0; i < NUM_HIJOS; i++)
		rIPC.pid_hijos[i] = -1;

	/* Declaración de la union para semaforos solaris.  */
	union semun {
		int val;
		struct semid_ds *buf;
		ushort_t *array;
	} semf;


	/* Inicia y comprueba la creacíon del array de semáforos.  */
	if (-1 == (rIPC.semaforos = semget(IPC_PRIVATE, 5, IPC_CREAT | 0600))) {
		perror("No se pudo crear el semaforos");
		exit(0);
	}

	/* Damos valor a los semáforos.  */
	semf.val = 0;
	if (-1 == semctl(rIPC.semaforos, 1, SETVAL, semf)) {
		perror("No se ha podido asignar");
		raise(SIGINT);
	}

	semf.val = 6;
	if (-1 == semctl(rIPC.semaforos, 2, SETVAL, semf)) {
		perror("No se ha podido asignar");
		raise(SIGINT);
	}

	semf.val = 1;
	if (-1 == semctl(rIPC.semaforos, 3, SETVAL, semf)) {
		perror("No se ha podido asignar");
		raise(SIGINT);
	}

	semf.val = NUM_HIJOS;
	if (-1 == semctl(rIPC.semaforos, 4, SETVAL, semf)) {
		perror("No se ha podido asignar");
		raise(SIGINT);
	}

	/* Creación del buzon.  */
	if (-1 == (rIPC.buzon = msgget(IPC_PRIVATE, 0600 | IPC_CREAT))) {
		perror("No se ha podido crear el buzón");
		raise(SIGINT);
	}

	/* Inicia y comprueba la creacíon de memoria compartida.  */
	if (-1 == (rIPC.memoriaID = shmget(IPC_PRIVATE, TAM, 0666 | IPC_CREAT))) {
		perror("No se ha podido crear la memoria");
		raise(SIGINT);
	}

	/* Asociación del puntero a la memoria compartida.  */
	if ((void *)-1 == (rIPC.pMemoria = shmat(rIPC.memoriaID, NULL, 0))) {
		perror("No se ha podido asignar el puntero");
		raise(SIGINT);
	}

	/* Enviar Mensaje a todas las posiciones, sumando 1 para no enviar al 0.  */
	for (i = 0; i < 274; i++) {
		rIPC.men.tipo = i + 1;
		if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0))
			printf("Imposible enviar mensaje\n");
	}

	inicio_falonso(atoi(argv[2]), rIPC.semaforos, rIPC.pMemoria);

	/* Creamos a los hijos/coches.  */
	for (i = 0; i < NUM_HIJOS; i++) {
		switch (rIPC.pid_hijos[i] = fork()) {
			case -1:
				perror("Error al crear a los hijos");
				raise(SIGINT);
			case 0:
				pidHijo = getpid();
				srand(pidHijo);
				carril = pidHijo % 2;
				desp = (i / 2) + i;
				velo = 1 + (rand() % 99);
				if (4 == (color = pidHijo % 8))
					color ++;

				if (carril == 1)
					rIPC.men.tipo = desp + 137 + 1;
				else
					rIPC.men.tipo = desp + 1;

				if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), rIPC.men.tipo, 0)) {
					perror("Error de puedoAvanzar");
					raise(SIGINT);
				}

				if (-1 == inicio_coche(&carril, &desp, color)) {
					perror("Error de creacion del coche");
					raise(SIGINT);
				}

				/* Esperamos a que todos los hijos estén creados.  */
				fprintf(stderr,"Carril %d Pos %d Velo %d\n",carril,desp,velo);
				opSemaforo(rIPC.semaforos, 4, -1);
				opSemaforo(rIPC.semaforos, 1, -1);

				while (1) {
					if (puedoAvanzar(carril, desp)) {
						velocidad(velo, carril, desp);

						/*
						 * Hacemos atómica el avanzar y enviar mensaje
						 *  a la posición anterior.
						 */
						opSemaforo(rIPC.semaforos, 3, -1);
						avance_coche(&carril, &desp, color);
						/* Enviamos mensaje a la posicion que dejamos libre.  */
						mandarMensajePosicion(carril, desp);
						opSemaforo(rIPC.semaforos, 3, 1);
					}
					else {
						if (puedoCambioCarril(carril, desp)) {
							opSemaforo(rIPC.semaforos, 3, -1);
							tempC = carril;
							tempD = desp;
							cambio_carril(&carril, &desp, color);
							mandarMensajeCambio(tempC,tempD);
							opSemaforo(rIPC.semaforos, 3, 1);
						}
					}
				}

				exit(0);
		}
	}


	opSemaforo(rIPC.semaforos, 4, 0);
	opSemaforo(rIPC.semaforos, 1, NUM_HIJOS);

	luz_semAforo(0, 1);
	leerMensaje(0, 105);
	leerMensaje(1, 98);
	luz_semAforo(1, 2);

	/* Sincronizacion de los semaforos y cambio de color de estos.  */
	while (1) {
		leerMensaje(0, 20);
		leerMensaje(1, 22);
		luz_semAforo(1, 1);

		opSemaforo(rIPC.semaforos, 2, -6);

		luz_semAforo(0, 2);

		opSemaforo(rIPC.semaforos, 2, 6);

		enviarMensaje(0, 105);
		enviarMensaje(1, 98);
		sleep(1);

		leerMensaje(0, 105);
		leerMensaje(1, 98);
		luz_semAforo(0, 1);
		opSemaforo(rIPC.semaforos, 2, -6);
		luz_semAforo(1, 2);
		opSemaforo(rIPC.semaforos, 2, 6);
		enviarMensaje(0, 20);
		enviarMensaje(1, 22);
		sleep(1);
	}


	raise(SIGINT);

}


int puedoAvanzar(int carril, int desp)
{
	if (carril == 0) {
		/* CARRIL DERECHO CON SUS OPCIONES Y VALORES.  */
		if (desp == 21 || desp == 106) {
			fprintf(stderr, "%d\n", -1);
			opSemaforo(rIPC.semaforos,2,-1);
		}
		else if (desp == 22 || desp == 23 || desp == 107 || desp == 108) {
			fprintf(stderr, "%d%d\n", -1,1);
			opSemaforo(rIPC.semaforos, 2, 1);
			opSemaforo(rIPC.semaforos, 2, -1);
		}
		else if (desp == 24 || desp == 109) {
			fprintf(stderr, "%d\n", 1);
			opSemaforo(rIPC.semaforos, 2, 1);
		}

		if (desp == 136) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 1, IPC_NOWAIT))
				return 0;
		}
		else if (desp == 19 || desp == 104) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 2, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
		}
		else {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 2, IPC_NOWAIT))
				return 0;
		}
	}
	else {
		/* CARRIL IZQUIERDO CON SUS OPCIONES Y VALORES.  */
		/* Wait y Signal del cruce.  */
		if (desp == 23 || desp == 99) {
			fprintf(stderr, "%d\n", -1);
			opSemaforo(rIPC.semaforos, 2, -1);
		}
		else if (desp == 24 || desp == 25 || desp == 100 || desp == 101) {
			fprintf(stderr, "%d%d\n", -1,1);
			opSemaforo(rIPC.semaforos, 2, 1);
			opSemaforo(rIPC.semaforos, 2, -1);
		}
		else if (desp == 26 || desp == 102) {
			fprintf(stderr, "%d\n", 1);
			opSemaforo(rIPC.semaforos, 2, 1);
		}

		if ((desp + 137) == 273) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 138, IPC_NOWAIT))
				return 0;
		}
		else if (desp == 21 || desp == 97) {
			if (msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp + 2, 0) == -1) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
		}
		else {
			if (msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp + 2, IPC_NOWAIT) == -1)
				return 0;
		}
	}

	return 1;
}


int puedoCambioCarril(int carril, int desp)
{
	if (carril == 0) {
		if ((desp >= 0 && desp <= 13) || (desp >= 29 && desp <= 60)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp + 1, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +0
		}
		else if (desp >= 14 && desp <= 28) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp + 2, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +1
		}
		else if ((desp >= 61 && desp <= 62) || (desp >= 135 && desp <= 136)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// -1
		}
		else if ((desp >= 63 && desp <= 65) || (desp >= 131 && desp <= 134)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp - 1, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			//-2
		}
		else if ((desp >= 66 && desp <= 67) || (desp == 130)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp - 2, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			//-3
		}
		else if (desp == 68) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp - 3, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			//-4
		}
		else if (desp >= 69 && desp <= 129) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp - 4, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			//-5
		}
	}
	else {
		if ((desp >= 0 && desp <= 15) || (desp >= 29 && desp <= 58)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 1, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// 0
		}
		else if (desp >= 16 && desp <= 28) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp , 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// -1
		}
		else if (desp >= 59 && desp <= 60) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 2, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +1
		}
		else if ((desp >= 61 && desp <= 62) || (desp >= 129 && desp <= 133)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 3, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +2
		}
		else if (desp >= 127 && desp <= 128) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 4, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +3
		}
		else if ((desp >= 63 && desp <= 64) || (desp == 126)) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 5, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +4
		}
		else if (desp >= 65 && desp <= 125) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 6, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// +5
		}
		else if (desp >= 134 && desp <= 136) {
			if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137, 0)) {
				perror("Error de puedoAvanzar");
				raise(SIGINT);
			}
			// 136
		}
	}

	return 1;
}


void leerMensaje(int carril, int desp)
{
	if (carril == 0) {
		if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), desp + 1, 0)) {
			perror("Error de puedoAvanzar");
			raise(SIGINT);
		}
	}
	else {
		if (-1 == msgrcv(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 137 + desp + 1, 0)) {
			perror("Error de puedoAvanzar");
			raise(SIGINT);
		}
	}
}


void enviarMensaje(int carril, int desp)
{
	if (carril == 0) {
		rIPC.men.tipo = desp + 1;
		if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
			perror("Imposible enviar mensaje");
			raise(SIGINT);
		}
	}
	else {
		rIPC.men.tipo = 137 + desp + 1;
		if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
			perror("Imposible enviar mensaje");
			raise(SIGINT);
		}
	}
}


void mandarMensajePosicion(int carril, int desp)
{
	if (carril == 0) {
		if (desp + 1 == 1) {
			rIPC.men.tipo = 137;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			rIPC.men.tipo = desp;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
	}
	else {
		if (137 + desp + 1 == 138) {
			rIPC.men.tipo = 274;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			rIPC.men.tipo = 137 + desp;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
	}
}


void mandarMensajeCambio(int carril, int desp)
{
	if (carril == 0) {
		if (desp + 1 == 1) {
			rIPC.men.tipo = 137;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			rIPC.men.tipo = desp + 1;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
	}
	else {
		if (137 + desp + 1 == 138) {
			rIPC.men.tipo = 274;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			rIPC.men.tipo = 137 + desp + 1;
			if (-1 == msgsnd(rIPC.buzon, &(rIPC.men), sizeof(rIPC.men.info), 0)) {
				perror("Imposible enviar mensaje");
				raise(SIGINT);
			}
		}
	}
}


/* Función para hacer signal y wait.  */
void opSemaforo(int semaforo, int nSemaforo, int nSignal)
{
	/* Declaramos la estructura de control del semáforo.  */
	struct sembuf opSem;

	/* Establecemos los valores de control.  */
	opSem.sem_num = nSemaforo;
	opSem.sem_op = nSignal;
	opSem.sem_flg = 0;

	/* Ejecutamos la operación del semáforo.  */
	if (-1 == semop(semaforo, &opSem, 1)) {
		perror("Error al hacer Signal");
		raise(SIGINT);
	}
}


/* Función de captura de la señal CTRL + C.  */
void salir(int senial)
{
	/* Ignoramos las nuevas señales CTRL + C.  */
	signal(SIGINT, SIG_IGN);


	/* variables de la función.  */
	int i;


	/* Eliminamos los hijos.  */
	for (i = 0; i < NUM_HIJOS; i++)
		if (rIPC.pid_hijos[i] != -1)
			kill(rIPC.pid_hijos[i], SIGKILL);

	/* @TODO wait for process finish.  ---------------------------------------*/

	fin_falonso(&i);

	/* Eliminamos los recursos IPC.  */
	if (rIPC.semaforos != -1)
		if (semctl(rIPC.semaforos, 0, IPC_RMID) == -1)
			perror("Error en el borrado de los semáforos");

	if (rIPC.buzon != -1)
		if (msgctl(rIPC.buzon, IPC_RMID, NULL) == -1)
			perror("Error en borrado de buzon");

	if (rIPC.pMemoria != NULL)
		if (shmdt(rIPC.pMemoria) == -1)
			perror("No se ha podido eliminar el puntero de memoria");

	if (rIPC.memoriaID != -1)
		if (shmctl(rIPC.memoriaID, IPC_RMID, NULL) == -1)
			perror("No se ha podido liberar la memoria");


	/* Bueno para borrar huellas... */
	// system("clear");


	/* Activamos de nuevo las señales CTRL + C.  */
	signal(SIGINT, salir);


	exit(0);
}


/* perror + exit.  */
void perrorExit(int code, char *msg)
{
	/* Imprime el mensaje de error.  */
	perror(msg);

	/* Sale del programa con el código especificado. */
	exit(code);
}

/*----------------------------------------------------------------------------*/

// while(1) {

// 	leerMensaje(V);
// 	ambar;
// 	funcionValores();
// 	luzRoja;
// 	luzVerd(H);
// 	enviarMensaje(H);
// 	alarm();

// 	leerMensaje(H);
// 	ambar;
// 	funcionValores();
// 	luzRoja(H);
// 	luzVerde(V);
// 	enviarMensaje(V);
// 	alarm();
// }

/*----------------------------------------------------------------------------*/

/*
case 0:

				desp = i ;

				if (i < 10) {
					carril = 0;
					color = rand() % 8;
					if (color == 4) {
						color += 1;
					}
					desp = rand() % 137;
					velo = (rand() % 90) + 9;
				}
				else {
					if (i == 10) {
						desp = 0;
					}
					carril = 1;
					color = rand() % 8;
					if (color == 4) {
						color += 1;
					}
					desp += 5;
					velo = (rand() % 90) + 9;
				}
				printf("Velo %d Carril %d Color %d desp %d\n",velo,carril,color,desp);
				printf("Entro aqui\n");

				if (i == 1) {
					carril = 0;
					desp = 20;
					color = AMARILLO;
					velo = 60;
				}
				else if (i == 2) {
					desp = 70;
					carril = 0;
					color = VERDE;
					velo = 10;
				}
				else if (i == 3) {
					desp = 100;
					carril = 0;
					color = BLANCO;
					velo = 60;
				}
				else if (i == 4) {
					carril = 0;
					desp = 80;
					color = AMARILLO;
					velo = 50;
				}
				else if (i == 5) {
					desp = 70;
					carril = 1;
					color = VERDE;
					velo = 15;
				}
				else if (i == 6) {
					desp = 125;
					carril = 1;
					color = BLANCO;
					velo = 93;
				}
				else if (i == 7) {
					carril = 1;
					desp = 10;
					color = AMARILLO;
					velo = 55;
				}
				else if (i == 8) {
					desp = 28;
					carril = 1;
					color = VERDE;
					velo = 20;
				}
				else if (i == 9) {
					desp = 37;
					carril = 1;
					color = BLANCO;
					velo = 90;
				}
*/

/*----------------------------------------------------------------------------*/
