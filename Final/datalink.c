#include "datalink.h"


unsigned int sequenceNumber = 0;   /*Número de sequência da trama: 0, 1*/
unsigned int timeout = 3;          /*Valor do temporizador: 1 s*/
unsigned int numTransmissions = 4; /*Número de tentativas em caso de falha*/

char* file_received_name;
int alarmFlag = FALSE;
int alarmCount = 0;

extern struct termios oldtio,newtio;
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

int create_data_packet(int fd, int file_size, int file_fd){
    int real_data_size = MAX_DATA_SIZE-4;
	char buf[real_data_size];
	int packetsSent = 0;
	int packetsToSend = file_size / real_data_size + (file_size % real_data_size != 0);
	int bytesRead = 0;

	int bytesWritten = 0;
	int totalBytesWritten = 0;

	printf("File size: %d\n", file_size);
	printf("Packets Sent: %d\n", packetsToSend);


	while (packetsSent < packetsToSend){
		bytesRead = read(file_fd, &buf, real_data_size);
		unsigned char packet[4 + bytesRead];

		packet[0] = 0x01;
		packet[1] = packetsSent % 255;
		packet[2] = bytesRead / 256;
		packet[3] = bytesRead % 256;
		memcpy(&packet[4], &buf, bytesRead);

		bytesWritten = llwrite(fd, packet, bytesRead + 4);
		if (bytesWritten == -1){
			printf("ERROR in llwrite!\n");
			return -1;
		}

		totalBytesWritten += bytesWritten;
		packetsSent++;
	}

	printf("Wrote %d bytes\n", totalBytesWritten);
	printf("Wrote %d data bytes\n", totalBytesWritten - 4 * packetsSent);
	return 0;
}


int llopen(char *port,int connection) {
    
   
    int fd,c, res;
    
    char buf[5];
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
                unsigned char ctr;                      
                state_machine(buf, &state, &ctr);
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
                unsigned char ctr;
                state_machine(buf, &state, &ctr);
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
   
              
    int data_sent =0;

    alarmFlag = FALSE;
    alarmCount = 0;
    unsigned char bcc2 = data[0];
    
   // printf("Intial bcc2: %x\n",bcc2);
    for(int i = 1; i < length; i++){
        //printf("%x\n",data[last_buffer_index]);
         //printf("data[last_buffer_index]=%x\n",data[last_buffer_index]);
        bcc2 ^= data[i];
    }
  //  printf("Final bcc2: %x\n",bcc2);
    unsigned char *framed_data = (unsigned char*)malloc(sizeof(unsigned char) * (2*length + 6));
    //byte stuffing
    unsigned char *stuffed_data = byte_stuffing(data, &length);
    //put stuffed data into frame
    framed_data[0] = FLAG; 
    framed_data[1] = A_R;

    if(sequenceNumber)
        framed_data[2] =C_1;
    else 
        framed_data[2] =C_0;

    framed_data[3] = A_R^ sequenceNumber;

    int j = 4;
                
    for(int i = 0; i < length; i++){
        framed_data[j] = stuffed_data[i];        //começa no buf[2]
   //     printf("bcc2: %x\n", framed_data[j]);
        j++;
    }
    framed_data[j+1] = bcc2;
   // printf(" framed_data[j+1] = %x\n", framed_data[j+1]);
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
    unsigned char ctrl_c;
    while(alarmCount <  numTransmissions)
    { 
        if (alarmFlag == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmFlag = TRUE;
        }
        if( (written_length = write(fd, framed_data, frame_length)) < 0){
                printf("written_length  = %d\n\n\n ", written_length);
                perror("i frame failed\n");
            }
        while(alarmFlag && state != BCC_OK ){ 
           
            //printf("Debugg\n");
            if(read(fd, &buf[state], 1)>0)
            state_machine(buf, &state,&ctrl_c);
        }
        if(state == BCC_OK){
            alarm(0);
            
            if (ctrl_c == C_RR0 && sequenceNumber == 1) {
                printf("RR received: %d\n", sequenceNumber);
              //  dataLink.stats.numReceivedRR++;
              change_sequenceNumber();
                break;
            }
            else if (ctrl_c == C_RR1 && sequenceNumber == 0) {
                printf("RR received: %d\n",sequenceNumber);
              //  dataLink.stats.numReceivedRR++;
                change_sequenceNumber();
                break;
            }
            else if (ctrl_c == C_REJ0 && sequenceNumber == 1) {
                printf("REJ received: %d\n", sequenceNumber);
                state = START;
                //continue;
            }
            else if (ctrl_c == C_REJ1 && sequenceNumber == 0) {
                printf("REJ received: %d\n", sequenceNumber);
              //  dataLink.stats.numReceivedREJ++;
                state = START;
           //     continue;
            }
            //break;
        }

    }
        if(alarmCount ==  numTransmissions){
            perror("Error sending i packet, too many attempts\n");
            return -1;
        }
        //else{
       //     printf("RR from i message recieved\n");
       // }
    
    
    
    
    return written_length;

}

