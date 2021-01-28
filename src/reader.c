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
	memset(buffer, 0, stringLength * sizeof(char));
	char foundString = 1;
	while (strcmp(searchText, buffer)) {
		for (int i = 0; i < stringLength-1; i++)
			buffer[i] = buffer[i+1];
		char c = fgetc(file);
		buffer[stringLength - 1] = c;
		if (c == EOF) {
			foundString = 0;
			break;
		}
	}
	if (foundString == 0)
		return -1;
	fflush(file);
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
		fseek(data, currentPosition, SEEK_SET);
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

		if (pageCount % 1000 == 0)
			printf("%d pages have been loaded into the lookup table with the last space being %lu\n", pageCount, currentPosition);
	}
	printf("Loaded all %d pages into the lookup table.\n", pageCount);
	fclose(lookup);

	lookupMade:
	lookup = fopen(argv[lookupIndex+1], "r");
	fseek(lookup, 0L, SEEK_END);
	unsigned long long articleCount = ftell(lookup) / sizeof(unsigned long long);
	fseek(lookup, 0, SEEK_SET);
	for (int i = 0; i < articleCount; i++) {
		char n[sizeof(unsigned long long)];
		for (int j = 0; j < sizeof(unsigned long long); j++)
			n[j] = fgetc(lookup);
		unsigned long long pos = * (unsigned long long *) &n;//This is stolen from the fast inverse square root function in Quake.
		fseek(data, pos, SEEK_SET);
		pos = searchString(data, "<title>");
		char c = fgetc(data);
		while (c != '<') {
			printf("%c", c);
			c = fgetc(data);
		}
		printf("\n");
	}

	return 0;
}
