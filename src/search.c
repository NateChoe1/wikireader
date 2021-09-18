#include <curses.h>
#include <string.h>
#include <stdlib.h>

#ifdef __unix__
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#else
#error "Unsupported OS (Only unix is supported for now)"
#endif

#include "search.h"
#include "lookup.h"

#define MAX_SEARCH 257
#define MAX_SPECIAL 10

typedef struct {
	char *verbose;
	char value;
} Escape;

Escape escapes[] = {
	{"amp", '&'},
	{"semi", ';'},
	{"quot", '"'},
	{"lt", '<'},
	{"gt", '>'},
};

static off_t searchForArticle(FILE *database, FILE *index, char *search) {
	fseek(index, 0, SEEK_END);
	long low = 0;
	long high = ftell(index) / sizeof(off_t);

	for (;;) {
		long mid = (low + high) / 2;
		char title[MAX_SEARCH];
		
		fseek(index, mid * sizeof(off_t), SEEK_SET);
		off_t location;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
		fread(&location, sizeof(location), 1, index);
#pragma GCC diagnostic pop
		//get the location specified in the index

		fseek(database, location, SEEK_SET);
		searchForTag(database, "title");
		readTillChar(database, title, MAX_SEARCH, '<', false);
		//get the title

		int comp = strcmp(search, title);
		if (comp < 0)
			high = mid;
		else if (comp == 0)
			return location;
		else
			low = mid;
		//binary search
		if (high <= low)
			return -1;
		if (high - 1 == low)
			return low;
	}
}

char showPage(FILE *content) {
	//TODO: Make this function OS independent.
#ifdef __unix__
	int pid = fork();
	if (pid == 0) {
		int fd = fileno(content);
		char path[20];
		sprintf(path, "/dev/fd/%d", fd);
		execlp("less", "less", path, NULL);
	}
	if (pid > 0)
		wait(&pid);
	else
		return 1;
#endif

	cbreak();
	noecho();
	curs_set(0);
	keypad(stdscr, true);
}

char enterSearch(FILE *database, FILE *index) {
	clear();
	echo();
	curs_set(1);
	mvaddstr(LINES / 3, COLS / 2 - 11, "Enter the search query");
	refresh();

	char search[MAX_SEARCH];
	mvgetnstr(LINES / 3 * 2, COLS / 3, search, MAX_SEARCH - 1);

	off_t location = searchForArticle(database, index, search);
	if (location == -1)
		return 1;
	for (;;) {
		fseek(database, location, SEEK_SET);
		off_t redirectLocation;
		for (;;) {
			char *tag = nextTag(database, &redirectLocation);
			if (strcmp(tag, "/page") == 0) {
				free(tag);
				goto noRedirects;
			}
			if (strcmp(tag, "redirect") == 0) {
				free(tag);
				break;
			}
			free(tag);
		}
		//detect redirects

		fseek(database, redirectLocation, SEEK_SET);
		int c = fgetc(database);
		while (c != '"' && c != EOF)
			c = fgetc(database);
		if (c != EOF)
			readTillChar(database, search, MAX_SEARCH, '"', false);
		//if we should redirect, get the new redirect location
		else
			return 1;

		location = searchForArticle(database, index, search);
	}
noRedirects:

	FILE *content = tmpfile();

	fseek(database, location, SEEK_SET);
	off_t articleLocation = searchForTag(database, "text");
	for (;;) {
		int c = fgetc(database);
		switch (c) {
			case '<': case EOF:
				goto wroteArticle;
			case '&':
				char special[MAX_SPECIAL];
				readTillChar(database, special, MAX_SPECIAL, ';', false);
				for (int i = 0; i < sizeof(escapes) / sizeof(escapes[0]); i++)
					if (strcmp(special, escapes[i].verbose) == 0) {
						fputc(escapes[i].value, content);
						break;
					}
				break;
			default:
				fputc(c, content);
				break;
		}
	}
wroteArticle:

	showPage(content);
	fclose(content);

	noecho();
	curs_set(0);

	return 0;
}
