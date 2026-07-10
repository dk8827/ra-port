#include <audio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mac_audio_stream.h"

int __cdecl Find_File(char const *)
{
	return 0;
}

int __cdecl Open_File(char const *, int)
{
	return -1;
}

void __cdecl Close_File(int)
{
}

long __cdecl Read_File(int, void *, unsigned long)
{
	return 0;
}

unsigned long __cdecl File_Size(int)
{
	return 0;
}

SFX_Type SoundType = SFX_NONE;
Sample_Type SampleType = SAMPLE_NONE;

static void fail(char const *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}

int main()
{
	setenv("SDL_AUDIODRIVER", "dummy", 1);

	if (sizeof(AUDHeaderType) != 12) fail("AUDHeaderType must match packed AUD file header");
	if (offsetof(AUDHeaderType, Size) != 2) fail("AUDHeaderType Size offset changed");
	if (offsetof(AUDHeaderType, Flags) != 10) fail("AUDHeaderType Flags offset changed");

	if (!Audio_Init(0, 16, FALSE, 22050, 0)) fail("Audio_Init failed");

	unsigned char movie_pcm[8] = {0x00, 0x80, 0x40, 0x80, 0x80, 0x80, 0xC0, 0x80};
	if (!MacAudio_BeginMovieStream(22050, 1, 16)) fail("movie stream failed to use initialized audio device");
	if (!MacAudio_QueueMovieStream(movie_pcm, sizeof(movie_pcm))) fail("movie PCM queue failed");
	MacAudio_SetMovieStreamPaused(true);
	MacAudio_SetMovieStreamPaused(false);
	MacAudio_ClearMovieStream();
	MacAudio_EndMovieStream();

	enum { PayloadBytes = 4096 };
	unsigned char sample[sizeof(AUDHeaderType) + PayloadBytes];
	AUDHeaderType header;
	header.Rate = 11025;
	header.Size = PayloadBytes;
	header.UncompSize = PayloadBytes;
	header.Flags = 0;
	header.Compression = 0;
	memcpy(sample, &header, sizeof(header));

	for (int index = 0; index < PayloadBytes; ++index) {
		sample[sizeof(header) + index] = (unsigned char)(0x80 + ((index % 64) - 32));
	}

	if (Sample_Length(sample) <= 0) fail("Sample_Length returned zero");

	int handle = Play_Sample(sample, 255, 255, 0);
	if (handle < 0) fail("Play_Sample returned no handle");
	if (!Sample_Status(handle)) fail("Sample_Status was false after playback");
	if (!Is_Sample_Playing(sample)) fail("Is_Sample_Playing was false after playback");

	Stop_Sample(handle);
	if (Sample_Status(handle)) fail("Sample_Status stayed true after Stop_Sample");
	if (Is_Sample_Playing(sample)) fail("Is_Sample_Playing stayed true after Stop_Sample");

	Sound_End();
	return 0;
}
