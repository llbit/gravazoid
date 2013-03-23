CC=gcc
LD=gcc
CFLAGS=-Wall -std=c99 -g

ifdef __MINGW32__
	LDFLAGS=-Lcttf -lcttf -lmingw32 -lSDLmain -lSDL -mwindows \
			-lglu32 -lopengl32 -g
	GAME=game.exe
	LVLEDIT=lvledit.exe
else
	LDFLAGS=-Lcttf -lcttf -lSDL -lGLU -lGL -g
	GAME=game
	LVLEDIT=lvledit
endif

.PHONY:	cttf

all:	cttf ${GAME} ${LVLEDIT}

cttf:
	$(MAKE) -C cttf libcttf.a vex

${GAME}:	main.o render.o bigint.o sfx.o block.o memfile.o
	${LD} -o $@ $^ ${LDFLAGS}

${LVLEDIT}:	lvledit.o render.o sfx.o block.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	$(MAKE) -C cttf clean
	rm -f main.o
	rm -f sfx.o
	rm -f bigint.o
	rm -f render.o
	rm -f ${GAME}
	rm -f ${LVLEDIT}
	rm -f data_level*.h
	rm -f pusselbit.h

main.o:	main.c ark.h cttf/list.h bigint.h cttf/text.h render.h sfx.h \
	cttf/shape.h block.h data_level1.h data_invader.h pusselbit.h
	${CC} ${CFLAGS} -c $< -o $@

block.o: block.c block.h
	${CC} ${CFLAGS} -c $< -o $@

bigint.o: bigint.c bigint.h
	${CC} ${CFLAGS} -c $< -o $@

render.o: render.c render.h block.h
	${CC} ${CFLAGS} -c $< -o $@

memfile.o: memfile.c memfile.h
	${CC} ${CFLAGS} -c $< -o $@

sfx.o:	sfx.c sfx.h music_level.h drums.h
	${CC} ${CFLAGS} -c $< -o $@

lvledit.o:	lvledit.c ark.h cttf/list.h bigint.h cttf/text.h render.h sfx.h cttf/shape.h block.h
	${CC} ${CFLAGS} -c $< -o $@

convertlevel:	convertlevel.c
		${CC} ${CFLAGS} -o $@ $<

toheader:	toheader.c
		${CC} ${CFLAGS} -o $@ $<

data_%.h:	levels/% convertlevel
		./convertlevel $< >$@

pusselbit.h: toheader
		./toheader Pusselbit.ttf pusselbit >pusselbit.h
