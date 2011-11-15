CC = gcc
CFLAGS = -pedantic -Wall -g
LD = gcc
LDFLAGS = 

all : 
	make mush

mush : mush.o parseline.o header.h
	$(LD) $(LDFLAGS) -o mush mush.o parseline.o

mush.o : mush.c
	$(CC) $(CFLAGS) -c -o mush.o mush.c

parseline.o : parseline.c
	$(CC) $(CFLAGS) -c -o parseline.o parseline.c

clean :
	rm *.o