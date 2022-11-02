#include "datalink.h"


struct termios oldtio, newtio;
int process_pic(char* path, int* size){
    
    int fd_file = open(path, O_RDONLY);
   
    struct stat buf;
	if (fstat(fd_file, &buf) == -1) {
		printf("ERROR: fstat");
		return -1;
	}
    *size = buf.st_size;

    printf("main- total file bytes = %d\n", *size);

	return fd_file;
}


int main(int argc, char** argv)
{

    char *port;
    char *img;
   

    int fd,res,file_fd,length = 5;
    
    char buf[255];

     if(argc < 2 || argc > 3){
    	printf("Invalid Usage:\tInvalid number of arguments\n");
        exit(1);
    }
  
    if(strcmp("/dev/ttyS0", argv[1]) == 0 || strcmp("/dev/ttyS5", argv[1]) == 0 || 
    strcmp("/dev/ttyS10", argv[1]) == 0 || strcmp("/dev/ttyS11", argv[1]) == 0 )
            port = argv[1];
    else{
    	printf("Invalid Usage:\tInvalid Port ex: /dev/ttyS0\n");
    	exit(1);
    }

    if(argc==3){
    	  
    	int accessableImg = access(argv[2], F_OK);
			if (accessableImg == 0) {
				img = argv[2];
			}
			else{
				printf("Invalid Usage:\tInvalid Image");
        		exit(1);
    		}	 	
    }
     
    if(argc == 2){ // Open comunications for receiver
  
        if((fd = llopen(port, RECEIVER))>0){
            printf("after ua received\n");
            unsigned char *msg ;//= NULL;
           // msg = (unsigned char *)malloc(1000 * 2);
            unsigned char *msg_start;
            unsigned char *msg_end;
            printf("receiver reading first control packet\n");
            int control_size = 0;
            msg_start = llread(fd, &control_size);
            if(msg_start==NULL){
                perror("Error reading control packet, its NULL\n");
                return -1;
            }
            printf("msg_start[0])=%x\n",msg_start[0]);
            printf("msg_start[1])=%x\n",msg_start[1]);
            printf("msg_start[2])=%x\n",msg_start[2]);
            int file_received_length=0;

           // char * file_received_name;
           
            int new_file_fd=analyseControlPacket(msg_start, &file_received_length);
            printf("receiver reading first data packet\n");
            
            if(msg_start[0] == REJ)
                printf("Received start\n");
           // printf("file_received_length=%d\n",file_received_length);
          //  printf("file_received_name= %s\n",file_received_name);
            int size = 0;
           
           printf("Start reading DATA PACKETS\n");
           int received_file=0;
           int bytesread=0;
           int data_packets_read=0;
           while(!received_file){
                msg = llread(fd, &size);
                printf("msg[0]= %x\n", msg[0]);
                if(msg[0]==0x01){
                    printf("Data Packet Received\n");
                     printf("Writing data packet to file\n");
            
                    int dataSize = 256 * msg[2] + msg[3];
                    printf("Packet size: %d bytes\n\n", dataSize);

                    write(new_file_fd, &msg[4], dataSize);
                    bytesread+=size;
                    data_packets_read++;
                }
                else if(msg[0]==3){
                    received_file=1;
                    printf("Read Last CONTROL PACKET\n");
                    if(analyseControlPacket(msg, &file_received_length)<0){
                        printf("entrou no if\n");
                        return -1;
                    }
                    
                }
            }
            
                printf("Bytes received = %d\n",bytesread);
                 printf("Data packets received= %d\n",data_packets_read);
        
            
            llclose(fd, RECEIVER);
            //int number_frames = sizeof(*msg) / (MAX_SIZE - 6);
            printf("SIZE OF READ IS %d\n\n\n", size);
            
        
        }
    }
    else if((fd = llopen(port, TRANSMITTER)) > 0){ // Open comunications for transmitter
        file_fd= process_pic(img, &length);
        if(length <= 5){ // demand at least a byte, the rest is the header
            printf("Error processing image\n");
            exit(1);
        }
        printf(" length in main is %d\n", length);
        int index=0;
        //CONTROL packet 
        unsigned char control[5+sizeof(length)+strlen(img)];
        control[index++] = 2; // C_BEGIN
        control[index++] = 0; // T_FILESIZE
        control[index++] = sizeof(length);
    //    printf("  control[2] %x\n", control[2]);
       // printf("Templlenght=%d\n",tempLength);
        for(int i = 0; i < sizeof(length); i++){
        control[index++] = (length >> 8*(4 - 1 - i)) & 0xFF; 
        //(tempLength >> 8*(i+1) & 0xff);   //so o ultimo byte do numero
      }
        control[index++]=1; // T_FILENAME
        control[index++]=strlen(img);
        for(int u =0;u<strlen(img);u++ ){
            control[index++]=img[u];
        }

        printf("starting transmission of CONTROL packet\n");
        //printf("fd = %d\n", fd);    
        if(llwrite(fd, control, index)<=0){    //escreve control packet  
            printf("ERROR sending the first control packet!\n");
		    return -1;
        }
        printf("starting transmission data packets:\n");
        if(create_data_packet(fd,length,file_fd)==-1){
            printf("ERROR sending the the data packets!\n");
		    return -1;
        }

        control[0] = 3;
        printf("starting transmission of last CONTROL packet\n");
        if(llwrite(fd, control, index)<=0){
            printf("ERROR sending the last CONTROL packet!\n");
		    return -1;
        }
        llclose(fd, TRANSMITTER);
    } 
   // tcflush(fd,TCIOFLUSH);
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);

	return 0;
}

