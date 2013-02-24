CC=gcc
LD=gcc
CFLAGS=-Wall -std=c99 -g -pg

ifdef __MINGW32__
	LDFLAGS=-Lcttf -lcttf -lmingw32 -lSDLmain -lSDL -mwindows -lGLU -lGL -g
else
	LDFLAGS=-Lcttf -lcttf -lSDL -lGLU -lGL -g
endif

.PHONY:	cttf

all:	cttf game

cttf:
	$(MAKE) -C cttf libcttf.a vex

game:	main.o render.o bigint.o sfx.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	$(MAKE) -C cttf clean
	rm main.o
	rm sfx.o
	rm bigint.o
	rm render.o
	rm game

main.o:	main.c ark.h cttf/list.h bigint.h cttf/text.h render.h sfx.h cttf/shape.h
	${CC} ${CFLAGS} -c $< -o $@

bigint.o: bigint.c bigint.h
	${CC} ${CFLAGS} -c $< -o $@

render.o: render.c render.h
	${CC} ${CFLAGS} -c $< -o $@

sfx.o:	sfx.c sfx.h music_level.h drums.h
	${CC} ${CFLAGS} -c $< -o $@
