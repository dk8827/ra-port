#include <VQA32/VQAPLAY.H>

#include <stdio.h>
#include <string.h>

#include "windows.h"

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

static unsigned char FrameBuffer[320 * 200];
static int FramesDrawn = 0;

static int fail(char const *message)
{
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

static long Callback(unsigned char *, long)
{
	FramesDrawn++;
	return FramesDrawn >= 3;
}

static long FileHandler(VQAHandle *vqa, long action, void *buffer, long nbytes)
{
	FILE *file = (FILE *)vqa->VQAio;
	switch (action) {
		case VQACMD_OPEN:
			file = fopen((char const *)buffer, "rb");
			vqa->VQAio = (unsigned long)file;
			return file ? 0 : 1;
		case VQACMD_CLOSE:
			if (file) fclose(file);
			vqa->VQAio = 0;
			return 0;
		case VQACMD_READ:
			return fread(buffer, 1, (size_t)nbytes, file) == (size_t)nbytes ? 0 : 1;
		case VQACMD_SEEK:
			return fseek(file, nbytes, SEEK_CUR) == 0 ? 0 : 1;
		default:
			return 0;
	}
}

int main(void)
{
	char const *path = "assets/redalert/allies/SIZZLE.VQA";
	FILE *probe = fopen(path, "rb");
	if (!probe) {
		printf("skipping VQA file smoke test; %s not present\n", path);
		return 0;
	}
	fclose(probe);

	VQAHandle *handle = VQA_Alloc();
	if (!handle) return fail("VQA_Alloc failed");
	VQA_Init(handle, FileHandler);

	VQAConfig config;
	VQA_DefaultConfig(&config);
	config.DrawerCallback = Callback;
	config.ImageBuf = FrameBuffer;
	config.ImageWidth = 320;
	config.ImageHeight = 200;

	if (VQA_Open(handle, path, &config) != VQAERR_NONE) {
		VQA_Free(handle);
		return fail("VQA_Open failed on SIZZLE.VQA");
	}

	VQAInfo info;
	VQA_GetInfo(handle, &info);
	if (info.NumFrames <= 0 || info.ImageWidth != 320 || info.ImageHeight != 200) {
		VQA_Close(handle);
		VQA_Free(handle);
		return fail("unexpected VQA metadata");
	}

	long rc = VQA_Play(handle, VQAMODE_RUN);
	if (rc != VQAERR_EOF || FramesDrawn != 3) {
		VQA_Close(handle);
		VQA_Free(handle);
		return fail("VQA_Play did not draw and stop through callback");
	}

	VQAStatistics stats;
	VQA_GetStats(handle, &stats);
	if (stats.SamplesPlayed == 0) {
		VQA_Close(handle);
		VQA_Free(handle);
		return fail("VQA audio chunks were not decoded");
	}

	VQA_Close(handle);
	VQA_Free(handle);
	return 0;
}
