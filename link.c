#include "link.h"

unsigned int sequenceNumber = 0;   /*Número de sequência da trama: 0, 1*/
unsigned int timeout = 3;          /*Valor do temporizador: 1 s*/
unsigned int numTransmissions = 4; /*Número de tentativas em caso de falha*/


int alarmFlag = FALSE;
int alarmCount = 0;

void sig_handler(int signal){
 		
  printf("Alarm %d\n\n", alarmCount);
  alarmFlag=FALSE;
  alarmCount++; 
  printf("TOu no sig handle\n");
  //alarm(timeout);
}

void change_sequenceNumber(){
    if(sequenceNumber)
        sequenceNumber = 0;
    else sequenceNumber = 1;
}

unsigned char * byte_stuffing(unsigned char *packet, int *length){
    //so do data no i packet
    unsigned char *stuffed_packet = NULL;
    stuffed_packet = (unsigned char *)malloc( *length * 2);
    int j = 0;
    for(int i = 0; i < *length; i++){
        if(packet[i] == FLAG){
            stuffed_packet[j] = 0x7d;
            stuffed_packet[++j] = 0x5e;
        }
        else if(packet[i] == ESCAPE_OCTET){
            stuffed_packet[j] = 0x7d;
            stuffed_packet[++j] = 0x5d;
        }
        else stuffed_packet[j] = packet[i];

        j++;
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
    for (int i = 0; i < *length; i++) {
        if (packet[i] == ESCAPE_OCTET)
            destuffed_packet[j] = packet[++i] ^ 0x20;
        else
            destuffed_packet[j] = packet[i];

        j++;
    }
    *length = j;
    return destuffed_packet;

}

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

int i_frame_write(int fd, char a, int length, unsigned char *data) {
    //bff2 before stuffing
    alarmFlag = FALSE;
   alarmCount = 0;
    unsigned char bcc2 = data[0];
    for(int i = 1; i < length; i++){
        bcc2 ^= data[i];
       
    }
    unsigned char *framed_data = (unsigned char*)malloc(sizeof(unsigned char) * (length + 7));
    //byte stuffing
    unsigned char *stuffed_data = byte_stuffing(data, &length);
    //put stuffed data into frame
    framed_data[0] = FLAG; 
    framed_data[1] = a;  
    framed_data[2] = sequenceNumber; 
    framed_data[3] = a^ sequenceNumber;
    int j = 4;

    for(int i = 0; i < length; i++){
        framed_data[j] = stuffed_data[i];        //começa no buf[2]
        //printf("   bcc2: %x   ", stuffed_data[j]);
        j++;
    }
    framed_data[j+1] = bcc2;
    framed_data[j+2] = FLAG;

    //write frame
    int frame_length = j+2+1; //+1 bcd 0 index //j+3 porque adicionamos o bcc2 e a flag sem aumentar o j primeiro
    int written_length = 0;
    int state = START;
   // alarmCount = 0;
    unsigned char buf[5];
    int flag = FALSE;
    //////////////////////////////////////////
    //(fcntl(fd, F_SETFL, VTIME);
    ///////////////////////////////////////
     (void)signal(SIGALRM, sig_handler);
    printf("frame length = %d\n", frame_length);
    while(alarmCount <  numTransmissions)
    {
        if (alarmFlag == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmFlag = TRUE;
        }
        if( (written_length = write(fd, framed_data, frame_length)) < 0){
                //printf("written_length  = %d\n\n\n ", written_length);
                perror("i frame failed\n");
            }
       // alarm( timeout);
     //   flag = FALSE;
        while(alarmFlag && state != BCC_OK ){ 
            read(fd, &buf[state], 1);
            state_machine(buf, &state);
        }
        if(state == BCC_OK){
            alarm(0);
            break;
        }

    }
/*
    do{
            if( (written_length = write(fd, framed_data, frame_length)) < 0){
                //printf("written_length  = %d\n\n\n ", written_length);
                perror("i frame failed\n");
            }
            alarm( timeout);
            flag = FALSE;
            while(!alarmFlag && state != BCC_OK ){
                read(fd, &buf[state], 1);
                state_machine(buf, &state);
            }
            if(state == BCC_OK){
                alarm(0);
                break;
            }
            

        }
        while(alarmFlag && alarmCount <   numTransmissions);
*/
        if(alarmCount ==  numTransmissions){
            perror("Error sending i packet, too many attempts\n");
            return -1;
        }
        else{
            printf("RR from i message recieved\n");
        }
    return written_length;

}

unsigned char* read_i_frame(int fd, int *size_read){
    unsigned char *temp = NULL;
    int state = START;
    int data_size = 0;
    unsigned char buffer;
    //unsigned char *data_received = (unsigned char*)malloc(data_size);
    unsigned char data_received[12000];
    int all_data_received = FALSE;
    int data_couter = 0;
    int testCount = 0;
    while(!all_data_received){
        if(read(fd, &buffer, 1) < 0)
            perror("failed to read i frame\n");
        else{
            //ver estado
            switch(state){

                case START:
                    //printf("buffer: %x state :start\n", buffer);
                    //printf("data counter = %d\n", data_couter);
                    data_couter++;
                    if(buffer == FLAG)
                        state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    //printf("buffer: %x state :flag_rcv\n", buffer);
                    // printf("data counter = %d\n", data_couter);
                    data_couter++;
                    if(buffer == 0x01 || buffer == 0x03)
                        state = A_RCV;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else state = START;  
                    break;
                case A_RCV:
                    //printf("buffer: %x state :a_rcv\n", buffer);
                    // printf("data counter = %d\n", data_couter);
                    data_couter++;
                    //printf("sequenceNumber = %d\n", sequenceNumber);
                    if(buffer ==  sequenceNumber)
                        state = C_RCV;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case C_RCV:
                    //printf("buffer: %x state :c_rcv\n", buffer);
                    // printf("data counter = %d\n", data_couter);
                    data_couter++;
                    if(buffer == 0x01 ^ sequenceNumber)
                        state = DATA;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case DATA:
                    //printf("buffer: %x state :data\n",buffer);
                    // printf("data counter = %d\n", data_couter);
                    data_couter++;
                    //sleep(1);
                    //exit(1);
                    if(buffer == FLAG){     //finished transmitting data
                        printf("received final flag!\n");
                        
                        temp = byte_destuffing(data_received, &data_size);  //data size starts in 0

                        unsigned char post_transmission_bcc2 = data_received[0];
                        for(int i = 1; i < data_size - 2; i++){
                            post_transmission_bcc2 ^= temp[i];
                            //printf("   bcc2: %x   ", data_received[i]);
                        }
                        unsigned char bcc2 = temp[data_size-1];

                        if(bcc2 == post_transmission_bcc2){
                            printf("data packet received!\n");
                            all_data_received = TRUE;
                        }
                        else{
                            perror("BCC2 dont match in llread\n");
                            data_size = 0;
                        }
                    }
                    else{
                        //printf(" data size in cicle %d \n", data_size);
                        data_size++;
                        data_received[data_size - 1] = buffer;
                        //printf("data receive saved %x\n", data_received[data_size-1]);
                    }
                    break;
            }
        }
    }
    
    printf("receiver received packet!\n");
    unsigned char *final_array = (unsigned char*)malloc(sizeof(data_received));
    if(data_received[0])  
        su_frame_write(fd, A_R, C_RR);
    
    *size_read = data_size;
    printf("data size is %d\n", data_size);
    return temp;
}

int iniciate_connection(char *port, int connection)
{

    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[5];
  //  alarmCount = 0;
   // alarmFlag = FALSE;
    int i, sum = 0, speed = 0;

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(port, O_RDWR | O_NOCTTY );

    if (fd <0) {perror(port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { 
    
/* save current port settings */
    perror("tcgetattr");
    exit(-1);
    }
    
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0; /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
    }

    printf("New termios structure set\n");
    
    
     
     //Register signal handler
    int state = START;
    (void)signal(SIGALRM, sig_handler);
    if(connection == TRANSMITTER){
      //  int flag = TRUE;

        //re-send message if no confirmation~
      
    while(alarmCount <  numTransmissions){
    
       if (alarmFlag == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmFlag = TRUE;           
        }
         if(su_frame_write(fd, A_E, C_SET) < 0)
                perror("set message failed\n");
          //  break;
            //printf("alarm do c\n");
            //printf("%ld\n",read(fd, &buf[state], 1));
           // printf("%d fd\n",fd);
           // printf("%s buf\n", &buf[state]); 
           // alarm(3);
            //pause();
       // printf("Enviou su\n");     
      //  flag = FALSE;
        while(alarmFlag && state != BCC_OK){              
                if(&buf[state]==NULL) break;
                read(fd, &buf[state], 1);                        
                state_machine(buf, &state);
            }      
            
        if(state == BCC_OK){
            printf("no if\n");   
            alarm(0);
            break;
        }
    }
      /*  do{      	
            if(su_frame_write(fd, A_E, C_SET) < 0)
                perror("set message failed\n");
            //printf("alarm do c\n");
            //printf("%ld\n",read(fd, &buf[state], 1));
           // printf("%d fd\n",fd);
           // printf("%s buf\n", &buf[state]); 
            alarm(3);
            //pause();
            
            flag = FALSE;
            while(!alarmFlag && state != BCC_OK){              
                if(&buf[state]==NULL) break;
                read(fd, &buf[state], 1);                        
                state_machine(buf, &state);
            }
            
            
            if(state == BCC_OK){
                alarm(0);
                break;
            }
            

        }while(alarmFlag && alarmCount < numTransmissions);
        */
        if(alarmCount ==   numTransmissions ){
            printf("erro\n");
            perror("Error establishing connection, too many attempts\n");
            return -1;
        }
        else(printf("UA from SET message recieved\n"));

    }

    else if(connection == RECEIVER){
        while(state != BCC_OK){
            if (read(fd, &buf[state], 1) < 0) { // Receive SET message
                perror("Failed to read SET message.");
            } else {
                state_machine(buf, &state);
            }
        }
        printf("establish connection - SET recieved!\n");
        su_frame_write(fd, A_E, C_UA);
            
    }
    else {
        printf("invalid type of connection!\n");
        return -1;
    }
    return fd;
    
}

int terminate_connection(int *fd, int connection)
{
    char buf[5];
    alarmCount = 0;
    alarmFlag = FALSE;
    int state = START;
    if(connection == TRANSMITTER){
        int flag = TRUE;
        printf("terminate connection(transmitter) starting\n");
        //re-send message if no confirmation
        //send and check if recieved DISC msg
        while(alarmCount <  numTransmissions){
        if (alarmFlag == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmFlag = TRUE;
        }
         if(su_frame_write(*fd, A_E, C_DISC) < 0){
                sleep(3);
                perror("disc message failed\n");
            }
         //   alarm( timeout);
        
            while(alarmFlag && state != BCC_OK ){
                read(*fd, &buf[state], 1);
                state_machine(buf, &state);
            }
            if(state == BCC_OK){
                alarm(0);
                break;
            }
    }
        
        
       /* do{
            if(su_frame_write(*fd, A_E, C_DISC) < 0){
                sleep(3);
                perror("disc message failed\n");
            }
            alarm( timeout);
            flag = FALSE;
            while(!alarmFlag && state != BCC_OK ){
                read(*fd, &buf[state], 1);
                state_machine(buf, &state);
            }
            if(state == BCC_OK){
                alarm(0);
                break;
            }
            

        }
        while(alarmFlag && alarmCount <   numTransmissions);
*/
        if(alarmCount ==   numTransmissions){
            perror("Error establishing connection, too many attempts\n");
            return -1;
        }
        else{
            printf("DISC from DISC message recieved\n");
            su_frame_write(*fd, A_E, C_UA);
        }
    }

    else if(connection == RECEIVER){
        printf("terminate connection(receiver) starting\n");
        while(state != BCC_OK){
            if (read(*fd, &buf[state], 1) < 0) { // Receive SET message
            sleep(10);
                perror("Failed to read DISC message.");
            } else {
                state_machine(buf, &state);
            }
        }
        printf("DISC recieved!\n");

        if(su_frame_write(*fd, A_E, C_DISC) < 0){    //write
            perror("ua message failed\n");
            return -1;
        }

        state = START;
        while(state != BCC_OK){
            if (read(*fd, &buf[state], 1) < 0) { // Receive UA message
                perror("Failed to read SET message.");
            } else {
                state_machine(buf, &state);
            }
        }
        printf("UA recieved!\n");
            
    }
    else {
        printf("invalid type of connection!\n");
        return -1;
    }

    return 1;

}



