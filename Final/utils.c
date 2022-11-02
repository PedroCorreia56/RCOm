#include "utils.h"


unsigned char * byte_stuffing(unsigned char *packet, int *length){
    //so do data no i packet
    unsigned char *stuffed_packet = NULL;
    stuffed_packet = (unsigned char *)malloc( *length * 2);
    int j = 0;
    for(int i = 0; i < *length; i++,j++){
        if(packet[i] == FLAG){
            stuffed_packet[j] = 0x7d;
            stuffed_packet[++j] = 0x5e;
        }
        else if(packet[i] == ESCAPE_OCTET){
            stuffed_packet[j] = 0x7d;
            stuffed_packet[++j] = 0x5d;
        }
        else stuffed_packet[j] = packet[i];

    //printf("data = %x, stuffed data = %x\n",  packet[i], stuffed_packet[i] );
    }
    *length = j;
    return stuffed_packet;

}

////////////////////////////////////////////////////
unsigned char * byte_destuffing(unsigned char *packet, int *length){
    //so do data no i packet
    unsigned char *destuffed_packet = NULL;
    destuffed_packet = (unsigned char *)malloc(*length * 2);
    int j = 0;
    // ALGUMA COIsA METER j++ Na CONDICAO DO FOR
    for (int i = 0; i < *length; i++) {
       
        if (packet[i] == ESCAPE_OCTET && packet[i+1]== 0x5e){
             destuffed_packet[j] = 0x7e;
             i++;
        }
        else if(packet[i] == ESCAPE_OCTET && packet[i+1]== 0x5d){
            destuffed_packet[j] = 0x7d;
             i++;
        }
        else
            destuffed_packet[j] = packet[i];

        j++;
        
    }
   
    *length = j;
    return destuffed_packet;

}
////////////////////////////////////////////////////
int su_frame_write(int fd, char a, char c) {
    unsigned char buf[5];

    buf[0] = FLAG;  
    buf[1] = a;     
    buf[2] = c;     
    buf[3] = a ^ c; 
    buf[4] = FLAG;  
    int wr=write(fd, buf, 5);
    // printf("wr=%d\n",wr);   
    if(wr<0){
        perror("set message failed\n");
        return -1;
    }

   
    //if(wr < 0)
        return 0;
    //else return 0;
}



