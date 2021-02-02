/*
    wikireader, an offline wikipedia reader
    Copyright (C) 2020  Nathaniel Choe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <stdio.h>

#define HIGHLIGHTED_TEXT 1

unsigned long long searchString(FILE *file, char searchText[]) {
	int stringLength = strlen(searchText);
	char buffer[stringLength];
	for (int i = 0; i < stringLength; i++) buffer[i] = 0;
	while (strcmp(searchText, buffer)) {
		for (int i = 0; i < stringLength-1; i++)
			buffer[i] = buffer[i+1];
		char c = fgetc(file);
		if (c == EOF)
			return -1;
		buffer[stringLength - 1] = c;
	}
	return ftell(file);
}

void createLookup(FILE *data, FILE *lookup) {
	unsigned long long pageCount = 0;
	for (;;) {
		unsigned long long currentPosition = searchString(data, "<page>");
		if (currentPosition == -1)
			break;
		pageCount++;

		char positionArray[sizeof(currentPosition)];
		unsigned long long positionClone = currentPosition;
		for (int i = sizeof(currentPosition) - 1; i >= 0; i--) {
			positionArray[i] = positionClone & 255;
			positionClone = positionClone >> 8;
		}
		//Load positionClone into a string

		for (int i = 0; i < sizeof(currentPosition); i++)
			fputc(positionArray[i], lookup);
		fputc('\0', lookup);
		searchString(data, "<title>");
		char c = fgetc(data);
		while (c != '<') {
			fputc(c, lookup);
			c = fgetc(data);
		}
		fputc('\0', lookup);

		if (pageCount % 1000 == 0)
			printf("%lu pages have been loaded into the lookup table with the last space being %lu\n", pageCount, currentPosition);
	}
	printf("Loaded all %lu pages into the lookup table.\n", pageCount);
	fclose(lookup);
}

void displayScreen(unsigned long long lookupPosition, unsigned long long lookupIndex, unsigned long long scrollingIndex, FILE *lookup) {
	fseek(lookup, lookupPosition, SEEK_SET);
	//            [8 bytes of lookup position]\0[the name of the article]\0
	//            ^- lookupPosition             ^- what we want

	for (int i = 0; i < LINES; i++) {
		for (int j = 0; j < COLS; j++) {
			char c = fgetc(lookup);
			if (c == 0) {
				fseek(lookup, sizeof(unsigned long long) + 1, SEEK_CUR);
				scrollingIndex++;
				break;
			}
			if (lookupIndex == scrollingIndex)
				attron(COLOR_PAIR(HIGHLIGHTED_TEXT));
			else
				attroff(COLOR_PAIR(HIGHLIGHTED_TEXT));
			mvaddch(i, j, c);
		}
	}
}

void scrollDown(unsigned long long *lookupIndex, unsigned long long *scrollingIndex, unsigned long long *lookupPosition, FILE *lookup) {
	(*scrollingIndex)++;
	(*lookupPosition)++;
	fseek(lookup, *lookupPosition, SEEK_SET);
	char c = 1;
	while (c != 0) {
		*lookupPosition = *lookupPosition + 1;
		c = fgetc(lookup);
	}
	(*lookupPosition) += 9;
}

int main(int argc, char *argv[]) {
	FILE *data = NULL;
	FILE *lookup = NULL;
	int lookupArgumentIndex;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 | strcmp(argv[i], "-h") == 0) {
			printf(
				"%s%s%s%s%s%s%s",
				"Arguments for wikireader:\n\n",

				"--help       -h        Show this screen\n",
				"             -d [FILE] Select the database download\n",
				"             -l [FILE] Select the lookup table for the database download.\n",
				"                       If the lookup table file doesn't exist, one is created.\n\n",
				
				"                       Please note that the -d and -l flags must be specified,\n",
				"                       if they aren't, you get a segmentation fault.\n"
			);
			return 0;
		}
		if (strcmp(argv[i], "-d") == 0)
			data = fopen(argv[i+1], "r");
		if (strcmp(argv[i], "-l") == 0)
			lookupArgumentIndex = i;
	}
	//Parse command line arguments

	if (access(argv[lookupArgumentIndex+1], F_OK) != 0) {
		lookup = fopen(argv[lookupArgumentIndex+1], "w");
		createLookup(data, lookup);
	}
	//At this point, arguments have been parsed and a lookup file exists.

	lookup = fopen(argv[lookupArgumentIndex+1], "r");
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	start_color();
	init_pair(HIGHLIGHTED_TEXT, COLOR_BLACK, COLOR_WHITE);
	unsigned long long lookupIndex = 0;
	unsigned long long scrollingIndex = 0;
	unsigned long long lookupPosition = 9;
	for (;;) {
		clear();
		displayScreen(lookupPosition, lookupIndex, scrollingIndex, lookup);
		char shouldEnd = 0;
		switch (getch()) {
			case '`':
				shouldEnd = 1;
				break;
			case 'j':
				lookupIndex++;
				break;
			case 'k':
				lookupIndex -= 1;
				break;
			case 'J':
				scrollDown(&lookupIndex, &scrollingIndex, &lookupPosition, lookup);
				break;
			case 'K':
				break;
			default:
				break;

		}
		if (shouldEnd)
			break;
	}
	endwin();

	return 0;
}
