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


/* Variables globales.  */
union semun {
	int val;
	struct semid_ds *buf;
	ushort_t *array;
} s;

typedef struct tipo_mensaje {
	long tipo;
	char info[20];
} mensaje;

int semaforo, memoriaID, buzon;
char *puntero = NULL;
pid_t pid_hijos[4];
pid_t pidPadre;
mensaje men;


int main(int argc, char const *argv[]) {

	if (signal(SIGINT, salir) == SIG_ERR)
		perrorExit(ERR_CAP_SIGN, "No se puede capturar la señal.");

	int i;
	int desp = 0;
	int color = ROJO;
	int carril = CARRIL_DERECHO;
	int velo = 700;
	//pid_t pidHijo = 0;
	//int c = CARRIL_IZQUIERDO;


	/* Inicia y comprueba la creacíon de un semáforo.  */
	if (-1 == (semaforo = semget(IPC_PRIVATE, 4, IPC_CREAT | 0600)))
		perrorExit(ERR_CR_SEM, "No se pudo crear el semaforo");

	/* Damos valor a los semáforos.  */
	s.val = 0;
	if (-1 == semctl(semaforo, 1, SETVAL, s))
		perrorExit(ERR_SET_SEM, "No se ha podido asignar");

	s.val = 6;
	if (-1 == semctl(semaforo, 2, SETVAL, s))
		perrorExit(ERR_SET_SEM, "No se ha podido asignar");

	s.val = 1;
	if (-1 == semctl(semaforo, 3, SETVAL, s))
		perrorExit(ERR_SET_SEM, "No se ha podido asignar");

	/* Inicia y comprueba la creacíon de memoria compartida.  */
	if (-1 == (memoriaID = shmget(IPC_PRIVATE, TAM, IPC_CREAT | 0666)))
		perrorExit(ERR_CR_MEM, "No se ha podido crear la memoria");

	/* Asociación del puntero a la memoria compartida.  */
	if ((void *)-1 == (puntero = shmat(memoriaID, NULL, 0)))
		perrorExit(ERR_SET_MEM, "No se ha podido asignar puntero");

	/* Creación del buzon.  */
	if (-1 == (buzon = msgget(IPC_PRIVATE, 0600 | IPC_CREAT)))
		perrorExit(ERR_CR_BUZ, "No se ha podido crear el buzón");

	/* Enviar Mensaje a todas las posiciones, sumando 1 para no enviar al 0.  */
	for (i = 0; i < 274; i++) {
		men.tipo = i + 1;
		sprintf(men.info, "%s", "Carta Recibida");
		if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
			printf("Imposible enviar mensaje\n");
	}

	pidPadre = getpid();
	inicio_falonso(1, semaforo, puntero);

	for (i = 0; i < 4; i++) {
		if (pidPadre == getpid()) {
			switch (fork()) {
				case -1:
					perrorExit(ERR_CR_HIJO, "Error de creacion del hijo");
				case 0:
					pid_hijos[i] = getpid();
					desp = i ;
					if (i == 1) {
						carril = 0;
						desp = 80;
						color = AMARILLO;
						velo = 80;
					} else if (i == 2) {
						desp = 60;
						carril = 0;
						color = VERDE;
						velo = 10;
					} else if (i == 3) {
						desp = 100;
						carril = 0;
						color = BLANCO;
						velo = 90;
					}

					if (carril == 1) {
						men.tipo = desp + 137 + 1;
					} else {
						men.tipo = desp + 1;
					}

					if (-1 == msgrcv(buzon, &men, sizeof(men.info), men.tipo, 0))
						perrorExit(ERR_GEN, "Error de puedoAvanzar");

					if ( -1 == inicio_coche(&carril, &desp, ROJO))
						perrorExit(ERR_CR_HIJO, "Error de creacion del coche");

					//Esperamos a que todos los hijos estén creados
					opSemaforo(semaforo, 1, -1);
					while (1) {
						if (puedoAvanzar(carril, desp)) {
							velocidad(velo, carril, desp);

							// Hacemos atómica el avanzar y enviar mensaje
							// a la posición anterior.
							opSemaforo(semaforo, 3, -1);
							avance_coche(&carril, &desp, color);
							// Enviamos mensaje a la posicion que dejamos libre
							mandarMensajePosicion(carril, desp);
							opSemaforo(semaforo, 3, 1);
						} else {
							if(puedoCambioCarril(carril, desp)) {
								opSemaforo(semaforo, 3, -1);
								cambio_carril(&carril, &desp, color);
								mandarMensajePosicion(carril,desp);
								opSemaforo(semaforo, 3, 1);
							}

						}
					}
			}
		}
	}

	if (getpid() == pidPadre) {
		opSemaforo(semaforo, 1, 4);
		luz_semAforo(0, 1);
		leerMensaje(0, 105);
		leerMensaje(1, 98);
		luz_semAforo(1, 2);

		//Semaforo Rojo en (1, 99), (0, 106) horizontal
		//Semaforo Rojo en (1, 23), (0, 21) vertical
		// while(1) {

		// }

		// Sincronizacion de los semaforos
		// y cambio de color de estos
		while (1) {
			leerMensaje(0, 20);
			leerMensaje(1, 22);
			luz_semAforo(1, 1);
			opSemaforo(semaforo, 2, -6);
			luz_semAforo(0, 2);
			opSemaforo(semaforo, 2, 6);
			enviarMensaje(0, 105);
			enviarMensaje(1, 98);
			sleep(1);

			leerMensaje(0, 105);
			leerMensaje(1, 98);
			luz_semAforo(0, 1);
			opSemaforo(semaforo, 2, -6);
			luz_semAforo(1, 2);
			opSemaforo(semaforo, 2, 6);
			enviarMensaje(0, 20);
			enviarMensaje(1, 22);
			sleep(1);
		}
	}

	return 1;

}


