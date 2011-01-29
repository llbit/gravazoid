CC=gcc
LD=gcc
CFLAGS=-Wall -O2 -std=c99
LDFLAGS=-lSDL -lGLU -lGL

all:	game

game:	main.o shape.o cttf.o
	${LD} -o $@ $^ ${LDFLAGS}

cttf.o: cttf/cttf.c
	${CC} ${CFLAGS} -c cttf/cttf.c -o cttf.o

shape.o: cttf/shape.c
	${CC} ${CFLAGS} -c cttf/shape.c -o shape.o
