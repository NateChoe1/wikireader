CC=gcc
COMMON=-O3 -lncurses
CFLAGS=${COMMON}

wikireader: src/reader.c
	${CC} src/reader.c ${CFLAGS} -o build/reader
