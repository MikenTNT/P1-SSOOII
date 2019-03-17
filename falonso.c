#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>

#include <unistd.h>
#include <signal.h>

#include "falonso.h"

#define TAM 304

int inicio_falonso(int ret, int semAforos, char *z);

void doAll(int semaforo, int nSemaforo, int nSignal);
//void salir();

// struct semid_ds
// {
// 	int sem_nsems;
//     time_t sem_otime;
//     time_t sem_ctime;
// };

union semun {
	int             val;
	struct semid_ds *buf;
	ushort_t        *array;
};




int main(void) {

	int memoriaID, s1;
	char *puntero = NULL;



	//signal(SIGINT, salir);

	//Creamos los semaforos
	if (-1 == (s1 = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600))) {
		printf("No se pudo crear el semaforo");
		return -2;
	}
	struct semid_ds ssd;
	ssd.sem_nsems = 2;
	//Los inicializamos
	semctl(s1, 0, IPC_STAT, &ssd);
	semctl(s1, 1, IPC_STAT, &ssd);

	// Creamos la memoria compartida
	puts("Vamos a crear la memoria compartida");
	if((memoriaID = shmget(IPC_PRIVATE, TAM, 0666| IPC_CREAT)) == -1){
		printf("No se ha podido crear la memoria\n");
	}
	//Asociación del puntero a la memoria compartida
	puntero = shmat(memoriaID, NULL, 0);

	inicio_falonso(1, s1, puntero);


	//write(1, "msg", strlen[4]);
	return 1;
}


//Función que nos permite hacer signal y wait
void doAll(int semaforo, int nSemaforo, int nSignal){

	struct sembuf pedir;

	pedir.sem_num = nSemaforo;
	pedir.sem_op = nSignal;
	pedir.sem_flg = 0;

	if(semop(semaforo, &pedir, 1) == -1){
		perror(NULL);
		perror("Error al hacer Signal\n");
	}

}

// void salir(){

// 	signal(SIGINT,SIG_IGN);
// 	int i, data;

// 	// for (i = 0; i < 5; i++) {
// 	// 	kill(hijos[i], SIGINT);
// 	// }

// 	shmdt(&puntero);
// 	if((shmctl(memoriaID, IPC_RMID, NULL))==-1){
// 		printf("Problema al liberar la memoria\n");
// 	}

// 	if (semctl(s1, 0, IPC_RMID) == -1) {
// 		perror("Error en borrado de semáforo");
// 		exit(5);
// 	}

// 	system("clear"); /* Bueno para borrar huellas... */
// 	signal(SIGINT, salir);
// 	exit(0);

// }
