CC=gcc
INC=-I./
OS_ARCH := $(shell uname -m)

%.o: %.c
	$(CC) $(INC) -c $<

sserver: main.o
ifeq ($(OS_ARCH),aarch64)
	$(CC) $(INC) -o bin/sserver-arm main.o
else
	$(CC) $(INC) -o bin/sserver main.o
endif

clean:
	rm -f *.o
