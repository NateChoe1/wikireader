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
#include <stdio.h>

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

int main(int argc, char *argv[]) {
	FILE *data = NULL;
	FILE *lookup = NULL;
	int lookupIndex;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0)
			data = fopen(argv[i+1], "r");
		if (strcmp(argv[i], "-l") == 0) {
			lookupIndex = i;
			if (access(argv[i+1], F_OK) == 0)
				goto lookupMade;
			lookup = fopen(argv[i+1], "w");
		}
	}

	unsigned long long pageCount = 0;
	while (1) {
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
		searchString(data, "<title>");
		fputc('\0', lookup);
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

	lookupMade:

	return 0;
}
