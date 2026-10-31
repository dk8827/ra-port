/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include	<ctype.h>
#include	<string.h>
#include	"wwfile.h"
#include	"xstraw.h"
#include	"readline.h"


// Disable the "temporary object used to initialize a non-constant reference" warning.
#pragma warning 665 9


void strtrim(char * buffer)
{
	if (buffer) {

		/*
		**	Strip leading white space from the string.
		*/
		char * source = buffer;
		while (isspace(*source)) {
			source++;
		}
		if (source != buffer) {
			memmove(buffer, source, strlen(source) + 1);
		}

		/*
		**	Clip trailing white space from the string.
		*/
		for (int index = strlen(buffer)-1; index >= 0; index--) {
			if (isspace(buffer[index])) {
				buffer[index] = '\0';
			} else {
				break;
			}
		}
	}
}



int Read_Line(FileClass & file, char * buffer, int len, bool & eof)
{
	FileStraw straw(file);
	return(Read_Line(straw, buffer, len, eof));
}


int Read_Line(Straw & file, char * buffer, int len, bool & eof)
{
	if (len == 0 || buffer == NULL) return(0);

	int count = 0;
	for (;;) {
		char c;
		if (file.Get(&c, sizeof(c)) != sizeof(c)) {
			eof = true;
			buffer[0] = '\0';
			break;
		}

		if (c == '\x0A') break;
		if (c != '\x0D' && count+1 < len) {
			buffer[count++] = c;
		}
	}
	buffer[count] = '\0';

	strtrim(buffer);
	return(strlen(buffer));
}
