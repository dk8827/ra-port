#ifndef WSA_FILE_FORMAT_H
#define WSA_FILE_FORMAT_H

#include <stdint.h>
#include <string.h>

// 6 u16 fields + i16 flags = 14 bytes before the frame offset table.
#define WSA_FILE_HEADER_SIZE 14
#define WSA_FILE_OFFSET_SIZE 4

static inline uint32_t WSA_Read_File_Offset(char const *file_buffer, int frame)
{
	uint32_t offset = 0;

	if (file_buffer == NULL || frame < 0) {
		return 0;
	}

	memcpy(&offset, file_buffer + ((unsigned long)frame * WSA_FILE_OFFSET_SIZE), sizeof(offset));
	return offset;
}

static inline unsigned long WSA_Resident_Frame_Offset(char const *file_buffer, int frame)
{
	uint32_t frame0_start = WSA_Read_File_Offset(file_buffer, 0);
	uint32_t frame0_end = WSA_Read_File_Offset(file_buffer, 1);
	uint32_t offset = WSA_Read_File_Offset(file_buffer, frame);
	uint32_t frame0_size = 0;
	uint32_t resident_adjust;

	if (frame0_start) {
		if (frame0_end < frame0_start) {
			return 0L;
		}
		frame0_size = frame0_end - frame0_start;
	}

	if (!offset) {
		return 0L;
	}

	resident_adjust = frame0_size + WSA_FILE_HEADER_SIZE;
	if (offset < resident_adjust) {
		return 0L;
	}

	return (unsigned long)(offset - resident_adjust);
}

#endif
