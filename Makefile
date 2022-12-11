CC ?= gcc

OPTS := -Wall -std=gnu99 -O2
DBG := -ggdb -O0
LIBS := -lSDL -lSDL_ttf -lSDL_image -lSDL_mixer

OBJS := main.o brain.o graphwin.o
PROG := roboarena

all: $(OBJS)
	$(CC) ${OBJS} -o ${PROG} ${LIBS}

%.o: %.c
	$(CC) ${DBG} ${OPTS} -c $<

clean:
	@echo "Clean project"
	@-rm ${PROG} $(OBJS)

strip:
	@echo "Strip program"
	@strip ${PROG}

install: strip
#	@echo "Install program"
#	cp -f ${PROG} ..


