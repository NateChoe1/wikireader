//Everything to do with the search menu

#include <fcntl.h>
#include <ctype.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "xml.h"
#include "search.h"
#include "lookup.h"

#define MAX_SEARCH TITLE_MAX_LENGTH

static void stringToLower(char *string) {
	for (int i = 0; string[i]; i++)
		string[i] = tolower(string[i]);
}

static int64_t searchForArticle(FILE *database, FILE *index, char *search) {
	stringToLower(search);
	fseek(index, 0, SEEK_END);
	long low = 0;
	long high = ftell(index) / sizeof(int64_t);

	for (;;) {
		long mid = (low + high) / 2;
		char title[MAX_SEARCH];
		
		fseek(index, mid * sizeof(int64_t), SEEK_SET);
		int64_t location;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
		fread(&location, sizeof(location), 1, index);
#pragma GCC diagnostic pop
		//get the location specified in the index

		fseek(database, location, SEEK_SET);
		searchForTag(database, "title");
		readTillChar(database, title, MAX_SEARCH, '<', false);
		//get the title

		stringToLower(title);

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
		if (high - 1 <= low)
			return -1;
	}
}

char showPage(FILE *content) {
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

	raw();
	noecho();
	curs_set(0);
	keypad(stdscr, true);
}

void sanitize(FILE *input, FILE *output) {
	sanitizeAmpersands(input, output);
	//This useless function call is to allow for extra things like math to be
	//added without having to do more organization later.
}

char enterSearch(FILE *database, FILE *index) {
	clear();
	echo();
	curs_set(1);
	mvaddstr(LINES / 3, COLS / 2 - 11, "Enter the search query");
	refresh();

	char search[MAX_SEARCH];
	mvgetnstr(LINES / 3 * 2, COLS / 3, search, MAX_SEARCH - 1);

	int64_t location = searchForArticle(database, index, search);
	if (location == -1) {
		noecho();
		curs_set(0);
		return 1;
	}
	for (;;) {
		fseek(database, location, SEEK_SET);
		int64_t redirectLocation;
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
	searchForTag(database, "text");
	sanitize(database, content);

	showPage(content);
	fclose(content);

	noecho();
	curs_set(0);

	return 0;
}
