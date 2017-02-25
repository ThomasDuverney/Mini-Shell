CC=gcc
CFLAGS=-Wall -g

all: shell

shell : shell.o readcmd.o
		$(CC) -o shell shell.o readcmd.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

shell.o : readcmd.h

clean: rm shell.o readcmd.o
