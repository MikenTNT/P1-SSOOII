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
#define ERR_GEN (unsigned char)150  /* Error genérico.  */
#define ERR_ARGS (unsigned char)151  /* Error de argumentos.  */
#define ERR_CAP_SIGN (unsigned char)152  /* Error captura de señal.  */
#define ERR_CR_MEM (unsigned char)152  /* Error crear memoria compartida.  */

/* Constantes de los semáforos.  */
#define SEM_PADRE 1
#define SEM_HIJOS 2
#define SEM_3 3
#define SEM_4 4

#define NUM_HIJOS 25
#define TAM_MEMO sizeof(memoria)
#define TAM_MENS sizeof(pMemoria->men.info)


/* Prototipos de función.  */
void inicioSemaforo(int *semaforo, int nSemaforo, int val);
void opSemaforo(int semaforos, int nSemaforo, int nSignal);
void perrorExit(int code, char *msg);
void finPadre(int seNial);
void finHijos(int seNial);

void leerMensaje(int carril, int desp);
void enviarMensaje(int carril, int desp);
int puedoAvanzar(int carril, int desp);
int puedoCambioCarril(int carril, int desp, int color);
void mandarMensajePosicion(int carril, int desp);
void mandarMensajeCambio(int carril, int desp);
int esperarParaAvance(int carril, int desp);
int verticalVacio();
int horizontalVacio();
int leerVerticalCruce();
int enviarVerticalCruce();
int leerHorizontalCruce();
int enviarHorizontalCruce();


/* Definicion de tipos de datos.  */
typedef struct tipo_mensaje {
	long tipo;
	char info[20];
} mensaje;

typedef struct memoriaCompartida {
	char cDer[137];  /* 0-136 */
	char cIzq[137];  /* 137-273 */
	char semH;  /* 274 */
	char semV;  /* 275 */
	char reservado[25];  /* 276-300 */

	int semaforos;
	int buzon;
	pid_t pid_hijos[NUM_HIJOS];
	mensaje men;

	int vueltasCoches;
} memoria;


