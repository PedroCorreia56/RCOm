#include "statemachine.h"



#define MODEMDEVICE_0 "/dev/ttyS0"
#define MODEMDEVICE_1 "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FLAG 0x7e
#define ESCAPE_OCTET   0X7d
#define A_R     0x03    //Comandos enviados pelo Emissor e Respostas enviadas pelo Receptor
#define A_E     0x01    //Comandos enviados pelo Receptor e Respostas enviadas pelo Emissor
#define REJ     2

#define DATA 4




unsigned char * byte_stuffing(unsigned char *packet, int *length);

unsigned char* byte_destuffing(unsigned char *packet, int *length);

int su_frame_write(int fd, char a, char c);

