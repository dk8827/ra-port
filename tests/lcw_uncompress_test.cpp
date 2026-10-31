#include <assert.h>
#include <string.h>

extern "C" int LCW_Uncompress_Bounded(void const *, unsigned long, void *, unsigned long, unsigned long *);

int main()
{
	unsigned char dest[8] = {10, 20, 30, 40, 50, 60, 70, 80};
	unsigned long written = 99;
	unsigned char forward[] = {0xc0, 3, 0, 0x80};
	assert(LCW_Uncompress_Bounded(forward, sizeof(forward), dest, sizeof(dest), &written));
	assert(written == 3 && dest[0] == 40 && dest[1] == 50 && dest[2] == 60);

	unsigned char same[] = {0xc0, 0, 0, 0x80};
	assert(LCW_Uncompress_Bounded(same, sizeof(same), dest, sizeof(dest), &written));
	assert(written == 3 && dest[0] == 40 && dest[1] == 50 && dest[2] == 60);

	unsigned char long_copy[] = {0xff, 3, 0, 5, 0, 0x80};
	assert(LCW_Uncompress_Bounded(long_copy, sizeof(long_copy), dest, sizeof(dest), &written));
	assert(written == 3 && dest[0] == 60 && dest[1] == 70 && dest[2] == 80);

	unsigned char overlap[] = {0x81, 42, 0xff, 7, 0, 0, 0, 0x80};
	assert(LCW_Uncompress_Bounded(overlap, sizeof(overlap), dest, sizeof(dest), &written));
	assert(written == sizeof(dest));
	for (unsigned int i = 0; i < sizeof(dest); ++i) assert(dest[i] == 42);

	unsigned char past_end[] = {0xc0, 6, 0, 0x80};
	assert(!LCW_Uncompress_Bounded(past_end, sizeof(past_end), dest, sizeof(dest), &written));
	assert(written == 0);
	unsigned char invalid_offset[] = {0xc0, 0xff, 0xff, 0x80};
	assert(!LCW_Uncompress_Bounded(invalid_offset, sizeof(invalid_offset), dest, sizeof(dest), &written));
	unsigned char overflow[] = {0xff, 9, 0, 0, 0, 0x80};
	assert(!LCW_Uncompress_Bounded(overflow, sizeof(overflow), dest, sizeof(dest), &written));
	unsigned char truncated[] = {0xff, 3, 0, 0};
	assert(!LCW_Uncompress_Bounded(truncated, sizeof(truncated), dest, sizeof(dest), &written));
	unsigned char bad_relative[] = {0x00, 1, 0x80};
	assert(!LCW_Uncompress_Bounded(bad_relative, sizeof(bad_relative), dest, sizeof(dest), &written));
	return 0;
}
