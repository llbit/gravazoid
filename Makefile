CC=gcc
LD=gcc
CFLAGS=-Wall -std=c99 -g

ifdef __MINGW32__
	LDFLAGS=-Lcttf -lcttf -lmingw32 -lSDLmain -lSDL -mwindows \
			-lglu32 -lopengl32 -g
else
	LDFLAGS=-Lcttf -lcttf -lSDL -lGLU -lGL -g
endif

.PHONY:	cttf

all:	cttf game lvledit

cttf:
	$(MAKE) -C cttf libcttf.a vex

game:	main.o render.o bigint.o sfx.o block.o
	${LD} -o $@ $^ ${LDFLAGS}

lvledit:	lvledit.o render.o sfx.o block.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	$(MAKE) -C cttf clean
	rm main.o
	rm sfx.o
	rm bigint.o
	rm render.o
	rm game
	rm lvledit

main.o:	main.c ark.h cttf/list.h bigint.h cttf/text.h render.h sfx.h cttf/shape.h block.h
	${CC} ${CFLAGS} -c $< -o $@

block.o: block.c block.h
	${CC} ${CFLAGS} -c $< -o $@

bigint.o: bigint.c bigint.h
	${CC} ${CFLAGS} -c $< -o $@

render.o: render.c render.h block.h
	${CC} ${CFLAGS} -c $< -o $@

sfx.o:	sfx.c sfx.h music_level.h drums.h
	${CC} ${CFLAGS} -c $< -o $@

lvledit.o:	lvledit.c ark.h cttf/list.h bigint.h cttf/text.h render.h sfx.h cttf/shape.h block.h
	${CC} ${CFLAGS} -c $< -o $@

