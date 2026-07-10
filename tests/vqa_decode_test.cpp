#include <assert.h>
#include <string.h>

#include "windows.h"

extern "C" void MacVQA_UnVQ4x2(unsigned char const *codebook, unsigned char const *pointers, unsigned char *buffer, unsigned long blocks_per_row, unsigned long rows, unsigned long buffer_width);

bool MacSDL_SetMode(int, int) { return true; }
void MacSDL_Shutdown(void) {}
void MacSDL_SetPalette(PALETTEENTRY const *, int) {}
void MacSDL_Present8(unsigned char const *, int, int, int) {}
void MacSDL_PumpEvents(void) {}
bool MacSDL_QuitRequested(void) { return false; }
void Flag_To_Set_Palette(unsigned char *, long, unsigned long) {}
bool MacAudio_BeginMovieStream(int, int, int) { return true; }
bool MacAudio_QueueMovieStream(void const *, size_t) { return true; }
void MacAudio_ClearMovieStream(void) {}
void MacAudio_SetMovieStreamPaused(bool) {}
void MacAudio_EndMovieStream(void) {}

int main()
{
	unsigned char codebook[16];
	for (int i = 0; i < 8; ++i) {
		codebook[i] = (unsigned char)(i + 1);
		codebook[8 + i] = (unsigned char)(0x20 + i);
	}

	unsigned char pointers[4];
	pointers[0] = 1;
	pointers[1] = 0x55;
	pointers[2] = 0;
	pointers[3] = 0x0f;

	unsigned char frame[16];
	memset(frame, 0, sizeof(frame));
	MacVQA_UnVQ4x2(codebook, pointers, frame, 2, 1, 8);

	assert(frame[0] == 0x20);
	assert(frame[1] == 0x21);
	assert(frame[2] == 0x22);
	assert(frame[3] == 0x23);
	assert(frame[4] == 0x55);
	assert(frame[5] == 0x55);
	assert(frame[6] == 0x55);
	assert(frame[7] == 0x55);
	assert(frame[8] == 0x24);
	assert(frame[9] == 0x25);
	assert(frame[10] == 0x26);
	assert(frame[11] == 0x27);
	assert(frame[12] == 0x55);
	assert(frame[13] == 0x55);
	assert(frame[14] == 0x55);
	assert(frame[15] == 0x55);

	return 0;
}
