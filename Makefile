

CFLAGS =  -std=c99 -g

LDFLAGS = -lpthread -lm



.PHONY: all
all:   a3

a3: a3.c  barrier.o state_array.o
	gcc ${CFLAGS} ${LDFLAGS} barrier.o state_array.o a3.c -o a3

state_array.o: state_array.c state_array.h 
	gcc ${CFLAGS} -c state_array.c
	
barrier.o:barrier.c barrier.h
	gcc ${CFLAGS} -c barrier.c
	
.PHONY: clean
clean:
	rm -rf    a3 *.o *.dSYM

