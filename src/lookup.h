#define TITLE_MAX_LENGTH 257
//256 + 1 for null char
FILE *createLookup(FILE *database, char *path);

typedef struct {
	char title[TITLE_MAX_LENGTH];
	int64_t offset;
} Page;
