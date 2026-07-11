#include <windows.h>
#include "AUDIO.H"
#include "FILE.H"

#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../include/mac_audio_stream.h"

void (*Audio_Focus_Loss_Function)(void) = 0;

extern SFX_Type SoundType;
extern Sample_Type SampleType;

namespace {

static const int kMaxSamples = 5;
static const unsigned long kAudMagic = 0x0000DEAFUL;
static const long kMaxAudBytes = 128L * 1024L * 1024L;

struct DecodedAudio {
	std::vector<short> pcm;
	int rate;
	int channels;
};

struct PlayingSample {
	bool active;
	bool is_score;
	void const *original;
	int priority;
	int volume;
	int pan;
	unsigned long position;
	unsigned long step;
	std::vector<short> pcm;
	int rate;
	int channels;
};

struct MovieStream {
	bool active;
	bool paused;
	int rate;
	int channels;
	int bits;
	unsigned long position;
	unsigned long step;
	std::vector<short> pcm;
};

static SDL_AudioDeviceID AudioDevice = 0;
static int AudioRate = 22050;
static int SoundVolume = 255;
static int ScoreVolume = 255;
static bool AudioReady = false;
static PlayingSample Samples[kMaxSamples];
static MovieStream MovieAudio;

static int clamp_int(int value, int low, int high)
{
	if (value < low) return low;
	if (value > high) return high;
	return value;
}

static short clamp_s16(long long value)
{
	if (value < -32768) return -32768;
	if (value > 32767) return 32767;
	return (short)value;
}

static int api_volume_to_255(int volume)
{
	if (volume > 255) volume = (volume + 128) / 256;
	return clamp_int(volume, 0, 255);
}

static unsigned short read_le16(unsigned char const *ptr)
{
	return (unsigned short)((unsigned int)ptr[0] | ((unsigned int)ptr[1] << 8));
}

static unsigned long read_le32(unsigned char const *ptr)
{
	return (unsigned long)ptr[0] |
		((unsigned long)ptr[1] << 8) |
		((unsigned long)ptr[2] << 16) |
		((unsigned long)ptr[3] << 24);
}

static void reset_sample(PlayingSample &sample)
{
	sample.active = false;
	sample.is_score = false;
	sample.original = 0;
	sample.priority = 0;
	sample.volume = 255;
	sample.pan = 0;
	sample.position = 0;
	sample.step = 0;
	sample.pcm.clear();
	sample.rate = 0;
	sample.channels = 1;
}

static void reset_movie_stream()
{
	MovieAudio.active = false;
	MovieAudio.paused = false;
	MovieAudio.rate = 0;
	MovieAudio.channels = 0;
	MovieAudio.bits = 0;
	MovieAudio.position = 0;
	MovieAudio.step = 0;
	MovieAudio.pcm.clear();
}

static void lock_audio()
{
	if (AudioDevice) SDL_LockAudioDevice(AudioDevice);
}

static void unlock_audio()
{
	if (AudioDevice) SDL_UnlockAudioDevice(AudioDevice);
}

static void convert_raw_pcm(unsigned char const *source, long size, int flags, DecodedAudio &decoded)
{
	int channels = (flags & AUD_FLAG_STEREO) ? 2 : 1;
	int bytes_per_sample = (flags & AUD_FLAG_16BIT) ? 2 : 1;
	long frame_count = size / (channels * bytes_per_sample);
	if (frame_count <= 0) return;

	decoded.channels = channels;
	decoded.pcm.reserve((size_t)frame_count * channels);

	if (flags & AUD_FLAG_16BIT) {
		for (long index = 0; index < frame_count * channels; ++index) {
			unsigned char const *ptr = source + index * 2;
			int value = (int)(int16_t)read_le16(ptr);
			decoded.pcm.push_back((short)value);
		}
	} else {
		for (long index = 0; index < frame_count * channels; ++index) {
			int value = ((int)source[index] - 128) << 8;
			decoded.pcm.push_back((short)value);
		}
	}
}

static long decompress_westwood_frame(unsigned char const *source, long source_size, unsigned char *dest, long dest_size)
{
	static signed char const decode2[4] = {-2, -1, 0, 1};
	static signed char const decode4[16] = {-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8};

	long in = 0;
	long out = 0;
	int previous = 0x80;

	while (out < dest_size && in < source_size) {
		unsigned char code = source[in++];
		int control = code >> 6;
		int data = code & 0x3F;

		if (control == 2) {
			if (data & 0x20) {
				int delta = data & 0x1F;
				if (delta & 0x10) delta -= 0x20;
				previous = (previous + delta) & 0xFF;
				dest[out++] = (unsigned char)previous;
			} else {
				int count = data + 1;
				while (count-- > 0 && out < dest_size && in < source_size) {
					previous = source[in++];
					dest[out++] = (unsigned char)previous;
				}
			}
			continue;
		}

		if (control == 1) {
			int count = data + 1;
			while (count-- > 0 && out < dest_size && in < source_size) {
				unsigned char packed = source[in++];
				int low = packed & 0x0F;
				previous = clamp_int(previous + decode4[low], 0, 255);
				dest[out++] = (unsigned char)previous;
				if (out >= dest_size) break;
				int high = packed >> 4;
				previous = clamp_int(previous + decode4[high], 0, 255);
				dest[out++] = (unsigned char)previous;
			}
			continue;
		}

		if (control == 0) {
			int count = data + 1;
			while (count-- > 0 && out < dest_size && in < source_size) {
				unsigned char packed = source[in++];
				for (int shift = 0; shift < 8 && out < dest_size; shift += 2) {
					int token = (packed >> shift) & 0x03;
					previous = clamp_int(previous + decode2[token], 0, 255);
					dest[out++] = (unsigned char)previous;
				}
			}
			continue;
		}

		int count = data + 1;
		while (count-- > 0 && out < dest_size) {
			dest[out++] = (unsigned char)previous;
		}
	}

	return out;
}

struct SosState {
	int predictor;
	int index;
};

static int ima_step(int index)
{
	static int const table[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};
	return table[clamp_int(index, 0, 88)];
}

static int ima_index_delta(int nibble)
{
	static int const table[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
	return table[nibble & 0x0F];
}

static short decode_ima_nibble(SosState &state, int nibble)
{
	int step = ima_step(state.index);
	int diff = step >> 3;
	if (nibble & 1) diff += step >> 2;
	if (nibble & 2) diff += step >> 1;
	if (nibble & 4) diff += step;
	if (nibble & 8) diff = -diff;

	state.predictor = clamp_int(state.predictor + diff, -32768, 32767);
	state.index = clamp_int(state.index + ima_index_delta(nibble), 0, 88);
	return (short)state.predictor;
}

static long decompress_sos_16_mono(unsigned char const *source, long source_size, unsigned char *dest, long dest_size, SosState &state)
{
	long samples = dest_size / 2;
	long out_samples = 0;

	for (long in = 0; in < source_size && out_samples < samples; ++in) {
		unsigned char packed = source[in];
		int nibbles[2] = {packed & 0x0F, packed >> 4};
		for (int index = 0; index < 2 && out_samples < samples; ++index) {
			short sample = decode_ima_nibble(state, nibbles[index]);
			dest[out_samples * 2] = (unsigned char)(sample & 0xFF);
			dest[out_samples * 2 + 1] = (unsigned char)(((unsigned short)sample >> 8) & 0xFF);
			++out_samples;
		}
	}

	return out_samples * 2;
}

static bool decode_framed_audio(unsigned char const *source, long source_size, long uncomp_size, int compression, int flags, std::vector<unsigned char> &raw)
{
	long in = 0;
	SosState sos = {0, 0};
	raw.clear();
	if (!source || source_size <= 0 || uncomp_size <= 0 || source_size > kMaxAudBytes || uncomp_size > kMaxAudBytes) return false;
	if (uncomp_size > 0) raw.reserve((size_t)uncomp_size);

	while (in + 8 <= source_size) {
		unsigned short frame_size = read_le16(source + in);
		unsigned short decoded_size = read_le16(source + in + 2);
		unsigned long magic = read_le32(source + in + 4);
		in += 8;

		if (magic != kAudMagic || in + frame_size > source_size) return false;

		size_t old_size = raw.size();
		raw.resize(old_size + decoded_size);
		unsigned char *dest = &raw[old_size];

		if (frame_size == decoded_size) {
			memcpy(dest, source + in, decoded_size);
		} else if (compression == 1) {
			long decoded = decompress_westwood_frame(source + in, frame_size, dest, decoded_size);
			if (decoded < decoded_size) memset(dest + decoded, 0x80, decoded_size - decoded);
		} else if (compression == 99 && (flags & AUD_FLAG_16BIT) && !(flags & AUD_FLAG_STEREO)) {
			long decoded = decompress_sos_16_mono(source + in, frame_size, dest, decoded_size, sos);
			if (decoded < decoded_size) memset(dest + decoded, 0, decoded_size - decoded);
		} else {
			return false;
		}

		in += frame_size;
	}

	return !raw.empty();
}

static bool decode_aud_sample(void const *sample, DecodedAudio &decoded)
{
	if (!sample) return false;

	AUDHeaderType header;
	memcpy(&header, sample, sizeof(header));
	if (header.Rate < 4000 || header.Size <= 0 || header.Size > kMaxAudBytes || header.UncompSize <= 0 || header.UncompSize > kMaxAudBytes) return false;
	if ((header.Flags & ~(AUD_FLAG_STEREO | AUD_FLAG_16BIT)) != 0) return false;

	unsigned char const *source = (unsigned char const *)sample + sizeof(header);
	decoded.rate = header.Rate;
	decoded.channels = (header.Flags & AUD_FLAG_STEREO) ? 2 : 1;
	decoded.pcm.clear();

	if (header.Compression == 0) {
		convert_raw_pcm(source, header.Size, header.Flags, decoded);
		return !decoded.pcm.empty();
	}

	if (header.Compression == 1 || header.Compression == 99) {
		std::vector<unsigned char> raw;
		if (!decode_framed_audio(source, header.Size, header.UncompSize, header.Compression, header.Flags, raw)) return false;
		convert_raw_pcm(raw.empty() ? 0 : &raw[0], (long)raw.size(), header.Flags, decoded);
		return !decoded.pcm.empty();
	}

	return false;
}

static long sample_frame_count(PlayingSample const &sample)
{
	if (sample.channels <= 0) return 0;
	return (long)(sample.pcm.size() / sample.channels);
}

static int free_handle_for_priority(int priority)
{
	for (int id = kMaxSamples - 1; id >= 0; --id) {
		if (!Samples[id].active) return id;
	}

	for (int id = 0; id < kMaxSamples; ++id) {
		if (Samples[id].priority <= priority) {
			reset_sample(Samples[id]);
			return id;
		}
	}

	return -1;
}

static void mix_audio(void *, Uint8 *stream, int len)
{
	memset(stream, 0, (size_t)len);
	if (!AudioReady || len <= 0) return;

	short *out = (short *)stream;
	int frames = len / (int)(sizeof(short) * 2);

	for (int id = 0; id < kMaxSamples; ++id) {
		PlayingSample &sample = Samples[id];
		if (!sample.active || sample.pcm.empty()) continue;

		long total_frames = sample_frame_count(sample);
		if (total_frames <= 0) {
			reset_sample(sample);
			continue;
		}

		int master = sample.is_score ? ScoreVolume : SoundVolume;
		int base_volume = clamp_int((master * sample.volume) / 255, 0, 255);
		int pan = clamp_int(sample.pan, -32768, 32767);
		int left_pan = 255;
		int right_pan = 255;
		if (pan < 0) {
			right_pan = (int)(((long long)(32768 + pan) * 255) / 32768);
		} else if (pan > 0) {
			left_pan = (int)(((long long)(32768 - pan) * 255) / 32768);
		}

		for (int frame = 0; frame < frames; ++frame) {
			long source_frame = (long)(sample.position >> 16);
			if (source_frame >= total_frames) {
				reset_sample(sample);
				break;
			}

			short left = sample.pcm[(size_t)source_frame * sample.channels];
			short right = (sample.channels > 1) ? sample.pcm[(size_t)source_frame * sample.channels + 1] : left;
			int out_index = frame * 2;

			long long mixed_left = out[out_index] + ((long long)left * base_volume * left_pan) / (255LL * 255LL);
			long long mixed_right = out[out_index + 1] + ((long long)right * base_volume * right_pan) / (255LL * 255LL);
			out[out_index] = clamp_s16(mixed_left);
			out[out_index + 1] = clamp_s16(mixed_right);
			sample.position += sample.step;
		}
	}

	if (!MovieAudio.active || MovieAudio.paused || MovieAudio.pcm.empty() || MovieAudio.channels <= 0) {
		return;
	}

	long movie_frames = (long)(MovieAudio.pcm.size() / (size_t)MovieAudio.channels);
	for (int frame = 0; frame < frames; ++frame) {
		long source_frame = (long)(MovieAudio.position >> 16);
		if (source_frame >= movie_frames) {
			break;
		}

		short left = MovieAudio.pcm[(size_t)source_frame * (size_t)MovieAudio.channels];
		short right = MovieAudio.channels > 1 ? MovieAudio.pcm[(size_t)source_frame * (size_t)MovieAudio.channels + 1] : left;
		int out_index = frame * 2;
		out[out_index] = clamp_s16(out[out_index] + ((long long)left * SoundVolume) / 255LL);
		out[out_index + 1] = clamp_s16(out[out_index + 1] + ((long long)right * SoundVolume) / 255LL);
		MovieAudio.position += MovieAudio.step;
	}
}

}

int File_Stream_Sample(char const *filename, BOOL real_time_start)
{
	return File_Stream_Sample_Vol(filename, 255, real_time_start);
}

int File_Stream_Sample_Vol(char const *filename, int volume, BOOL)
{
	void *sample = Load_Sample(filename);
	if (!sample) return -1;

	int handle = Play_Sample(sample, 0xFF, volume, 0);
	if (handle >= 0 && handle < kMaxSamples) {
		lock_audio();
		Samples[handle].is_score = true;
		Samples[handle].original = 0;
		unlock_audio();
	}

	Free_Sample(sample);
	return handle;
}

void Sound_Callback(void)
{
}

void *Load_Sample(char const *filename)
{
	if (!filename || !Find_File(filename)) return 0;

	int handle = Open_File(filename, READ);
	if (handle == -1) return 0;

	long size = (long)File_Size(handle) + (long)sizeof(AUDHeaderType);
	void *buffer = malloc((size_t)size);
	if (buffer) {
		long loaded = Sample_Read(handle, buffer, size);
		if (loaded <= (long)sizeof(AUDHeaderType)) {
			free(buffer);
			buffer = 0;
		}
	}

	Close_File(handle);
	return buffer;
}

long Load_Sample_Into_Buffer(char const *filename, void *buffer, long size)
{
	if (!filename || !buffer || size <= 0 || !Find_File(filename)) return 0;

	int handle = Open_File(filename, READ);
	if (handle == -1) return 0;

	long loaded = Sample_Read(handle, buffer, size);
	Close_File(handle);
	return loaded;
}

long Sample_Read(int handle, void *buffer, long size)
{
	if (handle == -1 || !buffer || size <= (long)sizeof(AUDHeaderType)) return 0;

	AUDHeaderType header;
	long got = Read_File(handle, &header, sizeof(header));
	if (got != (long)sizeof(header)) return 0;

	long payload_space = size - (long)sizeof(AUDHeaderType);
	long payload_size = header.Size;
	if (payload_size > payload_space) payload_size = payload_space;
	if (payload_size < 0) payload_size = 0;

	memcpy(buffer, &header, sizeof(header));
	long payload_read = Read_File(handle, (unsigned char *)buffer + sizeof(header), (unsigned long)payload_size);
	return (long)sizeof(header) + payload_read;
}

void Free_Sample(void const *sample)
{
	free((void *)sample);
}

BOOL Audio_Init(HWND, int bits_per_sample, BOOL, int rate, int)
{
	if (AudioReady) return TRUE;

	for (int id = 0; id < kMaxSamples; ++id) reset_sample(Samples[id]);
	reset_movie_stream();
	SoundVolume = 255;
	ScoreVolume = 255;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return FALSE;

	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;
	memset(&desired, 0, sizeof(desired));
	memset(&obtained, 0, sizeof(obtained));

	desired.freq = rate > 0 ? rate : 22050;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 1024;
	desired.callback = mix_audio;

	AudioDevice = SDL_OpenAudioDevice(0, 0, &desired, &obtained, 0);
	if (!AudioDevice) return FALSE;

	AudioRate = obtained.freq > 0 ? obtained.freq : desired.freq;
	AudioReady = true;
	SoundType = SFX_ALFX;
	SampleType = SAMPLE_SB;

	SDL_PauseAudioDevice(AudioDevice, 0);
	(void)bits_per_sample;
	return TRUE;
}

void Sound_End(void)
{
	if (AudioDevice) {
		SDL_PauseAudioDevice(AudioDevice, 1);
		SDL_LockAudioDevice(AudioDevice);
		for (int id = 0; id < kMaxSamples; ++id) reset_sample(Samples[id]);
		reset_movie_stream();
		AudioReady = false;
		SDL_UnlockAudioDevice(AudioDevice);
		SDL_CloseAudioDevice(AudioDevice);
		AudioDevice = 0;
	}
	SoundType = SFX_NONE;
	SampleType = SAMPLE_NONE;
}

bool MacAudio_BeginMovieStream(int rate, int channels, int bits)
{
	if (!AudioReady || !AudioDevice || rate <= 0 || (channels != 1 && channels != 2) || (bits != 8 && bits != 16)) {
		return false;
	}

	lock_audio();
	reset_movie_stream();
	MovieAudio.active = true;
	MovieAudio.rate = rate;
	MovieAudio.channels = channels;
	MovieAudio.bits = bits;
	MovieAudio.step = (unsigned long)(((unsigned long long)rate << 16) / (unsigned long long)AudioRate);
	if (MovieAudio.step == 0) MovieAudio.step = 1;
	unlock_audio();
	return true;
}

bool MacAudio_QueueMovieStream(void const *data, size_t bytes)
{
	if (!data || bytes == 0 || !AudioReady || !AudioDevice) {
		return false;
	}

	lock_audio();
	if (!MovieAudio.active) {
		unlock_audio();
		return false;
	}

	unsigned char const *source = (unsigned char const *)data;
	if (MovieAudio.bits == 16) {
		size_t samples = bytes / 2;
		for (size_t index = 0; index < samples; ++index) {
			MovieAudio.pcm.push_back((short)(int16_t)read_le16(source + index * 2));
		}
	} else {
		for (size_t index = 0; index < bytes; ++index) {
			MovieAudio.pcm.push_back((short)(((int)source[index] - 128) << 8));
		}
	}
	unlock_audio();
	return true;
}

void MacAudio_ClearMovieStream(void)
{
	lock_audio();
	MovieAudio.pcm.clear();
	MovieAudio.position = 0;
	unlock_audio();
}

void MacAudio_SetMovieStreamPaused(bool paused)
{
	lock_audio();
	MovieAudio.paused = paused;
	unlock_audio();
}

void MacAudio_EndMovieStream(void)
{
	lock_audio();
	reset_movie_stream();
	unlock_audio();
}

void Stop_Sample(int handle)
{
	if ((unsigned)handle >= kMaxSamples) return;
	lock_audio();
	reset_sample(Samples[handle]);
	unlock_audio();
}

BOOL Sample_Status(int handle)
{
	if ((unsigned)handle >= kMaxSamples) return FALSE;

	lock_audio();
	BOOL active = Samples[handle].active ? TRUE : FALSE;
	unlock_audio();
	return active;
}

BOOL Is_Sample_Playing(void const *sample)
{
	if (!sample) return FALSE;

	lock_audio();
	for (int id = 0; id < kMaxSamples; ++id) {
		if (Samples[id].active && Samples[id].original == sample) {
			unlock_audio();
			return TRUE;
		}
	}
	unlock_audio();
	return FALSE;
}

void Stop_Sample_Playing(void const *sample)
{
	if (!sample) return;

	lock_audio();
	for (int id = 0; id < kMaxSamples; ++id) {
		if (Samples[id].active && Samples[id].original == sample) {
			reset_sample(Samples[id]);
		}
	}
	unlock_audio();
}

int Play_Sample(void const *sample, int priority, int volume, signed short panloc)
{
	return Play_Sample_Handle(sample, priority, volume, panloc, Get_Free_Sample_Handle(priority));
}

int Play_Sample_Handle(void const *sample, int priority, int volume, signed short panloc, int id)
{
	if (!AudioReady || !sample || (unsigned)id >= kMaxSamples) return -1;

	DecodedAudio decoded;
	if (!decode_aud_sample(sample, decoded)) return -1;

	PlayingSample playing;
	reset_sample(playing);
	playing.active = true;
	playing.original = sample;
	playing.priority = priority;
	playing.volume = api_volume_to_255(volume);
	playing.pan = panloc;
	playing.position = 0;
	playing.rate = decoded.rate;
	playing.channels = decoded.channels;
	playing.pcm.swap(decoded.pcm);
	playing.step = (unsigned long)(((unsigned long long)playing.rate << 16) / (unsigned long long)AudioRate);
	if (playing.step == 0) playing.step = 1;

	lock_audio();
	Samples[id] = playing;
	unlock_audio();
	return id;
}

int Set_Sound_Vol(int volume)
{
	lock_audio();
	int old = SoundVolume;
	SoundVolume = api_volume_to_255(volume);
	unlock_audio();
	return old;
}

int Set_Score_Vol(int volume)
{
	lock_audio();
	int old = ScoreVolume;
	ScoreVolume = api_volume_to_255(volume);
	unlock_audio();
	return old;
}

void Fade_Sample(int handle, int)
{
	Stop_Sample(handle);
}

int Get_Free_Sample_Handle(int priority)
{
	lock_audio();
	int handle = free_handle_for_priority(priority);
	unlock_audio();
	return handle;
}

int Get_Digi_Handle(void)
{
	return AudioReady ? 0 : -1;
}

long Sample_Length(void const *sample)
{
	if (!sample) return 0;

	AUDHeaderType header;
	memcpy(&header, sample, sizeof(header));
	long time = header.UncompSize;

	if (header.Flags & AUD_FLAG_16BIT) time >>= 1;
	if (header.Flags & AUD_FLAG_STEREO) time >>= 1;
	if (header.Rate / 60) time /= (header.Rate / 60);

	return time;
}

void Restore_Sound_Buffers(void)
{
}

BOOL Set_Primary_Buffer_Format(void)
{
	return TRUE;
}

BOOL Start_Primary_Sound_Buffer(BOOL)
{
	return AudioReady ? TRUE : FALSE;
}

void Stop_Primary_Sound_Buffer(void)
{
}