/* Variables globales.  */
int idMemoria = -1;
memoria *pMemoria = NULL;


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
	struct sigaction parada;
	int i;  /* Variable de iteración.  */
	int carril;  /* Carril del coche.  */
	int desp;  /* Posición del coche.  */
	int color;  /* Color del coche.  */
	int velo;  /* Velocidad del coche.  */
	int tempC, tempD;
	pid_t pidPadre = getpid();  /* PID del padre.  */
	pid_t pidHijo;  /* PID del hijo.  */


	/* Inicia y comprueba la creacíon de memoria compartida.  */
	if (-1 == (idMemoria = shmget(IPC_PRIVATE, TAM_MEMO, 0666 | IPC_CREAT))) {
		perrorExit(ERR_CR_MEM, "No se ha podido crear la memoria");
	}

	/* Asociación del puntero a la memoria compartida.  */
	if ((void *)-1 == (pMemoria = (memoria *)shmat(idMemoria, NULL, 0))) {
		perror("No se ha podido asignar el puntero");
		raise(SIGINT);
	}

	/* Inicializamos los valores de la memoria.  */
	pMemoria->semaforos = -1;
	pMemoria->buzon = -1;
	for (i = 0; i < NUM_HIJOS; i++)
		pMemoria->pid_hijos[i] = -1;
	sprintf(pMemoria->men.info, "Carta Recibida");
	pMemoria->vueltasCoches = 0;


	/* Inicia y comprueba la creacíon del array de semáforos.  */
	if (-1 == (pMemoria->semaforos = semget(IPC_PRIVATE, 5, 0600 | IPC_CREAT))) {
		perror("No se pudo crear el semaforos");
		raise(SIGINT);
	}

	/* Creación del buzon.  */
	if (-1 == (pMemoria->buzon = msgget(IPC_PRIVATE, 0600 | IPC_CREAT))) {
		perror("No se ha podido crear el buzón");
		raise(SIGINT);
	}


	/* Damos valor a los semáforos.  */
	inicioSemaforo(&(pMemoria->semaforos), SEM_PADRE, 0);
	inicioSemaforo(&(pMemoria->semaforos), SEM_HIJOS, 0);
	inicioSemaforo(&(pMemoria->semaforos), SEM_3, 1);
	inicioSemaforo(&(pMemoria->semaforos), SEM_4, 6);


	/* Enviar mensaje a todas las posiciones, sumando 1 para no enviar al 0.  */
	for (i = 0; i < 274; i++) {
		pMemoria->men.tipo = i + 1;
		if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0))
			printf("Imposible enviar mensaje\n");
	}

	/* Iniciamos el circuito.  */
	inicio_falonso(atoi(argv[2]), pMemoria->semaforos, (char *)pMemoria);


	/* Creamos a los hijos/coches.  */
	for (i = 0; i < atoi(argv[1]); i++) {
		switch (pMemoria->pid_hijos[i] = fork()) {
			case -1:
				perror("Error al crear a los hijos");
				raise(SIGINT);
			case 0:
				parada.sa_handler = &finPadre;
				parada.sa_flags = SA_RESTART;
				sigfillset(&parada.sa_mask);

				/* Comprobación de la captura de la señal SIGTERM  */
				if (-1 == sigaction(SIGUSR1, &parada, NULL)) return -1;

				pidHijo = getpid();
				srand(pidHijo);

				/* Establecemos las variables del coche.  */
				carril = pidHijo % 2;
				desp = (i / 2) ;
				velo = 1 + (rand() % 99);
				if (4 == (color = pidHijo % 8))
					color ++;

				if (carril == 1)
					pMemoria->men.tipo = desp + 137 + 1;
				else
					pMemoria->men.tipo = desp + 1;

				if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, pMemoria->men.tipo, 0)) {
					perror("Error de avance");
					raise(SIGINT);
				}

				if (-1 == inicio_coche(&carril, &desp, color)) {
					perror("Error de creacion del coche");
					raise(SIGINT);
				}

				/* Esperamos a que todos los hijos estén creados.  */
				opSemaforo(pMemoria->semaforos, SEM_PADRE, 1);
				opSemaforo(pMemoria->semaforos, SEM_HIJOS, -1);

				while (1) {
					if (puedoAvanzar(carril, desp)) {
						velocidad(velo, carril, desp);

						/*
						 * Hacemos atómica el avanzar y enviar mensaje
						 *  a la posición anterior.
						 */
						opSemaforo(pMemoria->semaforos, SEM_3, -1);

						avance_coche(&carril, &desp, color);
						/* Enviamos mensaje a la posicion que dejamos libre.  */
						mandarMensajePosicion(carril, desp);

						if (((carril == 0) && (desp == 133)) || ((carril == 1) && (desp == 131)))
							(pMemoria->vueltasCoches)++;

						opSemaforo(pMemoria->semaforos, SEM_3, 1);
					}
					else {
						if (puedoCambioCarril(carril, desp, color)) {
							opSemaforo(pMemoria->semaforos, SEM_3, -1);

							tempC = carril;
							tempD = desp;
							cambio_carril(&carril, &desp, color);
							mandarMensajeCambio(tempC, tempD);

							if (((carril == 0) && (desp == 133)) || ((carril == 1) && (desp == 131)))
								(pMemoria->vueltasCoches)++;

							opSemaforo(pMemoria->semaforos, SEM_3, 1);
						}
						else {
							if (esperarParaAvance(carril,desp)) {
								velocidad(velo, carril, desp);

								opSemaforo(pMemoria->semaforos, SEM_3, -1);

								avance_coche(&carril, &desp, color);
								/* Enviamos mensaje a la posicion que dejamos libre.  */
								mandarMensajePosicion(carril, desp);

								if (((carril == 0) && (desp == 133)) || ((carril == 1) && (desp == 131)))
									(pMemoria->vueltasCoches)++;

								opSemaforo(pMemoria->semaforos, SEM_3, 1);
							}
						}
					}
				}

				exit(0);
		}
	}

	if (getpid() == pidPadre) {
		parada.sa_handler = &finPadre;
		parada.sa_flags = SA_RESTART;
		sigfillset(&parada.sa_mask);

		/* Comprobación de la captura de la señal SIGTERM  */
		if (-1 == sigaction(SIGINT, &parada, NULL)) return -1;

		/* Esperamos a que todos los hijos estén creados.  */
		opSemaforo(pMemoria->semaforos, SEM_PADRE, -atoi(argv[1]));
		opSemaforo(pMemoria->semaforos, SEM_HIJOS, atoi(argv[1]));

		luz_semAforo(0, 1);
		leerMensaje(0, 105);
		leerMensaje(1, 97);
		if (leerHorizontalCruce()) {
			luz_semAforo(1, 2);
		}

		/* Sincronizacion de los semaforos y cambio de color de estos.  */
		while(1) {
			leerMensaje(0,20);
			leerMensaje(1,22);
			luz_semAforo(1,3);
			if (leerVerticalCruce()) {
				enviarHorizontalCruce();
				luz_semAforo(1,1);
				luz_semAforo(0,2);
			}
			enviarMensaje(0,105);
			enviarMensaje(1,97);
			sleep(1);

			leerMensaje(0,105);
			leerMensaje(1,97);
			if (leerHorizontalCruce()) {
				luz_semAforo(0,3);
			}

			if (enviarVerticalCruce()) {
				luz_semAforo(0,1);
				luz_semAforo(1,2);
			}
			enviarMensaje(0,20);
			enviarMensaje(1,22);
			sleep(1);
		}

		raise(SIGINT);
	}

	exit(0);

}


