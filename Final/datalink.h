#include "utils.h"


#define C_DATA  0
#define C_START 1
#define C_END   2

#define TRANSMITTER 1
#define RECEIVER 0

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */


#define WRITE_SIZE 1024
#define MAX_DATA_SIZE ((WRITE_SIZE - 6)/2)
/*-------------------------------------------------------------------------------*/


int create_data_packet(int fd, int file_size, int file_fd);

int llopen(char *port,int flag);

int llwrite(int fd, char * buffer, int length);

unsigned char* llread(int fd, int *size);

int llclose(int fd, int flag);

int analyseControlPacket(unsigned char *packet, int *file_received_length);
