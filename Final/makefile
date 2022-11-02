CC = gcc

CFLAGS = -g
#CFLAGS = -Wall -g

all: 
	@gcc ${CFLAGS} -o protocol datalink.c utils.c main.c statemachine.c 

clean:	
	@rm -f protocol
	@rm -f return.gif
	