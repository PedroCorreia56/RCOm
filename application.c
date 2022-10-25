#include "application.h"


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

int checkBCC2(unsigned char *message, int sizeMessage)
{
  int i = 1;
  unsigned char BCC2 = message[0];
  for (; i < sizeMessage - 1; i++)
  {
    BCC2 ^= message[i];
  }
  if (BCC2 == message[sizeMessage - 1])
  {
    return TRUE;
  }
  else
    return FALSE;
}


int llopen(char *port,int connection) {
    
    // appL->fileDescriptor = iniciate_connection(port, flag);
    // appL->status = flag;
   // return iniciate_connection(port, flag);
   
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

       if (alarmFlag == FALSE){
            alarm(3); // Set alarm to be triggered in 3s
            alarmFlag = TRUE;           
        }
        if(su_frame_write(fd, A_E, C_SET) < 0)
                perror("set message failed\n");

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
    
        if(alarmCount ==   numTransmissions ){
            printf("erro\n");
            perror("Error establishing connection, too many attempts\n");
            return -1;
        }
        else 
            printf("UA from SET message recieved\n");
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

// MUDEI DE BUFFER PARA DATA PORQUE NÃO ME APETECEU TROCAR OS data NO CÓDIGO POR buffer
int llwrite(int fd, char *data, int length){
    //1º preparar buffer
    //fazer malloc
   // return i_frame_write(fd, A_E, length, buffer);
    //bff2 before stuffing
              
    int data_sent =0;
    int last_buffer_index=0;
    while (data_sent != length)
    {

        int data_available = 0;
        if(length - data_sent > MAX_DATA_SIZE)
            data_available = MAX_DATA_SIZE; 
        else 
            data_available = length - data_sent ;

    int data_to_add=data_available ;
    alarmFlag = FALSE;
    alarmCount = 0;
    unsigned char bcc2 = data[last_buffer_index];
    last_buffer_index++;
    printf("Intial bcc2: %x\n",bcc2);
    for(int i = 1; i < data_available; i++){
        bcc2 ^= data[last_buffer_index];
        last_buffer_index++;
    }
    printf("Final bcc2: %x\n",bcc2);
    unsigned char *framed_data = (unsigned char*)malloc(sizeof(unsigned char) * (data_available + 7));
    //byte stuffing
    unsigned char *stuffed_data = byte_stuffing(data+data_sent, &data_available);
    //put stuffed data into frame
    framed_data[0] = FLAG; 
    framed_data[1] = A_R;

    if(sequenceNumber)
        framed_data[2] =C_1;
    else 
        framed_data[2] =C_0;

    framed_data[3] = A_R^ sequenceNumber;

    int j = 4;
                
    for(int i = 0; i < data_available; i++){
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
    unsigned char buf[5];
   // int flag = FALSE;
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
        if( (written_length = write(fd, framed_data, data_available)) < 0){
                //printf("written_length  = %d\n\n\n ", written_length);
                perror("i frame failed\n");
            }
        while(alarmFlag && state != BCC_OK ){ 
            read(fd, &buf[state], 1);
            state_machine(buf, &state);
        }
        if(state == BCC_OK){
            alarm(0);
            data_sent+=data_to_add;
            break;
        }

    }
        if(alarmCount ==  numTransmissions){
            perror("Error sending i packet, too many attempts\n");
            return -1;
        }
        else{
            printf("RR from i message recieved\n");
        }
    
    }
    
    
    return data_sent;

}

unsigned char* llread(int fd, int *size){
   // return read_i_frame(fd, size);
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
                    data_couter++;
                    if(buffer == FLAG)
                        state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    data_couter++;
                    if(buffer == 0x01 || buffer == 0x03)
                        state = A_RCV;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else state = START;  
                    break;
                case A_RCV:
                    data_couter++;
                    if(buffer ==  sequenceNumber)
                        state = C_RCV;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case C_RCV:
                    data_couter++;
                    if(buffer == A_R ^ C_0 || buffer == A_R ^ C_1 )
                        state = DATA;
                    else if(buffer == 0x7e)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case DATA:
                    data_couter++;
                    if(buffer == FLAG){     //finished transmitting data
                        printf("received final flag!\n");
                        
                        temp = byte_destuffing(data_received, &data_size);  //data size starts in 0

                        unsigned char post_transmission_bcc2 = data_received[0];
                        printf("Intial post_transmission_bcc2: %x\n",post_transmission_bcc2);
                        for(int i = 1; i < data_size - 2; i++){
                            post_transmission_bcc2 ^= temp[i];
                        }
                        printf("Final post_transmission_bcc2: %x\n",post_transmission_bcc2);
                        unsigned char bcc2 = temp[data_size-1];
                        printf("BCC2 a comparar: %x  \n",bcc2);
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
    
    *size = data_size;
    printf("data size is %d\n", data_size);
    return temp;
}
//MUDEI NOME DE FLAG PARA CONECTION
//CHECKAR OS READ's
int llclose(int fd, int connection){
    //return terminate_connection(&fd, flag);
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
         if(su_frame_write(fd, A_E, C_DISC) < 0){
                sleep(3);
                perror("disc message failed\n");
            }
         //   alarm( timeout);
        
            while(alarmFlag && state != BCC_OK ){
                read(fd, &buf[state], 1);
                state_machine(buf, &state);
            }
            if(state == BCC_OK){
                alarm(0);
                break;
            }
    }
 
        if(alarmCount ==   numTransmissions){
            perror("Error establishing connection, too many attempts\n");
            return -1;
        }
        else{
            printf("DISC from DISC message recieved\n");
            su_frame_write(fd, A_E, C_UA);
        }
    }

    else if(connection == RECEIVER){
        printf("terminate connection(receiver) starting\n");
        while(state != BCC_OK){
            if (read(fd, &buf[state], 1) < 0) { // Receive SET message
            sleep(10);
                perror("Failed to read DISC message.");
            } else {
                state_machine(buf, &state);
            }
        }
        printf("DISC recieved!\n");

        if(su_frame_write(fd, A_E, C_DISC) < 0){    //write
            perror("ua message failed\n");
            return -1;
        }

        state = START;
        while(state != BCC_OK){
            if (read(fd, &buf[state], 1) < 0) { // Receive UA message
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
