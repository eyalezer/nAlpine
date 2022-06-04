CC=gcc
INC=-I./

%.o: %.c
	$(CC) $(INC) -c $<

sserver: main.o
	$(CC) $(INC) -o bin/sserver main.o

clean:
	rm -f *.o