#define INITIAL_TAG_SIZE 10

char *nextTag(FILE *file, int64_t *locationReturn);
int64_t searchForTag(FILE *file, char *tag);
void readTillChar(FILE *file, char *buffer, int max, char end, bool stopWhite);
void sanitizeAmpersands(FILE *input, FILE *output);
