#define TITLE_MAX_LENGTH 257
//256 + 1 for null char
FILE *createLookup(FILE *database, char *path);
int readTillChar(FILE *database, char *buffer, int max,
		char stop, bool stopSpace);
char *nextTag(FILE *database, off_t *location);
off_t searchForTag(FILE *database, char *tag);

typedef struct {
	char title[TITLE_MAX_LENGTH];
	off_t offset;
} Page;
