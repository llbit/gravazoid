CC=gcc
LD=gcc
CFLAGS=-Wall -O2 -std=c99
LDFLAGS=-lSDL -lGLU -lGL

all:	game

game:	main.o
	${LD} -o $@ $^ ${LDFLAGS}
