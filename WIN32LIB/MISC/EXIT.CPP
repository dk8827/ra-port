/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include "misc.h"

#include <stdio.h>

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>




/***************************************************************************
 * EXIT -- Exit routine with message.                                      *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/03/1994 SKB : Created.                                             *
 *=========================================================================*/
void __cdecl Exit(INT errorval, const char *message, ...)
{
	va_list		argptr;
	char		errstring[256];

	Prog_End();

	if (message && *message) {
		va_start (argptr, message);
		vsprintf ((char *)errstring, (const char *)message, argptr);
		va_end (argptr);
		printf("%s", errstring);
	}

	::exit(errorval);

}

void randomize	( void )
{
  srand ( time ( NULL ) ) ;
}

#if(0)
unsigned long	random ( unsigned long mod )
{
  return rand () * mod / RAND_MAX ;
}
#endif
