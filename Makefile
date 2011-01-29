CC=gcc
LD=gcc
CFLAGS=-Wall -O2 -std=c99 -g -pg
LDFLAGS=-lSDL -lGLU -lGL -g -pg

all:	game

game:	main.o render.o shape.o cttf.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	rm main.o
	rm shape.o
	rm cttf.o
	rm game

render.o: render.c
	${CC} ${CFLAGS} -c $^ -o $@

cttf.o: cttf/cttf.c
	${CC} ${CFLAGS} -c $^ -o $@

shape.o: cttf/shape.c
	${CC} ${CFLAGS} -c $^ -o $@
