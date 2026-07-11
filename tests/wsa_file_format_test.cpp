#include "wsa_file_format.h"

#include <assert.h>
#include <string.h>

int main(void)
{
	unsigned char offsets[16];
	memset(offsets, 0, sizeof(offsets));
	offsets[0] = 0x20;
	offsets[4] = 0x40;
	offsets[8] = 0x70;
	offsets[12] = 0xa0;

	assert(WSA_FILE_OFFSET_SIZE == 4);
	assert(WSA_FILE_HEADER_SIZE == 14);
	assert(WSA_Read_File_Offset((char const *)offsets, 0) == 0x20);
	assert(WSA_Read_File_Offset((char const *)offsets, 1) == 0x40);
	assert(WSA_Resident_Frame_Offset((char const *)offsets, 1) == 0x12);
	assert(WSA_Resident_Frame_Offset((char const *)offsets, 2) == 0x42);

	return 0;
}
