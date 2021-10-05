#include <curses.h>
#include <stdlib.h>

#include "curses.h"
#include "showpage.h"

void redrawPage(FILE *file, int currentLine, int curY, int curX) {
	int curXPos, curYPos;
	int fromLine = 0;
	clear();
	move(0, 0);
	for (;;) {
		int c = fgetc(file);
		int x, y;
		getyx(stdscr, y, x);
		if (fromLine == curX && currentLine == curY) {
			curXPos = x;
			curYPos = y;
		}
		if (y >= LINES)
			goto wroteLines;

		switch (c) {
			case '\n':
				move(y + 1, 0);
				attrset(A_NORMAL);
				currentLine++;
				fromLine = 0;
				break;
			case 0:
				for (;;) {
					int attribute = fgetc(file);
					switch (attribute) {
						case 0:
							goto setAttributes;
						case 1:
							attron(A_STANDOUT);
							break;
						case 2:
							attron(A_UNDERLINE);
							break;
						case 3:
							attrset(A_NORMAL);
							break;
					}
				}
				/*
				 * \x00 initiates an escape sequence
				 * \x00 means stop getting attributes
				 * \x01 means bold text
				 * \x02 means underlined text
				 * \x03 means reset (note, codes are reset on each newline)
				 * */
setAttributes:
				break;
			case EOF:
				goto wroteLines;
			default:
				fromLine++;
				addch(c);
		}
		
	}
wroteLines:

	move(curYPos, curXPos);
	refresh();
}

void showPage(FILE *file) {
	register int x = 0;
	register int y = 0;
	int ch;
	register int64_t currentPosition = 0;
	for (;;) {
		fseek(file, currentPosition, SEEK_SET);
		redrawPage(file, 0, y, x);
		int c = wgetch(stdscr);
		switch (c) {
			case KEY_DOWN:
				y++;
				break;
			case KEY_UP:
				y--;
				break;
			case 'e' & 31:
				fseek(file, currentPosition, SEEK_SET);
				ch = fgetc(file);
				while (ch != '\n' && ch != EOF)
					ch = fgetc(file);
				currentPosition = ftell(file);
				break;
			case 'y' & 31:
				fseek(file, currentPosition, SEEK_SET);
				fseek(file, -2, SEEK_CUR);
				ch = fgetc(file);
				while (ch != '\n') {
					fseek(file, -2, SEEK_CUR);
					ch = fgetc(file);
				}
				currentPosition = ftell(file);
				break;
			case 'q': case 'c' & 31:
				return;
		}
	}
}