int puedoAvanzar(int carril, int desp)
{
	if (carril == 0) {
		/* CARRIL DERECHO CON SUS OPCIONES Y VALORES.  */
		if (desp == 136) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 1, IPC_NOWAIT))
				return 0;
		}
		else if (desp == 19 || desp == 104) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 2, 0)) {
				perror("Error de avance");
				raise(SIGINT);
			}
		}
		else {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 2, IPC_NOWAIT))
				return 0;
		}
	}
	else {
		/* CARRIL IZQUIERDO CON SUS OPCIONES Y VALORES.  */
		if ((desp + 137) == 273) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 138, IPC_NOWAIT))
				return 0;
		}
		else if (desp == 21 || desp == 96) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 2, 0)) {
				perror("Error de avance");
				raise(SIGINT);
			}
		}
		else {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 2, IPC_NOWAIT))
				return 0;
		}
	}
	return 1;
}


int puedoCambioCarril(int carril, int desp, int color)
{
	if (carril == 0) {
		if ((desp >= 0 && desp <= 13) || (desp >= 29 && desp <= 60)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 1, IPC_NOWAIT))
				return 0;
			// +0
		}
		else if (desp >= 14 && desp <= 28) {
			if (desp == 24) {
				if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 21, IPC_NOWAIT))
					return 0;
				pMemoria->men.tipo = 21;
				if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
					perror("Error al enviar mensaje");
					raise(SIGINT);
				}
			}
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 2, IPC_NOWAIT))
				return 0;
			// +1
		}
		else if ((desp >= 61 && desp <= 62) || (desp >= 135 && desp <= 136)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp, IPC_NOWAIT))
				return 0;
			// -1
		}
		else if ((desp >= 63 && desp <= 65) || (desp >= 131 && desp <= 134)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp - 1, IPC_NOWAIT))
				return 0;
			//-2
		}
		else if ((desp >= 66 && desp <= 67) || (desp == 130)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp - 2, IPC_NOWAIT))
				return 0;
			//-3
		}
		else if (desp == 68) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp - 3, IPC_NOWAIT))
				return 0;
			//-4
		}
		else if (desp >= 69 && desp <= 129) {
			if (desp == 104 || desp == 103 || desp == 102 || desp == 101) {
				//Control para que no se salten el semaforo
				if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 106, IPC_NOWAIT)) {
					// perror("Error de mensaje de cambio de carril");
					// raise(SIGINT);
					return 0;
				}
				pMemoria->men.tipo = 106;
				if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
					perror("Error al enviar mensaje");
					raise(SIGINT);
				}
			}

			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp - 4, IPC_NOWAIT))
				return 0;
			//-5
		}
	}
	else {
		if ((desp >= 0 && desp <= 15) || (desp >= 29 && desp <= 58)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 1, IPC_NOWAIT))
				return 0;
			// 0
		}
		else if (desp >= 16 && desp <= 28) {
			if (desp == 22 || desp == 21) {
				//NO saltarse el semaforo
				if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 23, IPC_NOWAIT))
					return 0;

				pMemoria->men.tipo = 23;
				if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
					perror("Error al enviar mensaje");
					raise(SIGINT);
				}
			}
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp , IPC_NOWAIT))
				return 0;
			// -1
		}
		else if (desp >= 59 && desp <= 60) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 2, IPC_NOWAIT))
				return 0;
			// +1
		}
		else if ((desp >= 61 && desp <= 62) || (desp >= 129 && desp <= 133)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 3, IPC_NOWAIT))
				return 0;
			// +2
		}
		else if (desp >= 127 && desp <= 128) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 4, IPC_NOWAIT))
				return 0;
			// +3
		}
		else if ((desp >= 63 && desp <= 64) || (desp == 126)) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 5, IPC_NOWAIT))
				return 0;
			// +4
		}
		else if (desp >= 65 && desp <= 125) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 6, IPC_NOWAIT))
				return 0;
			// +5
		}
		else if (desp >= 134 && desp <= 136) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137, IPC_NOWAIT))
				return 0;
			// 136
		}
	}

	return 1;
}


int esperarParaAvance(int carril, int desp)
{
	if (carril == 0) {
		/* CARRIL DERECHO CON SUS OPCIONES Y VALORES.  */
		if (desp == 136) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 1, 0)) {
				perror("Error al leer mensaje");
				raise(SIGINT);
			}
		}
		else {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 2, 0)) {
				perror("Error al leer mensaje");
				raise(SIGINT);
			}
		}
	}
	else {
		/* CARRIL IZQUIERDO CON SUS OPCIONES Y VALORES.  */
		if ((desp + 137) == 273) {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 138, 0)) {
				perror("Error al leer mensaje");
				raise(SIGINT);
			}
		}
		else {
			if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 2, 0)) {
				perror("Error al leer mensaje");
				raise(SIGINT);
			}
		}
	}

	return 1;
}


