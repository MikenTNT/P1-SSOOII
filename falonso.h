#define CARRIL_DERECHO       0
#define CARRIL_IZQUIERDO     1

#define HORIZONTAL   0
#define VERTICAL     1

#define NEGRO    0
#define ROJO     1
#define VERDE    2
#define AMARILLO 3
#define AZUL     4
#define MAGENTA  5
#define CYAN     6
#define BLANCO   7

int inicio_falonso(int ret, int semAforos, char *z);
int inicio_coche(int *carril, int *desp, int color);
int avance_coche(int *carril, int *desp, int color);
int cambio_carril(int *carril, int *desp, int color);
int luz_semAforo(int direcciOn, int color);
int pausa(void);
int velocidad(int v, int carril, int desp);
int fin_falonso(int *cuenta);
