#ifndef SAVEDESCRIPTION_H
#define SAVEDESCRIPTION_H

#include <stddef.h>
#include <string.h>

static inline void Normalize_Save_Description(char *description, size_t capacity)
{
	if (description == NULL || capacity == 0) return;

	description[capacity - 1] = '\0';
	size_t length = strlen(description);
	if (length >= 2 && description[length - 2] == '\r' && description[length - 1] == '\n') {
		description[length - 2] = '\0';
	}
}

#endif
