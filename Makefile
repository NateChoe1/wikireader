SRC = $(shell find -name "*.c")
OBJ = $(shell echo $(SRC) | sed "s/src/work/g" | sed "s/\.c/.o/g")
LIBS = $(shell ncurses6-config --libs)
CFLAGS := -O2 -Wall -Wpedantic
CFLAGS += $(shell ncurses6-config --cflags)
INSTALLDIR := /usr/bin

build/wikireader: $(OBJ)
	$(CC) $(OBJ) -o build/wikireader $(LIBS)

work/%.o: src/%.c
	$(CC) $(CFLAGS) $< -c -o $@

install: build/wikireader
	cp build/wikireader $(INSTALLDIR)

uninstall: $(INSTALLDIR)/wikireader
	rm $(INSTALLDIR)/wikireader