int puedoAvanzar(int carril, int desp)
{
	if (carril == 0) {
		/* CARRIL DERECHO CON SUS OPCIONES Y VALORES.  */
		if (desp == 136) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 1, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
		} else if(desp == 19 || desp == 104){
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 2, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
		} else {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 2, IPC_NOWAIT))
				return 0;
				//puedoCambioCarril(carril, desp);
				//perrorExit(ERR_GEN, "Error de puedoAvanzar");

			if (desp == 21 || desp == 22 || desp == 23 || desp == 106 || desp == 107 || desp == 108) {
				opSemaforo(semaforo, 2, -1);
			} else if (desp == 24 || desp == 109) {
				opSemaforo(semaforo, 2, 3);
			}
		}
	} else {
		/* CARRIL IZQUIERDO CON SUS OPCIONES Y VALORES.  */
		if ((desp + 137) == 273) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 138, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
		} else {
			if (msgrcv(buzon, &men, sizeof(men.info), 137 + desp + 2, 0)==-1)
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
		}

		if (desp == 23 || desp == 24 || desp == 25 || desp == 99 || desp == 100 || desp == 101) {
			opSemaforo(semaforo, 2, -1);
		} else if (desp == 26 || desp == 102) {
			opSemaforo(semaforo, 2, 3);
		}
		//fprintf(stderr,"Mensaje leido %d\n", desp + 1);
	}

	return 1;
}

int puedoCambioCarril(int carril, int desp)
{
	if (carril == 0) {
		if ((desp >= 0 && desp <= 13) && (desp >= 29 && desp <= 60)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp + 1, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +0
		} else if (desp >= 14 && desp <= 28) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp + 2, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +1
		} else if ((desp >= 61 && desp <= 62) || (desp >= 135 && desp <= 136)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// -1
		} else if ((desp >= 63 && desp <= 65) || (desp >= 131 && desp <= 134)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp - 1, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			//-2
		} else if ((desp >= 66 && desp <= 67) || (desp == 130)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp - 2, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			//-3
		} else if (desp == 68) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp - 3, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			//-4
		} else if (desp >= 69 && desp <= 129) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp - 4, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			//-5
		}
	} else {
		if ((desp >= 0 && desp <= 15) && (desp >= 29 && desp <= 58)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 1, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// 0
		} else if (desp >= 16 && desp <= 28) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp , 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// -1
		} else if (desp >= 59 && desp <= 60) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 2, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +1
		} else if ((desp >= 61 && desp <= 62) || (desp >= 129 && desp <= 133)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 3, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +2
		} else if (desp >= 127 && desp <= 128) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 4, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +3
		} else if ((desp >= 63 && desp <= 64) || (desp == 126)) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 5, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +4
		} else if (desp >= 65 && desp <= 125) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 6, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// +5
		} else if (desp >= 134 && desp <= 136) {
			if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137, 0))
				perrorExit(ERR_GEN, "Error de puedoAvanzar");
			// 136
		}
	}
	return 1;
}


