int64_t searchForArticle(FILE *database, FILE *index, char *search,
		bool returnFirst, int64_t *indexLocation);
int64_t followRedirects(FILE *database, FILE *index);
char enterSearch(FILE *database, FILE *index);
