#pragma once

#include <stddef.h>

bool MacAudio_BeginMovieStream(int rate, int channels, int bits);
bool MacAudio_QueueMovieStream(void const *data, size_t bytes);
void MacAudio_ClearMovieStream(void);
void MacAudio_SetMovieStreamPaused(bool paused);
void MacAudio_EndMovieStream(void);