void leerMensaje(int carril, int desp)
{
	if (carril == 0) {
		if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, desp + 1, 0)) {
			perror("Error al leer mensaje");
			raise(SIGINT);
		}
	}
	else {
		if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + desp + 1, 0)) {
			perror("Error al leer mensaje");
			raise(SIGINT);
		}
	}
}


void enviarMensaje(int carril, int desp)
{
	if (carril == 0) {
		pMemoria->men.tipo = desp + 1;
		if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
			perror("Error al enviar mensaje");
			raise(SIGINT);
		}
	}
	else {
		pMemoria->men.tipo = 137 + desp + 1;
		if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
			perror("Error al enviar mensaje");
			raise(SIGINT);
		}
	}
}


void mandarMensajePosicion(int carril, int desp)
{
	if (carril == 0) {
		if (desp + 1 == 1) {
			pMemoria->men.tipo = 137;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			pMemoria->men.tipo = desp;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
	}
	else {
		if (137 + desp + 1 == 138) {
			pMemoria->men.tipo = 274;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			pMemoria->men.tipo = 137 + desp;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
	}
}


void mandarMensajeCambio(int carril, int desp)
{
	if (carril == 0) {
		if (desp + 1 == 1) {
			pMemoria->men.tipo = 1;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			pMemoria->men.tipo = desp + 1;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
	}
	else {
		if (137 + desp + 1 == 138) {
			pMemoria->men.tipo = 138;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
		else {
			pMemoria->men.tipo = 137 + desp + 1;
			if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
				perror("Error al enviar mensaje");
				raise(SIGINT);
			}
		}
	}
}


int verticalVacio()
{
	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 22, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 22;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 23, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 23;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 24, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 24;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 24, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 24;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 25, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 25;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 26, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 26;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	return 1;
}


int horizontalVacio()
{
	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 107, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 107;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 108, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 108;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 109, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 109;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 100, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 100;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 101, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 101;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 102, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}
	pMemoria->men.tipo = 137 + 102;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}
	return 1;
}


int leerVerticalCruce()
{
	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 22, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 23, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 24, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 24, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 25, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 26, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	return 1;
}


int enviarVerticalCruce()
{
	pMemoria->men.tipo = 22;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 23;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 24;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 24;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 25;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 26;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	return 1;
}


int leerHorizontalCruce()
{
	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 107, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 108, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 109, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 100, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 101, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	if (-1 == msgrcv(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 137 + 102, 0)) {
		perror("Error al leer mensaje");
		raise(SIGINT);
	}

	return 1;
}


int enviarHorizontalCruce()
{
	pMemoria->men.tipo = 107;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 108;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 109;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 100;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 101;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	pMemoria->men.tipo = 137 + 102;
	if (-1 == msgsnd(pMemoria->buzon, &(pMemoria->men), TAM_MENS, 0)) {
		perror("Error al enviar mensaje");
		raise(SIGINT);
	}

	return 1;
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
void finPadre(int seNial)
{
	/* Ignoramos las nuevas señales CTRL + C.  */
	signal(SIGINT, SIG_IGN);


	/* Eliminamos los hijos.  */
	int i;
	for (i = 0; i < NUM_HIJOS; i++) {
		if (pMemoria->pid_hijos[i] != -1) {
			kill(pMemoria->pid_hijos[i], SIGUSR1);
			wait(0);
		}
	}

	fin_falonso(&(pMemoria->vueltasCoches));


	/* Eliminamos los recursos IPC.  */
	if (pMemoria->semaforos != -1)
		if (semctl(pMemoria->semaforos, 0, IPC_RMID) == -1)
			perror("Error en el borrado de los semáforos");

	if (pMemoria->buzon != -1)
		if (msgctl(pMemoria->buzon, IPC_RMID, NULL) == -1)
			perror("Error en borrado de buzon");

	if ((pMemoria != NULL) || (pMemoria != (void *)-1))
		if (shmdt((void *)pMemoria) == -1)
			perror("No se ha podido eliminar el puntero de memoria");

	if (idMemoria != -1)
		if (shmctl(idMemoria, IPC_RMID, NULL) == -1)
			perror("No se ha podido liberar la memoria");


	/* Bueno para borrar huellas... */
	// system("clear");


	/* Activamos de nuevo las señales CTRL + C.  */
	signal(SIGINT, finPadre);


	exit(0);
}


void finHijos(int seNial)
{
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
