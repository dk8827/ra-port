#include <iff.h>

#include <stdio.h>
#include <string.h>

static int fail(char const *message)
{
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

void Mem_Copy(void const *source, void *dest, unsigned long bytes_to_copy)
{
	memmove(dest, source, bytes_to_copy);
}

int Open_File(char const *, int) { return -1; }
void Close_File(int) {}
long Read_File(int, void *, unsigned long) { return 0; }
long Write_File(int, void const *, unsigned long) { return 0; }
unsigned long Seek_File(int, long, int) { return 0; }
unsigned long File_Size(int) { return 0; }
void *Alloc(unsigned long, MemoryFlagType) { return 0; }
void Free(void const *) {}
int main(void)
{
	unsigned char source[32];
	unsigned char dest[4];
	memset(source, 0, sizeof(source));
	memset(dest, 0, sizeof(dest));

	source[0] = NOCOMPRESS;
	source[2] = 3;
	source[8] = 0x11;
	source[9] = 0x22;
	source[10] = 0x33;

	/*
	** If the implementation accidentally uses the host CompHeaderType layout,
	** these bytes make the old 64-bit mac parser copy from offset 24 instead.
	*/
	source[8] = 3;
	source[24] = 0xaa;
	source[25] = 0xbb;
	source[26] = 0xcc;

	unsigned long size = Uncompress_Data(source, dest);
	if (size != 3) return fail("uncompressed size should come from CPS byte offset 2");
	if (dest[0] != 3 || dest[1] != 0x22 || dest[2] != 0x33) {
		return fail("uncompressed bytes should start after the 8-byte CPS header");
	}

	memset(dest, 0, sizeof(dest));
	if (Uncompress_Data_Bounded(source, 11, dest, sizeof(dest)) != 3) {
		return fail("bounded CPS decode should accept valid input");
	}
	if (Uncompress_Data_Bounded(source, 10, dest, sizeof(dest)) != 0) {
		return fail("bounded CPS decode should reject truncated input");
	}
	source[2] = 5;
	if (Uncompress_Data_Bounded(source, 11, dest, sizeof(dest)) != 0) {
		return fail("bounded CPS decode should reject oversized output");
	}
	source[2] = 1;
	source[0] = LCW;
	source[8] = 0x81;
	if (Uncompress_Data_Bounded(source, 9, dest, sizeof(dest)) != 0) {
		return fail("bounded CPS decode should reject truncated LCW data");
	}

	return 0;
}
