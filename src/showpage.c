#include <curses.h>
#include <stdlib.h>

#include "curses.h"
#include "showpage.h"

typedef struct {
	char **content;
	int allocatedLines;
	int lines;
} Article;

void redrawPage(Article article, int start, int curY, int curX) {
	int curXPos, curYPos;
	clear();
	move(0, 0);
	for (int line = start; line < article.lines; line++) {
		int x, y;

		for (int i = 0; article.content[line][i]; i++) {
			getyx(stdscr, y, x);
			if (y >= LINES)
				goto wroteLines;
			if (i == curX && line == curY) {
				curXPos = x;
				curYPos = y;
			}

			addch(article.content[line][i]);
		}
		move(y + 1, 0);
		attrset(A_NORMAL);
	}
wroteLines:

	move(curYPos, curXPos);
	refresh();
}

void showPage(FILE *file) {
	register int x = 0;
	register int y = 0;

	fseek(file, 0, SEEK_SET);
	Article article;
	article.allocatedLines = 20;
	article.lines = 0;
	article.content = malloc(article.allocatedLines * sizeof(char *));
	for (;;) {
		int allocatedLength = 20;
		int lineLength = 0;
		char *currentLine = malloc(allocatedLength);
		for (;;) {
			int c = fgetc(file);
			if (lineLength == allocatedLength) {
				allocatedLength *= 2;
				currentLine = realloc(currentLine, allocatedLength);
			}
			switch (c) {
				case EOF:
					currentLine[lineLength] = '\0';
					article.content[article.lines++] = currentLine;
					goto gotArticle;
				case '\n':
					currentLine[lineLength] = '\0';
					goto gotLine;
				default:
					currentLine[lineLength++] = c;
					break;
			}
		}
gotLine:
		if (article.lines >= article.allocatedLines) {
			article.allocatedLines *= 2;
			article.content = realloc(article.content,
						article.allocatedLines * sizeof(char *));
		}
		article.content[article.lines++] = currentLine;
	}

gotArticle:;

	int scrollPosition = 0;

	for (;;) {
		redrawPage(article, scrollPosition, y, x);
		int c = wgetch(stdscr);
		switch (c) {
			case KEY_DOWN: case 'j':
				y++;
				break;
			case KEY_UP: case 'k':
				y--;
				break;
			case 'e' & 31:
				scrollPosition++;
				break;
			case 'y' & 31:
				if (--scrollPosition < 0)
					scrollPosition = 0;
				break;
			case 'q': case 'c' & 31:
				return;
		}
	}
}
