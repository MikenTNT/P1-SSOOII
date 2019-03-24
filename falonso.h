#ifndef __FALONSO_H
#define __FALONSO_H


/* Constantes del circuito.  */
#define CARRIL_DERECHO     0
#define CARRIL_IZQUIERDO   1

#define HORIZONTAL         0
#define VERTICAL           1

#define NEGRO              0
#define ROJO               1
#define VERDE              2
#define AMARILLO           3
#define AZUL               4
#define MAGENTA            5
#define CYAN               6
#define BLANCO             7


/* Constantes de la funcion exit().  */
#define ERR_GEN (unsigned char)150
#define ERR_CR_SEM (unsigned char)152
#define ERR_RM_SEM (unsigned char)153
#define ERR_SET_SEM (unsigned char)154
#define ERR_OP_SEM (unsigned char)155
#define ERR_CR_PIP (unsigned char)156
#define ERR_RM_PIP (unsigned char)157
#define ERR_CR_BUZ (unsigned char)158
#define ERR_RM_BUZ (unsigned char)159
#define ERR_SEND_BUZ (unsigned char)160
#define ERR_RECV_BUZ (unsigned char)161
#define ERR_CR_MEM (unsigned char)162
#define ERR_RM_MEM (unsigned char)163
#define ERR_SET_MEM (unsigned char)164
#define ERR_CR_HIJO (unsigned char)165
#define ERR_CAP_SIGN (unsigned char)170

#define TAM 304


/* Prototipos de función.  */
int inicio_falonso(int ret, int semAforos, char *z);
int inicio_coche(int *carril, int *desp, int color);
int avance_coche(int *carril, int *desp, int color);
int cambio_carril(int *carril, int *desp, int color);
int luz_semAforo(int direcciOn, int color);
int pausa(void);
int velocidad(int v, int carril, int desp);
int fin_falonso(int *cuenta);


void crearHijo();
void opSemaforo(int semaforo, int nSemaforo, int nSignal);
void perrorExit(int code, char *msg);
void salir();

void leerMensaje(int carril, int desp);
void enviarMensaje(int carril, int desp);
int puedoAvanzar(int carril, int desp);
void mandarMensajePosicion(int carril, int desp);
int puedoCambioCarril(int carril, int desp);


#endif