void leerMensaje(int carril, int desp)
{
	if (carril == 0) {
		if (-1 == msgrcv(buzon, &men, sizeof(men.info), desp + 1, 0))
			perrorExit(ERR_GEN, "Error de puedoAvanzar");
	} else {
		if (-1 == msgrcv(buzon, &men, sizeof(men.info), 137 + desp + 1, 0))
			perrorExit(ERR_GEN, "Error de puedoAvanzar");
	}
}


void enviarMensaje(int carril, int desp)
{
	if (carril == 0) {
		men.tipo = desp + 1;
		sprintf(men.info, "%s", "Carta Recibida");
		if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
			printf("Imposible enviar mensaje aqui\n");
	} else {
		men.tipo = 137 + desp + 1;
		sprintf(men.info, "%s", "Carta Recibida");
		if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
			printf("Imposible enviar mensaje aqui\n");
	}
}

void mandarMensajePosicion(int carril, int desp){

	if (carril == 0) {
		if (desp + 1 == 1) {
			men.tipo = 137;
			sprintf(men.info, "%s", "Carta Recibida");
			// fprintf(stderr, "Mensaje enviado there %ld\n", men.tipo);
			if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
				fprintf(stderr, "Imposible enviar mensaje %ld\n", men.tipo);
		} else {
			men.tipo = desp;
			sprintf(men.info, "%s", "Carta Recibida");
			//fprintf(stderr, "Mensaje enviado %ld\n", men.tipo);
			if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
				fprintf(stderr, "Imposible enviar %ld\n", men.tipo);
		}
	} else {
		if (137 + desp + 1 == 138) {
			men.tipo = 274;
			sprintf(men.info, "%s", "Carta Recibida");
			//fprintf(stderr, "Mensaje enviado there %ld\n", men.tipo);
			if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
				fprintf(stderr, "Imposible enviar mensaje %ld\n", men.tipo);
		} else {
			men.tipo = 137 + desp;
			sprintf(men.info, "%s", "Carta Recibida");
			//fprintf(stderr, "Mensaje enviado %ld\n", men.tipo);
			if (-1 == msgsnd(buzon, &men, sizeof(men.info), 0))
				fprintf(stderr, "Imposible enviar %ld\n", men.tipo);
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
	if (-1 == semop(semaforo, &opSem, 1))
		perrorExit(ERR_OP_SEM, "Error al hacer Signal\n");
}


/* Función de captura de la señal CTRL + C.  */
void salir()
{
	/* Ignoramos las nuevas señales.  */
	signal(SIGINT, SIG_IGN);
	int i;
	// int data;
	// char *puntero = NULL;

	/* Eliminamos los hijos.  */
	for (i = 0; i < 4; i++) {
		kill(pid_hijos[i], SIGINT);
	}

	if (msgctl (buzon, IPC_RMID, NULL) == -1)
		perrorExit(ERR_RM_BUZ, "Error en borrado de buzon");

	shmdt((void *)&puntero);
	if (shmctl(memoriaID, IPC_RMID, NULL) == -1)
		perrorExit(ERR_RM_MEM, "Problema al liberar la memoria");

	if (semctl(semaforo, 0, IPC_RMID) == -1)
		perrorExit(ERR_RM_SEM, "Error en el borrado de los semáforos");

	system("clear"); /* Bueno para borrar huellas... */

	/* Activamos de nuevo las señales.  */
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
