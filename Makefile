CC=gcc
COMMON=-O3
CFLAGS=${COMMON}

wikireader: src/reader.c
	${CC} src/reader.c ${CFLAGS} -o build/reader
