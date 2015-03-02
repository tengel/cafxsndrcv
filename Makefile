CC = gcc
CFLAGS = -Wall
LDFLAGS = 
SHELL = /bin/sh
RM = rm -f


all: cafxsnd cafxrcv

comm.o: comm.h comm.c
	$(CC) -c comm.c -o comm.o $(CFLAGS)

cafxsnd: cafxsnd.c comm.o
	$(CC) cafxsnd.c comm.o -o cafxsnd $(CFLAGS)

cafxrcv: cafxrcv.c comm.o
	$(CC) cafxrcv.c comm.o -o cafxrcv $(CFLAGS)

install: all
	cp ./cafxsnd /usr/local/bin
	cp ./cafxrcv /usr/local/bin

uninstall:
	$(RM) /usr/local/bin/cafxsnd
	$(RM) /usr/local/bin/cafxrcv
clean:
	$(RM) cafxsnd
	$(RM) cafxrcv
	$(RM) comm.o

distclean: clean
	$(RM) *.cafx

dist: clean
	cd .. && tar -cvzf cafxsndrcv-0.8.tar.gz ./cafxsndrcv-0.8




