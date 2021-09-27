SRC = $(shell find -name "*.c")
OBJ = $(shell echo $(SRC) | sed "s/src/work/g" | sed "s/\.c/.o/g")
LIBS = $(shell ncurses6-config --libs)
CFLAGS := -ggdb
CFLAGS += $(shell ncurses6-config --cflags)

build/wikireader: $(OBJ)
	$(CC) $(OBJ) -o build/wikireader $(LIBS)

work/%.o: src/%.c
	$(CC) $(CFLAGS) $< -c -o $@
