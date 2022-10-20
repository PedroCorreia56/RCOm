CC = gcc

CFLAGS = -g
#CFLAGS = -Wall -g

all: 
	@gcc ${CFLAGS} -o protocol application.c link.c main.c statemachine.c 

clean:	
	@rm -f protocol
	@rm -f return_file.gif
	