CC ?= gcc
CFLAGS ?= -Wall -Wextra -g -pipe
LDFLAGS ?=

INC = -I/usr/src/sys

EXE = wsfont
SRC = wsfont.c
HDR = wsfont.h

${EXE}: ${SRC} ${HDR}
	${CC} ${CFLAGS} ${INC} -o ${EXE} ${SRC} ${LDFLAGS}

${HDR}:
	awk -f mkhdr.awk /usr/src/sys/dev/wsfont/*.h > ${HDR}

clean:
	rm -f ${EXE} ${HDR}
