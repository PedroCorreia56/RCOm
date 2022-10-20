#include "application.h"


int llopen(char *port,int flag) {
    
    // appL->fileDescriptor = iniciate_connection(port, flag);
    // appL->status = flag;
    return iniciate_connection(port, flag);
}


int llwrite(int fd, char *buffer, int length){
    //1º preparar buffer
    //fazer malloc
    return i_frame_write(fd, A_E, length, buffer);
}

unsigned char* llread(int fd, int *size){
    return read_i_frame(fd, size);
}

int llclose(int fd, int flag){
    return terminate_connection(&fd, flag);
}
