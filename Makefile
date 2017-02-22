CC=gcc
CFLAGS=-Wall -g

all: tst shell

tst: tst.o readcmd.o
		$(CC) -o tst tst.o readcmd.o

shell : shell.o readcmd.o
		$(CC) -o shell shell.o readcmd.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

shell.o : readcmd.h
tst.o :   readcmd.h

clean:
	rm -f shell.o readcmd.o tst tst.o
