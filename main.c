#include "application.h"

unsigned char * process_pic(char* path, int* size){
    FILE *f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    *size = ftell(f); //qts bytes tem o ficheiro
    unsigned char *data = (unsigned char*)malloc(*size+4);
    unsigned char *buffer = (unsigned char *)malloc(*size);

    fseek(f, 0, SEEK_SET);
    fread(buffer, 1, *size, f);
    fclose(f);

	data[0] = C_REJ;	// C
	data[1] = 0; // N
	data[2] = *size / 255;	// L2
	data[3] = *size % 255; // L1

	for (int i = 0; i < *size; i++) {
		data[i+4] = buffer[i];
	}
    printf("main- total file bytes = %d\n", *size);

	return data;
}


int main(int argc, char** argv)
{

    char *port;
    char *img;
    unsigned char control[100];

    int fd,res,length = 5;
    struct termios oldtio, newtio;
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
  
        if((fd = llopen(port, RECEIVER))){
            printf("after ua received\n");
            unsigned char *msg = NULL;
            msg = (unsigned char *)malloc(1000 * 2);
            unsigned char *msg_start;
            unsigned char *msg_end;
            printf("receiver reading first control packet\n");
            int control_size = 0;
            msg_start = llread(fd, &control_size);

            printf("receiver reading first data packet\n");
            if(msg_start[0] == REJ)
                printf("Received start\n");
              
            int size = 0;
            msg = llread(fd, &size);
            printf("receiver reading last control packet\n");
          msg_end = llread(fd, &control_size);
            
            llclose(fd, RECEIVER);
            //int number_frames = sizeof(*msg) / (MAX_SIZE - 6);
            printf("SIZE OF READ IS %d\n\n\n", size);
            FILE *f = fopen("return_file.gif", "w");
            for(int i = 4; i< size; i++){
                fputc(msg[i], f);
            }
            int written = fwrite(msg, sizeof(unsigned char), sizeof(msg), f);
            if (written == 0) {
                printf("Error during writing to file !");
            }
            fclose(f);
        
        }
    }
    else if(fd = llopen(port, TRANSMITTER)){ // Open comunications for transmitter
        unsigned char * buffer = process_pic(img, &length);
        if(length <= 5){ // demand at least a byte, the rest is the header
            printf("Error processing image\n");
            exit(1);
        }
        printf(" length in main is %d\n", length);
        //CONTROL packet
        control[0] = 2; // C_BEGIN
        control[1] = 0; // T_FILESIZE
        control[2] = 1;
        control[3] = length;
        int tempLength = length;
        int l1 = (length / 255) + 1;
        int i;
       // printf("Templlenght=%d\n",tempLength);
        for(i = 0; i < l1; i++){
        control[4+i] =  (tempLength >> 8*(i+1) & 0xff);   //so o ultimo byte do numero
      //  printf("control[4+%d]=%x\n",i,(tempLength >> 8*(i+1) & 0xff));
    }
        printf("starting transmission of CONTROL packet\n");
        //printf("fd = %d\n", fd);    
        if(llwrite(fd, control, i + 4)){    //escreve control packet  
            printf("starting transmission of I packet\n");
            if(llwrite(fd, buffer, length)){
                control[0] = 3;
                printf("starting transmission of last CONTROL packet\n");
                llwrite(fd, control, 4);
            }
        }

        llclose(fd, TRANSMITTER);
    } 
    
    if (tcsetattr(fd, TCSAFLUSH, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);

	return 0;
}

