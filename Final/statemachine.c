#include "statemachine.h"

//state is the current state in the invoking function
//buf is the last char read in the invoking function
int state_machine(unsigned char *buf, int *state, unsigned char *ctrl_c ){

    switch (*state){
        case START:
            if(buf[START] == 0x7e){
                //printf("buffer: %x state :%d\n",buf[*state], *state);
                (*state)++;
            }
            break;

        case FLAG_RCV:
            //printf("buffer: %x state :%d\n",buf[*state], *state);
            if(buf[FLAG_RCV] == 0x03 || buf[FLAG_RCV] == 0x01){
                (*state)++;

            }
            else if(buf[FLAG_RCV] == 0x7e){
                //state = 1;
            }
            else {
                *state = START;
            }
            break;

        case A_RCV:
            //printf("buffer: %x state :%d\n",buf[*state], *state);
            if( buf[A_RCV] == C_SET || 
                buf[A_RCV] == C_UA || 
                buf[A_RCV] == C_DISC || 
                buf[A_RCV] == C_RR0 ||
                buf[A_RCV] == C_RR1 ||
                buf[A_RCV] == C_REJ0 ||
                buf[A_RCV] == C_REJ1 ){ //different options for each signal
                (*state)++;
               *ctrl_c = buf[A_RCV];

            }
            else if(buf[A_RCV] == 0x7e){
                *state = FLAG_RCV;
            }
            else{
                *state = START;
            }
            break;


        case C_RCV:
            //printf("buffer: %x state :%d\n",buf[*state], *state);
            if( buf[C_RCV] == (buf[1] ^ buf[2]) ) {
                (*state)++;
            }
            else if(buf[C_RCV] == 0x7e){
                *state = FLAG_RCV;
                buf[*state] = 0;
            }
            else{
                *state = START;
            }
            break;
    
        case BCC_OK:
            //printf("buffer: %x state :%d\n",buf[*state], *state);
            if(buf[BCC_OK] == 0x7e){
                printf("FIM\n");
                return TRUE;
            }
            else{
                *state = 0;

            }
            break;
    }
    return FALSE;
}