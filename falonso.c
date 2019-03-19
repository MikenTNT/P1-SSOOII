#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>  /* Funcion ftok().  */
#include <sys/sem.h>  /* Semáforos.  */
#include <sys/shm.h>
#include <sys/types.h>  /* Types pids.  */

#include <signal.h>  /* Tratamiento de señales.  */
#include <unistd.h>  /* Librería para fork().  */

#include "falonso.h"

/* Variables globales.  */
union semun {
	int val;
	struct semid_ds *buf;
	ushort_t *array;
} s;


int main(int argc, char const *argv[]) {

	// if (signal(SIGINT, salir) == SIG_ERR) {
	// 	perror("No se puede capturar la señal.");
	// 	exit(ERR_CAP_SIGN);
	// }

	int memoriaID, semaforo;
	char *puntero = NULL;
	pid_t pidPadre, pidHijo;
	int desp = 4;
	int carril = 0;

	/* Crea y calcula una clave para la función semget().  */
	key_t clave = ftok(argv[0], 'M');  /* IPC_PRIVATE.  */

	/* Inicia y comprueba la creacíon de un semáforo.  */
	if (-1 == (semaforo = semget(clave, 2, IPC_CREAT | 0600))) {
		perror("No se pudo crear el semaforo");
		exit(ERR_CR_SEM);
	}

	struct semid_ds ssd;
	//struct shmid_ds sds;
	ssd.sem_nsems = 2 ;
	s.buf = &ssd;

	/* Damos valor a los semáforos.  */
	semctl(semaforo, 0, IPC_STAT, s.buf);
	semctl(semaforo, 1, IPC_STAT, s.buf);

	/* Inicia y comprueba la creacíon de memoria compartida.  */
	puts("Vamos a crear la memoria compartida");
	if (-1 == (memoriaID = shmget(IPC_PRIVATE, TAM, IPC_CREAT | 0666))) {
		perror("No se ha podido crear la memoria");
		exit(ERR_CR_MEM);
	}

	/* Asociación del puntero a la memoria compartida.  */
	puntero = shmat(memoriaID, NULL, 0);


	pidPadre = getpid();
	switch (fork()) {
		case -1:
			perror("Error de creacion del hijo");
			exit(ERR_CR_HIJO);
		case 0:
			pidHijo = getpid();
			inicio_coche(&carril, &desp, 1);
			// puntero = shmat(memoriaID, NULL, 0); //Asociarse
			// shmctl(memoriaID, IPC_STAT, )
			// sprintf(puntero, "%d", 1);
			puts("Me he creado");
			break;
	}

	if (pidPadre == getpid()) {
		inicio_falonso(1, semaforo, puntero);

		while (1) {
			luz_semAforo(1,1);
			luz_semAforo(0,2);
			sleep(1);
			luz_semAforo(1,2);
			luz_semAforo(0,1);
			sleep(1);
		}
	}

	if (pidHijo == getpid()) {
		while (1) {
			avance_coche(&carril, &desp, 1);
		}
	}

	return 1;

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
		perror("Error al hacer Signal\n");
		exit(ERR_OP_SEM);
	}
}


/* Función de captura de la señal CTRL+C.  */
// void salir()
// {
// 	/* Ignoramos las nuevas señales.  */
// 	signal(SIGINT, SIG_IGN);

// 	int data;

// 	/* Eliminamos los hijos.  */
// 	// for (int i = 0; i < 5; i++) {
// 	// 	kill(hijos[i], SIGINT);
// 	// }

// 	shmdt(&puntero);
// 	if (shmctl(memoriaID, IPC_RMID, NULL) == -1) {
// 		perror("Problema al liberar la memoria");
// 	}

// 	if (semctl(s1, 0, IPC_RMID) == -1) {
// 		perror("Error en el borrado de los semáforos");
// 		exit(ERR_RM_SEM);
// 	}

// 	// system("clear"); /* Bueno para borrar huellas... */

// 	/* Activamos de nuevo las señales.  */
// 	signal(SIGINT, salir);

// 	exit(0);
// }
