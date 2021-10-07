//Everything to do with the search menu

#pragma GCC diagnostic ignored "-Wunused-result"

#include <fcntl.h>
#include <ctype.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>

#include "xml.h"
#include "curses.h"
#include "search.h"
#include "lookup.h"
#include "showpage.h"

void getTitle(FILE *database, char *ret) {
	searchForTag(database, "title");
	readTillChar(database, ret, TITLE_MAX_LENGTH, '<', false);
}

static int istrcmp(char *string1, char *string2) {
//case insensitive string comparison
	for (register int i = 0;; i++) {
		register char c1 = tolower(string1[i]);
		register char c2 = tolower(string2[i]);
		if (c1 != c2)
			return c1 - c2;
		if (c1 == '\0')
			return 0;
	}
}

static int64_t findFirst(FILE *database, FILE *index, char *search) {
	fseek(index, 0, SEEK_END);
	register long low = 0;
	register long high = ftell(index) / sizeof(int64_t);

	while (low + 1 < high) {
		register long mid = (low + high) / 2;
		fseek(index, mid * sizeof(int64_t), SEEK_SET);

		int64_t location;
		fread(&location, sizeof(location), 1, index);
		fseek(database, location, SEEK_SET);

		char title[TITLE_MAX_LENGTH];
		getTitle(database, title);
		int cmp = istrcmp(search, title);
		if (cmp >= 0)
			low = mid;
		else
			high = mid;
	}
	return high;
}

int64_t searchForArticle(FILE *database, FILE *index, char *search,
		uint8_t args, int64_t *indexLocation) {
	int64_t firstArticle = findFirst(database, index, search);
	if (!args & RETURN_FIRST) {
		fseek(index, firstArticle * sizeof(int64_t), SEEK_SET);
		for (;;) {
			fread(&firstArticle, sizeof(int64_t), 1, index);
			fseek(database, firstArticle, SEEK_SET);
			char title[TITLE_MAX_LENGTH];
			getTitle(database, title);
			if (istrcmp(title, search))
				return -1;
			if (strcmp(title, search) == 0)
				break;
		}
	}

	int64_t ret;
	fseek(index, firstArticle * sizeof(int64_t), SEEK_SET);
	fread(&ret, sizeof(int64_t), 1, database);
	if (indexLocation != NULL)
		*indexLocation = firstArticle * sizeof(int64_t);
	return ret;
}

int64_t followRedirects(FILE *database, FILE *index) {
	int64_t currentLocation = ftell(database);
	for (;;) {
		int64_t redirectTag;
		for (;;) {
			char *tag = nextTag(database, &redirectTag);
			if (strcmp(tag, "/page") == 0) {
				free(tag);
				fseek(database, currentLocation, SEEK_SET);
				return currentLocation;
			}
			if (strcmp(tag, "redirect") == 0) {
				free(tag);
				break;
			}
			if (tag == NULL) {
				free(tag);
				return -1;
			}
			free(tag);
		}

		fseek(database, redirectTag, SEEK_SET);

		for (;;) {
			int c = fgetc(database);
			if (c == '"')
				break;
			if (c == EOF)
				return -1;
		}

		char newTitle[TITLE_MAX_LENGTH];
		readTillChar(database, newTitle, TITLE_MAX_LENGTH, '"', false);

		currentLocation = searchForArticle(database, index, newTitle,
				0, NULL);
		fseek(database, currentLocation, SEEK_SET);
	}
}

char enterSearch(FILE *database, FILE *index) {
	clear();
	curs_set(1);

	char search[TITLE_MAX_LENGTH];
	int searchLen = 0;
	int selectedArticle = -1;
	//-1 means that you're entering the search query
	int64_t location;
	int64_t indexLocation = -1;
	int scrollPosition = 0;
	noraw();
	cbreak();
	//setting cbreak because KEY_BACKSPACE doesn't work in raw mode.

	for (;;) {
		if (indexLocation == -1) {
			searchForArticle(database, index, search,
					RETURN_FIRST, &indexLocation);
		}
		//ideally, we don't search on every single key press, so every time the
		//search query changes, we just store that as indexLocation being -1.

		clear();
		attrset(A_NORMAL);
		mvaddstr(0, 0, "Search: ");
		addnstr(search, searchLen);
		refresh();

		int drawingArticle = 0;
		if (searchLen > 0) {
			fseek(index, indexLocation + scrollPosition, SEEK_SET);
			int currentY = 1;
			for (drawingArticle = 0;; drawingArticle++) {
				int64_t thisLocation;
				fread(&thisLocation, sizeof(thisLocation), 1, index);
				fseek(database, thisLocation, SEEK_SET);
				char title[TITLE_MAX_LENGTH];
				getTitle(database, title);

				int len = strlen(title);
				int newPosition = currentY + (len - 1) / COLS + 1;
				if (newPosition > LINES)
					break;
				move(currentY, 0);
				currentY = newPosition;

				if (drawingArticle == selectedArticle) {
					if (has_colors())
						attrset(COLOR_PAIR(SPECIAL_PAIR));
					attron(A_STANDOUT);
					location = thisLocation;
				}
				else
					attrset(A_NORMAL);

				addstr(title);
			}
		}

		refresh();
		int c = wgetch(stdscr);
		switch (c) {
			case KEY_LEFT: case KEY_BACKSPACE:
				 if (--searchLen < 0)
					 searchLen = 0;
				 indexLocation = -1;
				 break;
			 case KEY_DOWN:
				 if (++selectedArticle >= drawingArticle)
					 selectedArticle--;
				 break;
			 case KEY_UP:
				 if (--selectedArticle < 0)
					 selectedArticle = 0;
				 break;
			 case 'e' & 31:
				 scrollPosition += sizeof(int64_t);
				 break;
			 case 'y' & 31:
				 scrollPosition -= sizeof(int64_t);
				 break;
			 case KEY_RIGHT: case KEY_ENTER: case '\n':
				 goto gotArticle;
			 default:
				 search[searchLen++] = (char) c;
				 indexLocation = -1;
				 scrollPosition = 0;
				 selectedArticle = -1;
				 break;
		}
		search[searchLen] = '\0';
	}
gotArticle:
	nocbreak();
	raw();

	FILE *content = tmpfile();

	fseek(index, indexLocation + scrollPosition, SEEK_SET);
	fread(&location, sizeof(location), 1, index);
	fseek(database, location, SEEK_SET);
	location = followRedirects(database, index);
	fseek(database, location, SEEK_SET);
	searchForTag(database, "text");
	for (;;) {
		int c = fgetc(database);
		if (c == '<')
			break;
		fputc(c, content);
	}

	showPage(content);
	fclose(content);

	noecho();
	curs_set(0);

	return 0;
}
