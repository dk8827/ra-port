/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

#ifndef RA_ENDIAN_H
#define RA_ENDIAN_H

/*
** Some modern platform headers define BIG_ENDIAN as a numeric byte-order
** constant even when the target is little-endian. The legacy code used
** `#ifdef BIG_ENDIAN` as a host-endian check, so keep the decision local and
** require an actual byte-order comparison when those constants are present.
*/
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define RA_BIG_ENDIAN 1
#elif defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)
#define RA_BIG_ENDIAN 1
#elif defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN)
#define RA_BIG_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(_M_PPC)
#define RA_BIG_ENDIAN 1
#elif defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) && !defined(BYTE_ORDER) && !defined(_BYTE_ORDER) && !defined(__BYTE_ORDER__)
#define RA_BIG_ENDIAN 1
#else
#define RA_BIG_ENDIAN 0
#endif

#endif
