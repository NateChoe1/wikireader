#include <ctype.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>

#include "xml.h"
#include "lookup.h"

#define STATUS_MAX_LENGTH 45

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

		if (searchForTag(database, "title") < 0)
			break;
		readTillChar(database, pages[pagesSeen].title, TITLE_MAX_LENGTH,
				'<', false);
		for (int i = 0; pages[pagesSeen].title[i]; i++)
			pages[pagesSeen].title[i] = tolower(pages[pagesSeen].title[i]);

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
