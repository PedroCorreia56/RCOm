#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7E
#define A 0x03
#define C 0x03

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char buf[6];
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = A^C;
    buf[4] = FLAG;
    buf[5] = '\0';

	printf("buf=%s\n",buf);
    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';

    int bytes = write(fd, buf, 6);
    for(int i=0;i<6;i++) printf("%x\n", buf[i]);
    printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    sleep(1);
	

    unsigned char received[6];
    char c;
    int pos=0;
    bytes=0;
    int state=0;
    int i =0;
    int a_pos = 0;
    int c_pos = 0;
    int bcc_pos = 0;
    while (STOP == FALSE)
    {
        // Returns after 5 chars have been input
        bytes=read(fd, &c, 1);
        printf("bytes= %d\n", bytes);
        if(bytes>0){
            printf("c=%x\n",c);
            printf("pos=%d\n",pos);
            received[pos]=c;
            printf("buf[pos]=%x\n",received[pos]);
            pos++;
          
            switch (state)
            {
            case 0://
           
                if(c==FLAG){
                    
                    state=1;
                    i++;
                    printf("entrei no estado 1\n");
                }
                break;
            case 1:
                if(c!=FLAG){
                    state=2;
                    i++;
                    a_pos=pos-1;
                    printf("entrei no estado 2\n");
                }
                break;
            case 2:
                if(i==3){ 
                    bcc_pos=pos-1;
                    printf("li o bcc\n");
                    i++;
                }
                if(i==2){ 
                    c_pos=pos-1;
                    printf("li o c\n");
                     printf("buf[c_pos]=%x\n",received[c_pos]);
                    i++;
                }
                if(c==FLAG){
                    printf("Entrou no ultimo if\n");
                    printf("i=%d\nbuf[a_pos]=%x\nbuf[c_pos]=%x\n",i,received[a_pos],received[c_pos]);
                    if(i==4 && received[c_pos]==C && received[bcc_pos]==(received[a_pos]^received[c_pos])){
                     state=3;
                     printf("entrei no estado 3\n");
                    }
                    else {
                        i=0;
                        state=0;
                    }
                }
                break;
            
            case 3:
            printf("Tou no estado 3\n");
            if(c=='\0'){
            printf("Entrei no stop\n");
            printf("c=%x\n",c);
            STOP=TRUE;
                }
                if(c==FLAG){
                    i=0;
                    state=1;
                    i++;
                    printf("entrei no estado 1\n");
                }
            break;
               
            
            default:
                break;
            }
        }
       // buf[bytes] = '\0'; // Set end of string to '\0', so we can printf
        
    }
    
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
