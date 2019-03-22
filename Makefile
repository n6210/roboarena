OPTS := -Wall -std=gnu99
DBG := -ggdb -O0
LIBS := -lSDL -lSDL_ttf -lSDL_image -lSDL_mixer

FILES := main.c brain.c graphwin.c
PROG := roboarena

all: roboarena.h
	gcc ${DBG} ${OPTS} ${FILES} -o ${PROG} ${LIBS}

clean:
	@rm ${PROG} *.o

strip:
	@strip ${PROG}

install:
	cp -f ${PROG} ..


