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

#include <gbuffer.h>
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
void __cdecl Set_Palette(void *palette);
void __cdecl Set_Palette_Color(void *palette, int color, void *data);
void Fade_Palette_To(void *palette1, unsigned int delay, void (*callback)() );

/*
-------------------------------- loadpal.cpp --------------------------------
*/
void __cdecl Load_Palette(char *palette_file_name, void *palette_pointer);

/*
------------------------------- morphpal.cpp --------------------------------
*/
void __cdecl Morph_Palette (void *src_palette, void *dst_palette, unsigned int delay,
	void *callback);

/*
---------------------------------- pal.asm ----------------------------------
*/
#ifdef __cplusplus
extern "C" {
#endif

extern void __cdecl Set_Palette_Range(void *palette);
extern BOOL __cdecl Bump_Color(void *palette, int changable, int target);

#ifdef __cplusplus
}
#endif
extern "C" unsigned char  CurrentPalette[];		/* in pal.asm */


#endif // PALETTE_H

/***************************** End of palette.h ****************************/
