/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#include <windows.h>
#include <stdio.h>
#include	<string.h>


#define	bool	int

/*
** Prototypes for linkage to assembly module
*/
extern "C" {
	bool __cdecl Detect_MMX_Availability (void);

	extern char CPUType;
	extern char VendorID;
}


/***********************************************************************************************
 * Get_CPU_Type -- Find out what kind of CPU we are running on                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    int   - reference to cpu type                                                     *
 *           bool  - reference to mmx availability flag                                        *
 *           char* - ptr to buffer to receive chip vendor info                                 *
 *           int   - length of above buffer                                                    *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/26/96 10:15AM ST : Created                                                             *
 *=============================================================================================*/
void Get_CPU_Type(int & cpu_type, bool & mmx, char * vendor_id, int vendor_id_length)
{
	/*
	** Call the asm CPU detection code
	*/
	mmx = Detect_MMX_Availability();

	/*
	** Return the promised results
	*/
	cpu_type = (int)CPUType;
	char * vendor_ptr = &VendorID;
	strncpy(vendor_id, vendor_ptr, vendor_id_length);
}









