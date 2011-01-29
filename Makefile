CC=gcc
LD=gcc
CFLAGS=-Wall -g -std=c99
LDFLAGS=-lSDL -lGLU -lGL

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
