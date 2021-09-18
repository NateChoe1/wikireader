#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#include "lookup.h"
#include "search.h"

#define SPECIAL_PAIR 1

#define SEARCH_WIN 0
#define CREATE_WIN 1
#define OPTION_COUNT 2

static void redrawSelection(WINDOW *options[OPTION_COUNT], int selectedOption) {
	clear();

	wclear(options[SEARCH_WIN]);
	wclear(options[CREATE_WIN]);
	wresize(options[SEARCH_WIN], 3, COLS / 2);
	wresize(options[CREATE_WIN], 3, COLS / 2);
	mvwin(options[SEARCH_WIN], LINES / 4 * 3 - 2, COLS / 4);
	mvwin(options[CREATE_WIN], LINES / 4 * 3, COLS / 4);
	mvwaddstr(options[SEARCH_WIN], 1, COLS / 4 - 8, "Search Wikipedia");
	mvwaddstr(options[CREATE_WIN], 1, COLS / 4 - 10, "Create an index file");

	mvaddstr(LINES / 8, COLS / 2 - 4, "Actions:");
	wborder(options[0], 0, 0, 0, 0, 0, 0, ACS_LTEE, ACS_RTEE);
	for (int i = 1; i < OPTION_COUNT; i++)
		wborder(options[i], 0, 0, 0, 0, ACS_LTEE, ACS_RTEE, 0, 0);
	if (has_colors()) {
		wattron(options[selectedOption], COLOR_PAIR(SPECIAL_PAIR));
		if (selectedOption == 0)
			wborder(options[selectedOption],
					0, 0, 0, 0, 0, 0, ACS_LTEE, ACS_RTEE);
		else
			wborder(options[selectedOption],
					0, 0, 0, 0, ACS_LTEE, ACS_RTEE, 0, 0);
		wattroff(options[selectedOption], COLOR_PAIR(SPECIAL_PAIR));
	}
	else {
		wborder(options[selectedOption],
				'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');
	}
	refresh();
	for (int i = 0; i < OPTION_COUNT; i++)
		wrefresh(options[i]);
	redrawwin(options[selectedOption]);
	wrefresh(options[selectedOption]);
}

long countArticles(FILE *index) {
	fseek(index, 0, SEEK_END);
	return ftell(index) / 8;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		puts("Usage: wikireader [database file] [index file]");
		exit(EXIT_SUCCESS);
	}

	FILE *database = fopen(argv[1], "r");
	FILE *index = fopen(argv[2], "r");
	long articleCount = 0;
	if (index)
		articleCount = countArticles(index);;

	initscr();
	start_color();
	cbreak();
	noecho();
	curs_set(0);
	keypad(stdscr, true);

	if (has_colors())
		init_pair(SPECIAL_PAIR, COLOR_GREEN, COLOR_BLACK);

	WINDOW *options[OPTION_COUNT] = {
		[SEARCH_WIN] = newwin(3, COLS / 2, LINES / 4 * 3 - 2, COLS / 4),
		[CREATE_WIN] = newwin(3, COLS / 2, LINES / 4 * 3, COLS / 4),
	};

	int selectedOption = 0;
	redrawSelection(options, selectedOption);
	for (;;) {
		int c = wgetch(stdscr);
		switch (c) {
			case KEY_DOWN:
				selectedOption++;
				selectedOption %= OPTION_COUNT;
				break;
			case KEY_UP:
				selectedOption--;
				if (selectedOption < 0)
					selectedOption += OPTION_COUNT;
				break;
			case KEY_ENTER: case '\n':
				switch (selectedOption) {
					case CREATE_WIN:
						index = createLookup(database, argv[2]);
						if (index == NULL) {
							clear();
							mvaddstr(LINES / 2, COLS / 2 - 15,
									"ERROR: Couldn't make index file");
							refresh();
							break;
						}
						articleCount = countArticles(index);
						break;
					case SEARCH_WIN:
						if (enterSearch(database, index))
							continue;
						break;
				}
				break;
		}
		redrawSelection(options, selectedOption);
	}

	endwin();
}
