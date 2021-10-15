#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "xml.h"

#define MAX_SPECIAL_LEN 8

typedef struct {
	char title[MAX_SPECIAL_LEN];
	char c;
} Escape;

static Escape escapes[] = {
	{"amp", '&'},
	{"semi", ';'},
	{"quot", '"'},
	{"lt", '<'},
	{"gt", '>'},
	{"nbsp", ' '},
};

void readTillChar(FILE *file, char *buffer, int max, char end, bool stopWhite) {
	for (int i = 0; i < max; i++) {
		int c = fgetc(file);
		if (c == end || (stopWhite && isspace(c))) {
			buffer[i] = '\0';
			while (c != end)
				c = fgetc(file);
			return;
		}
		buffer[i] = c;
	}
	buffer[max - 1] = '\0';
}

char *nextTag(FILE *file, int64_t *locationReturn) {
	for (;;) {
		int c = fgetc(file);
		switch (c) {
			case '<':
				goto getTag;
			case EOF:
				return NULL;
		}
	}

getTag:

	*locationReturn = ftell(file) - 1;
	int allocatedTag = INITIAL_TAG_SIZE;
	int len = 0;
	char *buffer = malloc(allocatedTag);

	for (;;) {
		int c = fgetc(file);
		buffer[len] = c;

		if (c == '>' || c == EOF || isspace(c)) {
			buffer[len] = '\0';
			while (c != '>' && c != EOF)
				c = fgetc(file);
			return buffer;
		}

		len++;
		if (len == allocatedTag) {
			allocatedTag *= 2;
			buffer = realloc(buffer, allocatedTag);
		}
	}
}

int64_t searchForTag(FILE *file, char *search) {
	for (;;) {
		int64_t possibility;
		char *tag = nextTag(file, &possibility);
		if (tag == NULL)
			return -1;
		if (strcmp(tag, search) == 0) {
			free(tag);
			return possibility;
		}
		free(tag);
	}
}

void sanitizeAmpersands(FILE *input, FILE *output) {
	for (;;) {
		int c = fgetc(input);
		char special[MAX_SPECIAL_LEN];
		switch (c) {
			case '<': case EOF:
				fflush(output);
				return;
			case '&':
				readTillChar(input, special, sizeof(special), ';', false);
				for (int i = 0; i < sizeof(escapes) / sizeof(escapes[0]); i++) {
					if (strcmp(escapes[i].title, special) == 0) {
						fputc(escapes[i].c, output);
						break;
					}
				}
				break;
			default:
				fputc(c, output);
				break;
		}
	}
}
