int64_t searchForArticle(FILE *database, FILE *index, char *search,
		uint8_t args, int64_t *indexLocation);
int64_t followRedirects(FILE *database, FILE *index);
char enterSearch(FILE *database, FILE *index);

//args to searchForArticle.

#define RETURN_FIRST (1 << 0)
//If there are no exact matches, return the first match alphabetically.
//Otherwise, only return case sensitive exact matches