unsigned char* llread(int fd, int *size){
   
    char *temp = NULL;
    int state = START;
    int data_size = 0;
    unsigned char buffer;
    char data_received[WRITE_SIZE];
    int all_data_received = FALSE;
    int data_couter = 0;
    int testCount = 0;
    unsigned char control_c;
    int error_in_bcc1;
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
                    if(buffer ==  C_0 || buffer==C_1){
                        control_c=buffer;
                        state = C_RCV;
                    }
                        
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
                    else{
                        state = START;
                    }
                    break;
                case DATA:
                    data_couter++;
                    //printf("%x\n", buffer);
                    if(buffer == FLAG){     //finished transmitting data
                        printf("received final flag!\n");
                        
                        temp = byte_destuffing(data_received, &data_size);  //data size starts in 0
                        
                        unsigned char post_transmission_bcc2 = data_received[0];
                       // printf("Intial post_transmission_bcc2: %x\n",post_transmission_bcc2);
                        
                        for(int i = 1; i < data_size - 1; i++){
                            
                            //printf("%x\n", temp[i]);
                            //printf("temp[i] = %x\n", temp[i]);
                            post_transmission_bcc2 ^= temp[i];
                        }
                        printf("\n\ndata size = %d\n", data_size);
                        //printf("size = %ld\n", sizeof(data_received));
                      //  printf("Final post_transmission_bcc2: %x\n",post_transmission_bcc2);
                        unsigned char bcc2 = temp[data_size-1];
                      //  printf("BCC2 a comparar: %x  \n",bcc2);
                        if(bcc2 == post_transmission_bcc2){
                            printf("data packet received!\n");
                            all_data_received = TRUE;
                        }
                        else{
                            perror("BCC2 dont match in llread\n");
                            data_size = 0;
                            
                            if (control_c == C_0){
                               // dataLink.stats.numSentREJ++; 
                               su_frame_write(fd, A_R, C_REJ1);
                               printf("REJ sent: 1\n");
                            }
                            else if (control_c == C_1){
                                //dataLink.stats.numSentREJ++;
                                su_frame_write(fd, A_R, C_REJ0);
                                printf("REJ sent: 0\n"); 
                            }

                            *size=0;
                            temp[0]=0;
                            return temp;
                        }
                    }
                    else{
                        //printf(" data size in cicle %d \n", data_size);
                        data_size++;
                        data_received[data_size - 1] = buffer;
                        //printf("buffer = %x\n", buffer);
                        //printf("data receive saved %x\n", data_received[data_size-1]);
                    }
                    break;
            }
        }
    }
    printf("receiver received packet!\n");
    unsigned char *final_array = (unsigned char*)malloc(sizeof(data_received));
    if(control_c != C_0 && control_c!=C_1){
         perror("Error in protocol\n");            
            if (control_c == C_0){
                su_frame_write(fd, A_R, C_REJ1);
                printf("REJ sent: 1\n");
            }
             else if (control_c == C_1){
                su_frame_write(fd, A_R, C_REJ0);
                printf("REJ sent: 0\n"); 
            }
            *size=0;
            temp[0]=0;
            return temp;
    }

    if (control_c == C_0){
        su_frame_write(fd, A_R, C_RR1);
		printf("RR sent: 1\n");
	}
	else if (control_c == C_1){
        su_frame_write(fd, A_R, C_RR0);
		printf("RR sent: 0\n");
	}
    
    *size = data_size;
    //printf("data size is %d\n", data_size);
    return temp;
}


int llclose(int fd, int connection){
    
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
                unsigned char ctrl_c;
                state_machine(buf, &state,&ctrl_c);
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
                unsigned char ctr;
                state_machine(buf, &state, &ctr);
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
                unsigned char ctr;
                state_machine(buf, &state, &ctr);
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


int analyseControlPacket(unsigned char *packet, int *file_received_length){

	int index = 1;
	int file_size = 0;
	char* file_name;
    int flag1 = 0, flag2 = 0;
	

	printf("\n***File info:***\n\n");

	//FILE SIZE
	if(packet[index] == 0){
		index++;
       // int file_size = 0;
		int size_length = packet[index];
		index++;

		for (int i = 0; i < size_length; i++){
			file_size += packet[index] << 8 * (size_length - 1 - i);
			index++;
		}

        printf("size length = %x\n", size_length);
		if (*file_received_length != 0){
			if(*file_received_length  == file_size){
                flag1=1;
                printf("Start Control Packet and End Control Packet have the same size\n");
			}
            else{
                printf("Start Control Packet and End Control Packet data does not match\n");
            }
                
		}

		printf("File size: %d bytes\n", file_size);
	}
	if (file_size <= 0) {
		perror("File size error\n");
		return -1;
	}

	*file_received_length= file_size;

	//FILE NAME
	if (packet[index] == 1) {
		index++;
		int name_length = packet[index];
		index++;

		file_name = (char*) malloc(name_length + 1);
		for (int i = 0; i < name_length; i++) {
			file_name[i] = packet[index];
			index++;
		}

		file_name[name_length] = '\0';

		if (file_received_name != NULL){
			if(strcmp(file_name,file_received_name)==0){
				printf("Start Control Packet and End Control Packet have the same name\n");
                flag2=1;
            }
            else{
                printf("Start Control Packet and End Control Packet data does not match\n");
                return -1;
            }
		}

		printf("File Name: %s\n\n", file_name);
        if(file_received_name==NULL){
            file_received_name=(char*) malloc(name_length + 1);
            for (int i = 0; i < name_length; i++) {
			    file_received_name[i] = file_name[i];
		    } 
            file_received_name[name_length]='\0';
        }
        
	}

	

	int file_fd = open("return.gif", O_WRONLY | O_CREAT | O_APPEND);
    if(file_fd <0 && flag1==1 && flag2==1)
        return 1;
	return file_fd;
}
