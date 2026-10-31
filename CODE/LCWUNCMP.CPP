/*
 * Copyright 2025 Electronic Arts Inc.
 *
 * Derived from Electronic Arts Inc.'s public source release for
 * Command & Conquer: Red Alert.
 * Licensed under GPLv3-or-later with additional Electronic Arts terms.
 * Modified for the macOS port, 2026.
 * See LICENSE.md.
 */

extern "C" {

int __cdecl LCW_Uncompress_Bounded(void const *source, unsigned long source_length, void *dest, unsigned long dest_length, unsigned long *written)
{
	unsigned char const *source_ptr = (unsigned char const *)source;
	unsigned char *dest_ptr = (unsigned char *)dest;
	unsigned long source_pos = 0;
	unsigned long dest_pos = 0;

	if (!source || !dest || !written) return 0;
	*written = 0;

	while (source_pos < source_length) {
		unsigned char op_code = source_ptr[source_pos++];
		unsigned long count;
		unsigned long offset;

		if (!(op_code & 0x80)) {
			if (source_pos >= source_length) return 0;
			count = (op_code >> 4) + 3;
			offset = source_ptr[source_pos++] + (((unsigned long)op_code & 0x0f) << 8);
			if (offset == 0 || offset > dest_pos || count > dest_length - dest_pos) return 0;
			while (count--) {
				dest_ptr[dest_pos] = dest_ptr[dest_pos - offset];
				dest_pos++;
			}
			continue;
		}

		if (!(op_code & 0x40)) {
			if (op_code == 0x80) {
				*written = dest_pos;
				return 1;
			}
			count = op_code & 0x3f;
			if (count > source_length - source_pos || count > dest_length - dest_pos) return 0;
			while (count--) dest_ptr[dest_pos++] = source_ptr[source_pos++];
			continue;
		}

		if (op_code == 0xfe) {
			if (source_length - source_pos < 3) return 0;
			count = source_ptr[source_pos] + ((unsigned long)source_ptr[source_pos + 1] << 8);
			unsigned char data = source_ptr[source_pos + 2];
			source_pos += 3;
			if (count > dest_length - dest_pos) return 0;
			while (count--) dest_ptr[dest_pos++] = data;
			continue;
		}

		if (op_code == 0xff) {
			if (source_length - source_pos < 4) return 0;
			count = source_ptr[source_pos] + ((unsigned long)source_ptr[source_pos + 1] << 8);
			offset = source_ptr[source_pos + 2] + ((unsigned long)source_ptr[source_pos + 3] << 8);
			source_pos += 4;
		} else {
			if (source_length - source_pos < 2) return 0;
			count = (op_code & 0x3f) + 3;
			offset = source_ptr[source_pos] + ((unsigned long)source_ptr[source_pos + 1] << 8);
			source_pos += 2;
		}
		/* VQA deltas may copy from the previous frame, including bytes at
		** or beyond the current write cursor. Bound both ranges, not their
		** relative positions; the destination is initialized by the caller. */
		if (offset > dest_length || count > dest_length - offset || count > dest_length - dest_pos) return 0;
		while (count--) dest_ptr[dest_pos++] = dest_ptr[offset++];
	}

	return 0;
}

/***************************************************************************
 * LCW_UNCOMPRESS -- Decompress an LCW encoded data block.                 *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 * Command code, n        |Description                                     *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy    |short copy back y bytes and run x+3   from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1          |med copy from dest x+3 bytes from offset w1     *
 * n=11111111,w1,w2       |long copy from dest w1 bytes from offset w2     *
 * n=11111110,w1,b1       |long run of byte b1 for w1 bytes                *
 * n=10000000             |end of data reached                             *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned long length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned long # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *=========================================================================*/
unsigned long __cdecl LCW_Uncompress (void * source, void * dest, unsigned long )
//unsigned long LCW_Uncompress (void * source, void * dest, unsigned long length)
{
	unsigned char * source_ptr, * dest_ptr, * copy_ptr, op_code, data;
	unsigned	  count, * word_dest_ptr, word_data;

	/* Copy the source and destination ptrs. */
	source_ptr = (unsigned char*) source;
	dest_ptr   = (unsigned char*) dest;

	while (1 /*TRUE*/) {

		/* Read in the operation code. */
		op_code = *source_ptr++;

		if (!(op_code & 0x80)) {

			/* Do a short copy from destination. */
			count	 = (op_code >> 4) + 3;
			copy_ptr = dest_ptr - ((unsigned) *source_ptr++ + (((unsigned) op_code & 0x0f) << 8));

			while (count--) *dest_ptr++ = *copy_ptr++;

		} else {

			if (!(op_code & 0x40)) {

				if (op_code == 0x80) {

					/* Return # of destination bytes written. */
					return ((unsigned long) (dest_ptr - (unsigned char*) dest));

				} else {

					/* Do a medium copy from source. */
					count = op_code & 0x3f;

					while (count--) *dest_ptr++ = *source_ptr++;
				}

			} else {

				if (op_code == 0xfe) {

					/* Do a long run. */
					count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
					word_data = data = *(source_ptr + 2);
					word_data  = (word_data << 24) + (word_data << 16) + (word_data << 8) + word_data;
					source_ptr += 3;

					copy_ptr = dest_ptr + 4 - ((unsigned long) dest_ptr & 0x3);
					count -= (copy_ptr - dest_ptr);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

					word_dest_ptr = (unsigned*) dest_ptr;

					dest_ptr += (count & 0xfffffffc);

					while (word_dest_ptr < (unsigned*) dest_ptr) {
						*word_dest_ptr		= word_data;
						*(word_dest_ptr + 1) = word_data;
						word_dest_ptr += 2;
					}

					copy_ptr = dest_ptr + (count & 0x3);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

				} else {

					if (op_code == 0xff) {

						/* Do a long copy from destination. */
						count	 = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						copy_ptr = (unsigned char*) dest + *(source_ptr + 2) + ((unsigned) *(source_ptr + 3) << 8);
						source_ptr += 4;

						while (count--) *dest_ptr++ = *copy_ptr++;

					} else {

						/* Do a medium copy from destination. */
						count = (op_code & 0x3f) + 3;
						copy_ptr = (unsigned char*) dest + *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						source_ptr += 2;

						while (count--) *dest_ptr++ = *copy_ptr++;
					}
				}
			}
		}
	}
}

}
