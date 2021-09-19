#define TITLE_MAX_LENGTH 257
//256 + 1 for null char
FILE *createLookup(FILE *database, char *path);

typedef struct {
	char title[TITLE_MAX_LENGTH];
	off_t offset;
} Page;
