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
#define ERR_CR_SEM (unsigned char)152
#define ERR_RM_SEM (unsigned char)153
#define ERR_OP_SEM (unsigned char)154
#define ERR_CR_MEM (unsigned char)156
#define ERR_RM_MEM (unsigned char)157
#define ERR_CR_HIJO (unsigned char)165
#define ERR_CAP_SIGN (unsigned char)170

#define TAM 304

/* Prototipos de función.  */
int inicio_falonso(int ret, int semAforos, char *z);
int luz_semAforo(int direcciOn, int color);
int inicio_coche(int *carril, int *desp, int color);
int avance_coche(int *carril, int *desp, int color);

void opSemaforo(int semaforo, int nSemaforo, int nSignal);

// void salir();
