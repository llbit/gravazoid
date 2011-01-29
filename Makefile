CC=gcc
LD=gcc
CFLAGS=-Wall -O3 -std=c99 -g -pg
LDFLAGS=-lSDL -lGLU -lGL -g -pg

all:	game vex

game:	main.o render.o shape.o cttf.o text.o
	${LD} -o $@ $^ ${LDFLAGS}

vex:	vex.o shape.o render.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	rm main.o
	rm shape.o
	rm cttf.o
	rm text.o
	rm vex
	rm game

main.o:	main.c ark.h
	${CC} ${CFLAGS} -c $< -o $@

vex.o: vex.c
	${CC} ${CFLAGS} -c $< -o $@

render.o: render.c render.h
	${CC} ${CFLAGS} -c $< -o $@

text.o: text.c text.h
	${CC} ${CFLAGS} -c $< -o $@

cttf.o: cttf/cttf.c
	${CC} ${CFLAGS} -c $< -o $@

shape.o: cttf/shape.c
	${CC} ${CFLAGS} -c $< -o $@
