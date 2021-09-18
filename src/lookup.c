#include <ctype.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>

#include "lookup.h"

#define TAG_MAX_LENGTH 10
#define STATUS_MAX_LENGTH 45

int readTillChar(FILE *database, char *buffer, int max,
		char stop, bool stopSpace) {
	for (int i = 0; i < max; i++) {
		int c = fgetc(database);
		if (c == stop || (stopSpace && isspace(c))) {
			while (c != stop)
				c = fgetc(database);
			//guarantee that we're ending at stop.
			buffer[i] = '\0';
			return i;
		}
		if (c == EOF)
			return -1;
		buffer[i] = c;
	}
	buffer[max - 1] = '\0';
	return max - 1;
}

char *nextTag(FILE *database, off_t *location) {
	for (;;) {
		int c = fgetc(database);
		if (c == '<')
			break;
		if (c == EOF)
			return NULL;
	}

	if (location != NULL) {
		*location = ftell(database) - 1;
	}
	char *buffer = malloc(TAG_MAX_LENGTH);
	int length = readTillChar(database, buffer, TAG_MAX_LENGTH, '>', true);
	
	return buffer;
}

off_t searchForTag(FILE *database, char *tag) {
	for (;;) {
		off_t possibility;
		char *buffer = nextTag(database, &possibility);
		if (buffer == NULL)
			return -1;
		if (strcmp(buffer, tag) == 0) {
			free(buffer);
			return possibility;
		}
		free(buffer);
	}
}
//finds the next instance of <tag> in the file, and returns the position of
//that '<'. Returns 0 if EOF is hit. Seeks the file to after the '>'.

static int comparePage(Page *page1, Page *page2) {
	return strcmp(page1->title, page2->title);
}

FILE *createLookup(FILE *database, char *path) {
	fseek(database, 0, SEEK_SET);

	long allocatedPages = 2000000;
	long pagesSeen;
	Page *pages = malloc(sizeof(Page) * allocatedPages);

	clear();

	for (pagesSeen = 0;; pagesSeen++) {
		off_t pos = searchForTag(database, "page");
		if (pos == -1)
			break;
		if (pagesSeen >= allocatedPages) {
			allocatedPages *= 2;
			pages = realloc(pages, sizeof(pages[0]) * allocatedPages);
		}
		pages[pagesSeen].offset = pos;

		searchForTag(database, "title");
		readTillChar(database, pages[pagesSeen].title, TITLE_MAX_LENGTH,
				'<', false);

		if (pagesSeen % 100000 == 0) {
			char buffer[STATUS_MAX_LENGTH];
			int len = snprintf(buffer, STATUS_MAX_LENGTH,
					"%ld pages have been read (%ld)", pagesSeen, pos);
			clear();
			mvaddnstr(LINES / 2, COLS / 2 - len / 2, buffer, len);
			refresh();
		}
	}

	clear();
	mvaddstr(LINES / 2, COLS / 2 - 18, "All pages have been gotten, sorting.");
	refresh();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	qsort(pages, pagesSeen, sizeof(pages[0]), comparePage);
#pragma GCC diagnostic pop

	FILE *lookup = fopen(path, "w+");
	if (lookup == NULL)
		return NULL;

	for (long i = 0; i < pagesSeen; i++)
		fwrite(&pages[i].offset, sizeof(off_t), 1, lookup);
	return lookup;
}
