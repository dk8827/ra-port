/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef PALETTE_H
#define PALETTE_H

/*
********************************* Constants *********************************
*/
#define RGB_BYTES			3
#define PALETTE_SIZE		256
#define PALETTE_BYTES	768

/*
******************************** Prototypes *********************************
*/
/*
-------------------------------- Palette.cpp --------------------------------
*/
VOID cdecl Set_Palette(VOID *palette);
VOID cdecl Set_Palette_Color(VOID *palette, WORD color, VOID *data);
VOID Fade_Palette_To(VOID *palette1, UWORD delay, VOID (*callback)() );

/*
-------------------------------- loadpal.cpp --------------------------------
*/
VOID cdecl Load_Palette(BYTE *palette_file_name, VOID *palette_pointer);

/*
------------------------------- morphpal.cpp --------------------------------
*/
VOID cdecl Morph_Palette (VOID *src_palette, VOID *dst_palette, UWORD delay,
	VOID *callback);

/*
---------------------------------- pal.asm ----------------------------------
*/
#ifdef __cplusplus
extern "C" {
#endif

extern VOID Set_Palette_Range(VOID *palette);
extern BOOL Bump_Color(VOID *palette, WORD changable, WORD target);

#ifdef __cplusplus
}
#endif
extern "C" UBYTE  CurrentPalette[];		/* in pal.asm */


#endif // PALETTE_H

/***************************** End of palette.h ****************************/

