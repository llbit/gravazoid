CC=gcc
LD=gcc
CFLAGS=-Wall -O2 -std=c99 -g
LDFLAGS=-lSDL -lGLU -lGL -g

all:	game

game:	main.o
	${LD} -o $@ $^ ${LDFLAGS}
