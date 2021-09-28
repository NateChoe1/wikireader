//Everything to do with the search menu

#include <fcntl.h>
#include <ctype.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "xml.h"
#include "curses.h"
#include "search.h"
#include "lookup.h"

static void stringToLower(char *string) {
	for (int i = 0; string[i]; i++)
		string[i] = tolower(string[i]);
}

void getTitle(FILE *database, char *ret) {
	searchForTag(database, "title");
	readTillChar(database, ret, TITLE_MAX_LENGTH, '<', false);
}

int64_t searchForArticle(FILE *database, FILE *index, char *search,
		uint8_t args, int64_t *indexLocation) {
	stringToLower(search);
	fseek(index, 0, SEEK_END);
	long low = 0;
	long high = ftell(index) / sizeof(int64_t);

	for (;;) {
		long mid = (low + high) / 2;
		char title[TITLE_MAX_LENGTH];
		
		int64_t seekLocation = mid * sizeof(int64_t);
		if (indexLocation != NULL)
			*indexLocation = seekLocation;
		fseek(index, seekLocation, SEEK_SET);
		int64_t location;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
		fread(&location, sizeof(location), 1, index);
#pragma GCC diagnostic pop
		//get the location specified in the index

		fseek(database, location, SEEK_SET);
		getTitle(database, title);
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
		if (high - 1 <= low) {
			if (args & RETURN_FIRST)
				if (comp < 0)
					return high;
				else
					return low;
			else
				return -1;
		}
	}
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

static char showPage(FILE *content) {
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

static void sanitize(FILE *input, FILE *output) {
	sanitizeAmpersands(input, output);
	//This useless function call is to allow for extra things like math to be
	//added without having to do more organization later.
}

char enterSearch(FILE *database, FILE *index) {
	clear();
	curs_set(1);

	char search[TITLE_MAX_LENGTH];
	//mvgetnstr(LINES / 3 * 2, COLS / 3, search, TITLE_MAX_LENGTH);
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
		if (indexLocation == -1)
			searchForArticle(database, index, search,
					RETURN_FIRST, &indexLocation);
		//ideally, we don't search on every single key press, so every time the
		//search query changes, we just store that as indexLocation being -1.

		clear();
		attrset(A_NORMAL);
		mvaddstr(0, 0, "Search: ");
		addnstr(search, searchLen);
		refresh();

		int drawingArticle;
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

	fseek(database, location, SEEK_SET);
	location = followRedirects(database, index);
	fseek(database, location, SEEK_SET);
	searchForTag(database, "text");
	sanitize(database, content);

	showPage(content);
	fclose(content);

	noecho();
	curs_set(0);

	return 0;
}
