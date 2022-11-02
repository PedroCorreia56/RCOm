#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4


#define C_SET   0x03
#define C_DISC  0x0b
#define C_UA    0x07
#define C_RR0	0x05
#define C_RR1	0x85
#define C_REJ0	0x81
#define C_REJ1	0x01
#define C_0     0x00
#define C_1     0x40

int state_machine(unsigned char *buf, int *state, unsigned char *ctrl_c);