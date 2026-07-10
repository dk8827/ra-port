#ifdef WIN32
#define MACVQA_RESTORE_WIN32 1
#undef WIN32
#endif
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <SDL.h>
#ifdef MACVQA_RESTORE_WIN32
#define WIN32 1
#endif

#include <VQA32/VQAPLAY.H>

#include <mmsystem.h>

#include "../include/mac_audio_stream.h"
#include "../include/mac_sdl_runtime.h"

extern "C" unsigned long __cdecl LCW_Uncompress(void *source, void *dest, unsigned long length);
void Flag_To_Set_Palette(unsigned char *palette, long numbytes, unsigned long slowpal);

struct MacVQAChunk {
	char id[4];
	unsigned long size;
};

struct MacVQAHeader {
	unsigned short Version;
	unsigned short Flags;
	unsigned short Frames;
	unsigned short ImageWidth;
	unsigned short ImageHeight;
	unsigned char BlockWidth;
	unsigned char BlockHeight;
	unsigned char FPS;
	unsigned char Groupsize;
	unsigned short Num1Colors;
	unsigned short CBentries;
	unsigned short Xpos;
	unsigned short Ypos;
	unsigned short MaxFramesize;
	unsigned short SampleRate;
	unsigned char Channels;
	unsigned char BitsPerSample;
	unsigned short AltSampleRate;
	unsigned char AltChannels;
	unsigned char AltBitsPerSample;
};

struct MacVQAStats {
	long start_time;
	long end_time;
	long loaded_frames;
	long drawn_frames;
	long skipped_frames;
	long max_frame_size;
	unsigned long samples_played;
	unsigned long mem_used;
};

struct MacVQAADPCMChannel {
	MacVQAADPCMChannel(void) : predicted(0), index(0), step(7)
	{
	}

	long predicted;
	short index;
	short step;
};

struct MacVQAState {
	MacVQAState(void) : handle(0), handler(0), io_cookie(0), open(false), file_pos(0), form_end(0), palette_size(0), current_frame(0), stop_frame(0), partial_count(0), partial_compressed(false), audio_open(false), audio_rate(0), audio_channels(0), audio_bits(0), next(0)
	{
		memset(&config, 0, sizeof(config));
		memset(&header, 0, sizeof(header));
		memset(&stats, 0, sizeof(stats));
	}

	VQAHandle *handle;
	long (*handler)(VQAHandle *, long, void *, long);
	unsigned long io_cookie;
	bool open;
	unsigned long file_pos;
	unsigned long form_end;
	VQAConfig config;
	MacVQAHeader header;
	std::vector<unsigned long> frame_offsets;
	std::vector<unsigned char> image;
	std::vector<unsigned char> codebook;
	std::vector<unsigned char> partial_codebook;
	std::vector<unsigned char> pointers;
	std::vector<unsigned char> palette;
	long palette_size;
	long current_frame;
	long stop_frame;
	long partial_count;
	bool partial_compressed;
	bool audio_open;
	int audio_rate;
	int audio_channels;
	int audio_bits;
	MacVQAADPCMChannel adpcm[2];
	MacVQAStats stats;
	MacVQAState *next;
};

static MacVQAState *MacVQAStates = 0;
static bool MacVQAAudioPaused = false;

static unsigned short vqa_u16le(unsigned char const *data)
{
	return (unsigned short)(data[0] | (data[1] << 8));
}

static unsigned long vqa_u32le(unsigned char const *data)
{
	return ((unsigned long)data[0]) | ((unsigned long)data[1] << 8) | ((unsigned long)data[2] << 16) | ((unsigned long)data[3] << 24);
}

static unsigned long vqa_u32be(unsigned char const *data)
{
	return ((unsigned long)data[0] << 24) | ((unsigned long)data[1] << 16) | ((unsigned long)data[2] << 8) | (unsigned long)data[3];
}

static unsigned long vqa_pad(unsigned long size)
{
	return (size + 1UL) & ~1UL;
}

static long vqa_clamp(long value, long min_value, long max_value)
{
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static bool vqa_id_is(char const id[4], char const *text)
{
	return id[0] == text[0] && id[1] == text[1] && id[2] == text[2] && id[3] == text[3];
}

static MacVQAState *vqa_find(VQAHandle *handle)
{
	for (MacVQAState *state = MacVQAStates; state; state = state->next) {
		if (state->handle == handle) {
			return state;
		}
	}
	return 0;
}

static void vqa_link(MacVQAState *state)
{
	state->next = MacVQAStates;
	MacVQAStates = state;
}

static void vqa_unlink(MacVQAState *state)
{
	MacVQAState **cursor = &MacVQAStates;
	while (*cursor) {
		if (*cursor == state) {
			*cursor = state->next;
			state->next = 0;
			return;
		}
		cursor = &(*cursor)->next;
	}
}

static bool vqa_call(MacVQAState *state, long action, void *buffer, long nbytes)
{
	if (!state || !state->handler || !state->handle) {
		return false;
	}
	state->handle->VQAio = state->io_cookie;
	long error = state->handler(state->handle, action, buffer, nbytes);
	state->io_cookie = state->handle->VQAio;
	return error == 0;
}

static bool vqa_open_stream(MacVQAState *state, char const *filename)
{
	state->handle->VQAio = 0;
	state->io_cookie = 0;
	if (!vqa_call(state, VQACMD_OPEN, (void *)filename, 0)) {
		state->handle->VQAio = 0;
		state->io_cookie = 0;
		return false;
	}
	state->file_pos = 0;
	return true;
}

static void vqa_close_stream(MacVQAState *state)
{
	if (!state || !state->handler || !state->io_cookie) {
		return;
	}
	vqa_call(state, VQACMD_CLOSE, 0, 0);
	state->io_cookie = 0;
	if (state->handle) {
		state->handle->VQAio = 0;
	}
}

static bool vqa_read(MacVQAState *state, void *buffer, unsigned long bytes)
{
	if (bytes == 0) {
		return true;
	}
	if (!vqa_call(state, VQACMD_READ, buffer, (long)bytes)) {
		return false;
	}
	state->file_pos += bytes;
	return true;
}

static bool vqa_skip(MacVQAState *state, unsigned long bytes)
{
	if (bytes == 0) {
		return true;
	}
	if (!vqa_call(state, VQACMD_SEEK, 0, (long)bytes)) {
		return false;
	}
	state->file_pos += bytes;
	return true;
}

static bool vqa_seek_abs(MacVQAState *state, unsigned long pos)
{
	long delta = (long)pos - (long)state->file_pos;
	if (delta == 0) {
		return true;
	}
	if (!vqa_call(state, VQACMD_SEEK, 0, delta)) {
		return false;
	}
	state->file_pos = pos;
	return true;
}

static bool vqa_read_chunk(MacVQAState *state, MacVQAChunk *chunk)
{
	unsigned char raw[8];
	if (!vqa_read(state, raw, sizeof(raw))) {
		return false;
	}
	memcpy(chunk->id, raw, 4);
	chunk->size = vqa_u32be(raw + 4);
	return true;
}

static bool vqa_read_chunk_data(MacVQAState *state, MacVQAChunk const &chunk, std::vector<unsigned char> &data)
{
	data.resize(chunk.size);
	if (chunk.size != 0 && !vqa_read(state, &data[0], chunk.size)) {
		return false;
	}
	if ((chunk.size & 1) && !vqa_skip(state, 1)) {
		return false;
	}
	return true;
}

static bool vqa_skip_chunk_data(MacVQAState *state, MacVQAChunk const &chunk)
{
	return vqa_skip(state, vqa_pad(chunk.size));
}

static bool vqa_parse_header(MacVQAHeader *header, unsigned char const *data, unsigned long size)
{
	if (!header || !data || size < 42) {
		return false;
	}
	header->Version = vqa_u16le(data + 0);
	header->Flags = vqa_u16le(data + 2);
	header->Frames = vqa_u16le(data + 4);
	header->ImageWidth = vqa_u16le(data + 6);
	header->ImageHeight = vqa_u16le(data + 8);
	header->BlockWidth = data[10];
	header->BlockHeight = data[11];
	header->FPS = data[12];
	header->Groupsize = data[13];
	header->Num1Colors = vqa_u16le(data + 14);
	header->CBentries = vqa_u16le(data + 16);
	header->Xpos = vqa_u16le(data + 18);
	header->Ypos = vqa_u16le(data + 20);
	header->MaxFramesize = vqa_u16le(data + 22);
	header->SampleRate = vqa_u16le(data + 24);
	header->Channels = data[26];
	header->BitsPerSample = data[27];
	header->AltSampleRate = vqa_u16le(data + 28);
	header->AltChannels = data[30];
	header->AltBitsPerSample = data[31];
	return true;
}

static void vqa_reset_adpcm(MacVQAState *state);

static void vqa_reset_runtime(MacVQAState *state)
{
	state->frame_offsets.clear();
	state->image.clear();
	state->codebook.clear();
	state->partial_codebook.clear();
	state->pointers.clear();
	state->palette.clear();
	state->palette_size = 0;
	state->current_frame = 0;
	state->stop_frame = 0;
	state->partial_count = 0;
	state->partial_compressed = false;
	state->audio_rate = 0;
	state->audio_channels = 0;
	state->audio_bits = 0;
	vqa_reset_adpcm(state);
	memset(&state->header, 0, sizeof(state->header));
	memset(&state->stats, 0, sizeof(state->stats));
}

static void vqa_setup_config_from_header(MacVQAState *state)
{
	if (state->config.ImageWidth == -1) {
		state->config.ImageWidth = state->header.ImageWidth;
	}
	if (state->config.ImageHeight == -1) {
		state->config.ImageHeight = state->header.ImageHeight;
	}
	if (state->config.FrameRate == -1) {
		state->config.FrameRate = state->header.FPS;
	}
	if (state->config.DrawRate == -1) {
		state->config.DrawRate = state->header.FPS;
	}
}

static void vqa_reset_adpcm(MacVQAState *state)
{
	if (!state) {
		return;
	}
	for (int channel = 0; channel < 2; ++channel) {
		state->adpcm[channel] = MacVQAADPCMChannel();
	}
}

static bool vqa_audio_requested(MacVQAState *state)
{
	return state && (state->config.OptionFlags & VQAOPTF_AUDIO) && (state->header.Flags & 1);
}

static bool vqa_audio_format(MacVQAState *state, int *rate, int *channels, int *bits)
{
	if (!vqa_audio_requested(state) || !rate || !channels || !bits) {
		return false;
	}

	if (state->header.Version < 2) {
		*rate = 22050;
		*channels = 1;
		*bits = 8;
	} else if ((state->config.OptionFlags & VQAOPTF_ALTAUDIO) && (state->header.Flags & 2)) {
		*rate = state->header.AltSampleRate;
		*channels = state->header.AltChannels;
		*bits = state->header.AltBitsPerSample;
	} else {
		*rate = state->header.SampleRate;
		*channels = state->header.Channels;
		*bits = state->header.BitsPerSample;
	}

	if (*rate <= 0 || (*channels != 1 && *channels != 2) || (*bits != 8 && *bits != 16)) {
		return false;
	}

	if (state->config.AudioRate > 0) {
		*rate = (int)state->config.AudioRate;
	} else if (state->config.FrameRate > 0 && state->header.FPS > 0 && state->config.FrameRate != state->header.FPS) {
		*rate = (int)(((long)*rate * state->config.FrameRate) / state->header.FPS);
		if (*rate <= 0) {
			*rate = state->header.SampleRate;
		}
		state->config.AudioRate = *rate;
	} else {
		state->config.AudioRate = *rate;
	}
	return true;
}

static void vqa_close_audio(MacVQAState *state)
{
	if (!state || !state->audio_open) {
		return;
	}
	MacAudio_EndMovieStream();
	state->audio_open = false;
	state->audio_rate = 0;
	state->audio_channels = 0;
	state->audio_bits = 0;
}

static bool vqa_open_audio(MacVQAState *state)
{
	if (!state) {
		return false;
	}
	if (state->audio_open) {
		return true;
	}

	int rate = 0;
	int channels = 0;
	int bits = 0;
	if (!vqa_audio_format(state, &rate, &channels, &bits)) {
		return false;
	}
	if (!MacAudio_BeginMovieStream(rate, channels, bits)) {
		return false;
	}

	state->audio_open = true;
	state->audio_rate = rate;
	state->audio_channels = channels;
	state->audio_bits = bits;
	vqa_reset_adpcm(state);
	MacAudio_SetMovieStreamPaused(MacVQAAudioPaused);
	return true;
}

static long vqa_adpcm_decode_nibble(MacVQAADPCMChannel *channel, unsigned char code)
{
	static short const index_table[16] = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
	};
	static short const step_table[89] = {
		7, 8, 9, 10, 11, 12, 13, 14,
		16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66,
		73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411,
		1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
		7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
		32767
	};

	long step = channel->step;
	long diff = step >> 3;
	if (code & 4) diff += step;
	if (code & 2) diff += step >> 1;
	if (code & 1) diff += step >> 2;
	if (code & 8) diff = -diff;

	channel->predicted = vqa_clamp(channel->predicted + diff, -32768, 32767);
	channel->index = (short)vqa_clamp(channel->index + index_table[code & 0x0f], 0, 88);
	channel->step = step_table[channel->index];
	return channel->predicted;
}

static bool vqa_decode_snd2(MacVQAState *state, unsigned char const *source, unsigned long source_size, std::vector<unsigned char> &decoded)
{
	int rate = 0;
	int channels = 0;
	int bits = 0;
	if (!vqa_audio_format(state, &rate, &channels, &bits)) {
		return false;
	}

	unsigned long out_size = source_size * (unsigned long)(bits / 4);
	decoded.assign(out_size, 0);
	if (out_size == 0 || !source) {
		return true;
	}

	int bytes_per_sample = bits / 8;
	unsigned long samples_per_channel = out_size / ((unsigned long)bytes_per_sample * (unsigned long)channels);
	for (int channel = 0; channel < channels; ++channel) {
		unsigned long src_index = (unsigned long)channel;
		unsigned char token = 0;
		for (unsigned long sample = 0; sample < samples_per_channel; ++sample) {
			unsigned char code;
			if ((sample & 1UL) == 0) {
				if (src_index >= source_size) {
					return false;
				}
				token = source[src_index];
				src_index += (unsigned long)channels;
				code = token & 0x0f;
			} else {
				code = (token >> 4) & 0x0f;
			}

			long value = vqa_adpcm_decode_nibble(&state->adpcm[channel], code);
			unsigned long out_index = (sample * (unsigned long)channels + (unsigned long)channel) * (unsigned long)bytes_per_sample;
			if (bits == 16) {
				unsigned short sample16 = (unsigned short)((short)value);
				decoded[out_index + 0] = (unsigned char)(sample16 & 0xff);
				decoded[out_index + 1] = (unsigned char)((sample16 >> 8) & 0xff);
			} else {
				decoded[out_index] = (unsigned char)(((value >> 8) & 0xff) ^ 0x80);
			}
		}
	}
	return true;
}

static void vqa_queue_audio(MacVQAState *state, std::vector<unsigned char> const &decoded)
{
	if (!state || !state->audio_open || decoded.empty()) {
		return;
	}
	if (MacAudio_QueueMovieStream(&decoded[0], decoded.size())) {
		state->stats.samples_played += (unsigned long)decoded.size();
	}
}

static bool vqa_load_audio_raw(MacVQAState *state, MacVQAChunk const &chunk)
{
	if (!vqa_audio_requested(state)) {
		return vqa_skip_chunk_data(state, chunk);
	}
	std::vector<unsigned char> data;
	if (!vqa_read_chunk_data(state, chunk, data)) {
		return false;
	}
	if (state->audio_open) {
		vqa_queue_audio(state, data);
	}
	return true;
}

static bool vqa_load_audio_snd2(MacVQAState *state, MacVQAChunk const &chunk)
{
	if (!vqa_audio_requested(state)) {
		return vqa_skip_chunk_data(state, chunk);
	}

	std::vector<unsigned char> data;
	if (!vqa_read_chunk_data(state, chunk, data)) {
		return false;
	}
	if (!state->audio_open) {
		return true;
	}

	std::vector<unsigned char> decoded;
	if (!vqa_decode_snd2(state, data.empty() ? 0 : &data[0], chunk.size, decoded)) {
		return false;
	}
	vqa_queue_audio(state, decoded);
	return true;
}

extern "C" void MacVQA_UnVQ4x2(unsigned char const *codebook, unsigned char const *pointers, unsigned char *buffer, unsigned long blocks_per_row, unsigned long rows, unsigned long buffer_width)
{
	if (!codebook || !pointers || !buffer || blocks_per_row == 0 || rows == 0 || buffer_width == 0) {
		return;
	}

	unsigned long entries = blocks_per_row * rows;
	unsigned long pointer_index = 0;
	for (unsigned long row = 0; row < rows; ++row) {
		unsigned char *row0 = buffer + row * 2UL * buffer_width;
		unsigned char *row1 = row0 + buffer_width;
		for (unsigned long col = 0; col < blocks_per_row; ++col, ++pointer_index) {
			unsigned char low = pointers[pointer_index];
			unsigned char high = pointers[pointer_index + entries];
			unsigned char *dst0 = row0 + col * 4UL;
			unsigned char *dst1 = row1 + col * 4UL;
			if (high == 0x0f) {
				dst0[0] = dst0[1] = dst0[2] = dst0[3] = low;
				dst1[0] = dst1[1] = dst1[2] = dst1[3] = low;
			} else {
				unsigned long codebook_index = (((unsigned long)high << 8) | low) * 8UL;
				memcpy(dst0, codebook + codebook_index, 4);
				memcpy(dst1, codebook + codebook_index + 4, 4);
			}
		}
	}
}

static void vqa_apply_palette(MacVQAState *state, bool force_fallback)
{
	if (!state || state->palette.empty() || state->palette_size <= 0) {
		return;
	}
	if (!force_fallback && state->config.DrawerCallback) {
		Flag_To_Set_Palette(&state->palette[0], state->palette_size, (unsigned long)((state->config.OptionFlags & VQAOPTF_SLOWPAL) != 0));
		return;
	}

	PALETTEENTRY entries[256];
	memset(entries, 0, sizeof(entries));
	long count = state->palette_size / 3;
	if (count > 256) {
		count = 256;
	}
	for (long index = 0; index < count; ++index) {
		entries[index].peRed = (BYTE)(state->palette[index * 3 + 0] << 2);
		entries[index].peGreen = (BYTE)(state->palette[index * 3 + 1] << 2);
		entries[index].peBlue = (BYTE)(state->palette[index * 3 + 2] << 2);
		entries[index].peFlags = 0;
	}
	MacSDL_SetPalette(entries, (int)count);
}

static bool vqa_load_codebook(MacVQAState *state, MacVQAChunk const &chunk, bool compressed, bool partial)
{
	std::vector<unsigned char> data;
	if (!vqa_read_chunk_data(state, chunk, data)) {
		return false;
	}

	if (!partial) {
		state->partial_codebook.clear();
		state->partial_count = 0;
		state->partial_compressed = false;
		if (compressed) {
			if (!data.empty()) {
				LCW_Uncompress(&data[0], &state->codebook[0], (unsigned long)state->codebook.size());
			}
		} else {
			unsigned long count = std::min((unsigned long)data.size(), (unsigned long)state->codebook.size());
			if (count) {
				memcpy(&state->codebook[0], &data[0], count);
			}
		}
		return true;
	}

	if (state->partial_count == 0) {
		state->partial_codebook.clear();
		state->partial_compressed = compressed;
	}
	if (state->partial_compressed != compressed) {
		state->partial_codebook.clear();
		state->partial_count = 0;
		state->partial_compressed = compressed;
	}
	if (!data.empty()) {
		size_t old_size = state->partial_codebook.size();
		state->partial_codebook.resize(old_size + data.size());
		memcpy(&state->partial_codebook[old_size], &data[0], data.size());
	}
	state->partial_count++;

	long group_size = state->header.Groupsize ? state->header.Groupsize : 1;
	if (state->partial_count >= group_size) {
		if (!state->partial_codebook.empty()) {
			if (compressed) {
				LCW_Uncompress(&state->partial_codebook[0], &state->codebook[0], (unsigned long)state->codebook.size());
			} else {
				unsigned long count = std::min((unsigned long)state->partial_codebook.size(), (unsigned long)state->codebook.size());
				memcpy(&state->codebook[0], &state->partial_codebook[0], count);
			}
		}
		state->partial_codebook.clear();
		state->partial_count = 0;
		state->partial_compressed = false;
	}
	return true;
}

static bool vqa_load_palette(MacVQAState *state, MacVQAChunk const &chunk, bool compressed, bool *changed)
{
	std::vector<unsigned char> data;
	if (!vqa_read_chunk_data(state, chunk, data)) {
		return false;
	}
	if (state->palette.empty()) {
		state->palette.resize(768);
	}
	if (compressed) {
		state->palette_size = data.empty() ? 0 : (long)LCW_Uncompress(&data[0], &state->palette[0], (unsigned long)state->palette.size());
	} else {
		state->palette_size = (long)std::min((unsigned long)data.size(), (unsigned long)state->palette.size());
		if (state->palette_size > 0) {
			memcpy(&state->palette[0], &data[0], (size_t)state->palette_size);
		}
	}
	if (changed) {
		*changed = true;
	}
	return true;
}

static bool vqa_load_pointers(MacVQAState *state, MacVQAChunk const &chunk, bool compressed, bool *have_pointers)
{
	std::vector<unsigned char> data;
	if (!vqa_read_chunk_data(state, chunk, data)) {
		return false;
	}
	if (compressed) {
		if (!data.empty()) {
			LCW_Uncompress(&data[0], &state->pointers[0], (unsigned long)state->pointers.size());
		}
	} else {
		unsigned long count = std::min((unsigned long)data.size(), (unsigned long)state->pointers.size());
		if (count) {
			memcpy(&state->pointers[0], &data[0], count);
		}
	}
	if (have_pointers) {
		*have_pointers = true;
	}
	return true;
}

static bool vqa_process_video_chunk(MacVQAState *state, MacVQAChunk const &chunk, bool *have_pointers, bool *palette_changed)
{
	if (vqa_id_is(chunk.id, "CBF0")) return vqa_load_codebook(state, chunk, false, false);
	if (vqa_id_is(chunk.id, "CBFZ")) return vqa_load_codebook(state, chunk, true, false);
	if (vqa_id_is(chunk.id, "CBP0")) return vqa_load_codebook(state, chunk, false, true);
	if (vqa_id_is(chunk.id, "CBPZ")) return vqa_load_codebook(state, chunk, true, true);
	if (vqa_id_is(chunk.id, "CPL0")) return vqa_load_palette(state, chunk, false, palette_changed);
	if (vqa_id_is(chunk.id, "CPLZ")) return vqa_load_palette(state, chunk, true, palette_changed);
	if (vqa_id_is(chunk.id, "VPT0")) return vqa_load_pointers(state, chunk, false, have_pointers);
	if (vqa_id_is(chunk.id, "VPTZ") || vqa_id_is(chunk.id, "VPTD") || vqa_id_is(chunk.id, "VPTK")) {
		return vqa_load_pointers(state, chunk, true, have_pointers);
	}
	if (vqa_id_is(chunk.id, "SND0") || vqa_id_is(chunk.id, "SNA0")) return vqa_load_audio_raw(state, chunk);
	if (vqa_id_is(chunk.id, "SND2") || vqa_id_is(chunk.id, "SNA2")) return vqa_load_audio_snd2(state, chunk);
	return vqa_skip_chunk_data(state, chunk);
}

static bool vqa_process_container(MacVQAState *state, MacVQAChunk const &container, bool *have_pointers, bool *palette_changed)
{
	unsigned long end = state->file_pos + container.size;
	while (state->file_pos + 8 <= end) {
		MacVQAChunk chunk;
		if (!vqa_read_chunk(state, &chunk)) {
			return false;
		}
		if (!vqa_process_video_chunk(state, chunk, have_pointers, palette_changed)) {
			return false;
		}
	}
	if (state->file_pos < end && !vqa_skip(state, end - state->file_pos)) {
		return false;
	}
	if ((container.size & 1) && !vqa_skip(state, 1)) {
		return false;
	}
	return true;
}

static bool vqa_process_frame_range(MacVQAState *state, unsigned long start, unsigned long end, bool *have_pointers, bool *palette_changed)
{
	if (!vqa_seek_abs(state, start)) {
		return false;
	}
	while (state->file_pos + 8 <= end) {
		MacVQAChunk chunk;
		if (!vqa_read_chunk(state, &chunk)) {
			return false;
		}
		if (vqa_id_is(chunk.id, "VQFR") || vqa_id_is(chunk.id, "VQFK")) {
			if (!vqa_process_container(state, chunk, have_pointers, palette_changed)) {
				return false;
			}
		} else {
			if (!vqa_process_video_chunk(state, chunk, have_pointers, palette_changed)) {
				return false;
			}
		}
	}
	if (state->file_pos < end) {
		return vqa_skip(state, end - state->file_pos);
	}
	return true;
}

static void vqa_decode_to_buffer(MacVQAState *state, std::vector<unsigned char> const &frame_codebook)
{
	int target_w = state->config.ImageWidth > 0 ? (int)state->config.ImageWidth : (int)state->header.ImageWidth;
	int target_h = state->config.ImageHeight > 0 ? (int)state->config.ImageHeight : (int)state->header.ImageHeight;
	if (target_w <= 0 || target_h <= 0) {
		return;
	}

	unsigned char *dest = state->config.ImageBuf;
	if (!dest) {
		if ((int)state->image.size() < target_w * target_h) {
			state->image.resize((size_t)target_w * (size_t)target_h);
		}
		dest = &state->image[0];
	}

	int x = 0;
	int y = 0;
	if ((state->config.DrawFlags & VQACFGF_ORIGIN) == 0) {
		x = state->config.X1 >= 0 ? (int)state->config.X1 : (target_w - (int)state->header.ImageWidth) / 2;
		y = state->config.Y1 >= 0 ? (int)state->config.Y1 : (target_h - (int)state->header.ImageHeight) / 2;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	}

	unsigned long blocks_per_row = state->header.ImageWidth / state->header.BlockWidth;
	unsigned long rows = state->header.ImageHeight / state->header.BlockHeight;
	MacVQA_UnVQ4x2(frame_codebook.empty() ? &state->codebook[0] : &frame_codebook[0], &state->pointers[0], dest + y * target_w + x, blocks_per_row, rows, (unsigned long)target_w);
}

static bool vqa_present_or_callback(MacVQAState *state)
{
	unsigned char *image = state->config.ImageBuf ? state->config.ImageBuf : (state->image.empty() ? 0 : &state->image[0]);
	int width = state->config.ImageWidth > 0 ? (int)state->config.ImageWidth : (int)state->header.ImageWidth;
	int height = state->config.ImageHeight > 0 ? (int)state->config.ImageHeight : (int)state->header.ImageHeight;
	if (!image || width <= 0 || height <= 0) {
		return false;
	}

	bool stop = false;
	if (state->config.DrawerCallback) {
		stop = state->config.DrawerCallback(image, state->current_frame) != 0;
	} else {
		MacSDL_SetMode(width, height);
		MacSDL_Present8(image, width, height, width);
	}
	state->stats.drawn_frames++;
	return stop;
}

static void vqa_wait_for_next_frame(MacVQAState *state, DWORD start_time, long next_frame)
{
	long fps = state->config.FrameRate > 0 ? state->config.FrameRate : state->header.FPS;
	if (fps <= 0) {
		fps = 15;
	}
	DWORD target = start_time + (DWORD)((next_frame * 1000L) / fps);
	while (!MacSDL_QuitRequested()) {
		DWORD now = timeGetTime();
		long remaining = (long)target - (long)now;
		if (remaining <= 0) {
			break;
		}
		MacSDL_PumpEvents();
		Sleep((DWORD)(remaining > 5 ? 5 : remaining));
	}
}

void VQA_DefaultConfig(VQAConfig *config)
{
	if (!config) {
		return;
	}
	memset(config, 0, sizeof(*config));
	config->ImageWidth = 320;
	config->ImageHeight = 200;
	config->X1 = -1;
	config->Y1 = -1;
	config->FrameRate = -1;
	config->DrawRate = -1;
	config->TimerMethod = VQA_TMETHOD_DEFAULT;
	config->OptionFlags = VQAOPTF_AUDIO;
	config->NumFrameBufs = 6;
	config->NumCBBufs = 3;
#if (VQADIRECT_SOUND)
	config->SoundObject = 0;
	config->PrimaryBufferPtr = 0;
#endif
	config->AudioBufSize = -1;
	config->AudioRate = -1;
	config->Volume = 0x00FF;
	config->HMIBufSize = 8192L;
	config->DigiHandle = -1;
	config->DigiCard = -1;
	config->DigiPort = -1;
	config->DigiIRQ = -1;
	config->DigiDMA = -1;
}

void VQA_INIConfig(VQAConfig *config)
{
	VQA_DefaultConfig(config);
}

VQAHandle *VQA_Alloc(void)
{
	VQAHandle *handle = new VQAHandle;
	if (!handle) {
		return 0;
	}
	memset(handle, 0, sizeof(*handle));

	MacVQAState *state = new MacVQAState;
	if (!state) {
		delete handle;
		return 0;
	}
	state->handle = handle;
	VQA_DefaultConfig(&state->config);
	vqa_link(state);
	return handle;
}

void VQA_Free(VQAHandle *handle)
{
	if (!handle) {
		return;
	}
	MacVQAState *state = vqa_find(handle);
	if (state) {
		if (state->open) {
			VQA_Close(handle);
		}
		vqa_unlink(state);
		delete state;
	}
	delete handle;
}

void VQA_Reset(VQAHandle *handle)
{
	MacVQAState *state = vqa_find(handle);
	if (state) {
		state->current_frame = 0;
		vqa_reset_adpcm(state);
		if (state->audio_open) {
			MacAudio_ClearMovieStream();
		}
	}
}

void VQA_InitAsDOS(VQAHandle *)
{
}

void VQA_Init(VQAHandle *handle, long (*handler)(VQAHandle *, long, void *, long))
{
	MacVQAState *state = vqa_find(handle);
	if (state) {
		state->handler = handler;
	}
}

unsigned char *VQA_GetPalette(VQAHandle *handle)
{
	MacVQAState *state = vqa_find(handle);
	return state && !state->palette.empty() ? &state->palette[0] : 0;
}

long VQA_GetPaletteSize(VQAHandle *handle)
{
	MacVQAState *state = vqa_find(handle);
	return state ? state->palette_size : 0;
}

void VQA_Set_DrawBuffer(VQAHandle *handle, unsigned char *buffer, unsigned long width, unsigned long height, long xpos, long ypos)
{
	MacVQAState *state = vqa_find(handle);
	if (!state) {
		return;
	}
	state->config.ImageBuf = buffer;
	state->config.ImageWidth = (long)width;
	state->config.ImageHeight = (long)height;
	state->config.X1 = xpos;
	state->config.Y1 = ypos;
}

long VQA_Open(VQAHandle *handle, char const *filename, VQAConfig *config)
{
	MacVQAState *state = vqa_find(handle);
	if (!state || !filename || !state->handler) {
		return VQAERR_OPEN;
	}
	if (state->open) {
		VQA_Close(handle);
	}
	vqa_reset_runtime(state);
	if (config) {
		memcpy(&state->config, config, sizeof(state->config));
	} else {
		VQA_DefaultConfig(&state->config);
	}

	if (!vqa_open_stream(state, filename)) {
		return VQAERR_OPEN;
	}

	unsigned char form[12];
	if (!vqa_read(state, form, sizeof(form)) || memcmp(form, "FORM", 4) != 0 || memcmp(form + 8, "WVQA", 4) != 0) {
		vqa_close_stream(state);
		return VQAERR_NOTVQA;
	}
	state->form_end = vqa_u32be(form + 4) + 8UL;

	bool found_header = false;
	bool found_finf = false;
	while (state->file_pos + 8 <= state->form_end && (!found_header || !found_finf)) {
		MacVQAChunk chunk;
		if (!vqa_read_chunk(state, &chunk)) {
			vqa_close_stream(state);
			return VQAERR_READ;
		}

		std::vector<unsigned char> data;
		if (vqa_id_is(chunk.id, "VQHD")) {
			if (!vqa_read_chunk_data(state, chunk, data) || !vqa_parse_header(&state->header, data.empty() ? 0 : &data[0], chunk.size)) {
				vqa_close_stream(state);
				return VQAERR_NOTVQA;
			}
			found_header = true;
			vqa_setup_config_from_header(state);
		} else if (vqa_id_is(chunk.id, "FINF")) {
			if (!vqa_read_chunk_data(state, chunk, data)) {
				vqa_close_stream(state);
				return VQAERR_READ;
			}
			unsigned long entries = chunk.size / 4;
			state->frame_offsets.resize(entries);
			for (unsigned long index = 0; index < entries; ++index) {
				unsigned long raw = vqa_u32le(&data[index * 4]);
				state->frame_offsets[index] = (raw & 0x0FFFFFFFUL) << 1;
			}
			found_finf = true;
		} else if (!vqa_skip_chunk_data(state, chunk)) {
			vqa_close_stream(state);
			return VQAERR_READ;
		}
	}

	if (!found_header || !found_finf || state->header.BlockWidth != 4 || state->header.BlockHeight != 2 || state->header.ImageWidth == 0 || state->header.ImageHeight == 0) {
		vqa_close_stream(state);
		return VQAERR_NOTVQA;
	}

	unsigned long blocks_per_row = state->header.ImageWidth / state->header.BlockWidth;
	unsigned long rows = state->header.ImageHeight / state->header.BlockHeight;
	unsigned long pointer_size = blocks_per_row * rows * 2UL;
	unsigned long codebook_size = (unsigned long)state->header.CBentries * state->header.BlockWidth * state->header.BlockHeight;
	if (pointer_size == 0 || codebook_size == 0) {
		vqa_close_stream(state);
		return VQAERR_NOTVQA;
	}

	state->image.resize((size_t)state->config.ImageWidth * (size_t)state->config.ImageHeight);
	state->codebook.assign(codebook_size, 0);
	state->pointers.assign(pointer_size, 0);
	state->palette.assign(768, 0);
	state->current_frame = 0;
	state->stop_frame = state->header.Frames;
	if (vqa_audio_requested(state) && !vqa_open_audio(state)) {
		state->config.OptionFlags &= ~VQAOPTF_AUDIO;
	}
	state->open = true;
	state->stats.max_frame_size = state->header.MaxFramesize;
	state->stats.mem_used = (unsigned long)(state->image.size() + state->codebook.size() + state->pointers.size() + state->palette.size());
	return VQAERR_NONE;
}

void VQA_Close(VQAHandle *handle)
{
	MacVQAState *state = vqa_find(handle);
	if (!state) {
		return;
	}
	vqa_close_audio(state);
	vqa_close_stream(state);
	vqa_reset_runtime(state);
	state->open = false;
}

long VQA_Play(VQAHandle *handle, long mode)
{
	MacVQAState *state = vqa_find(handle);
	if (!state || !state->open) {
		return VQAERR_OPEN;
	}
	if (mode == VQAMODE_PAUSE) {
		return VQAERR_PAUSED;
	}
	if (mode == VQAMODE_STOP) {
		state->current_frame = state->stop_frame;
		return VQAERR_EOF;
	}

	DWORD start_time = timeGetTime();
	state->stats.start_time = (long)start_time;
	long end_frame = std::min(state->stop_frame, (long)state->frame_offsets.size());

	do {
		if (state->current_frame >= end_frame || MacSDL_QuitRequested()) {
			state->stats.end_time = (long)timeGetTime();
			return VQAERR_EOF;
		}

		unsigned long frame_start = state->frame_offsets[(size_t)state->current_frame];
		unsigned long frame_end = state->current_frame + 1 < (long)state->frame_offsets.size() ? state->frame_offsets[(size_t)state->current_frame + 1] : state->form_end;
		if (frame_end < frame_start) {
			frame_end = frame_start;
		}

		std::vector<unsigned char> frame_codebook = state->codebook;
		bool have_pointers = false;
		bool palette_changed = false;
		if (!vqa_process_frame_range(state, frame_start, frame_end, &have_pointers, &palette_changed)) {
			state->stats.end_time = (long)timeGetTime();
			return VQAERR_READ;
		}
		state->stats.loaded_frames++;

		if (palette_changed) {
			vqa_apply_palette(state, false);
		}
		if (have_pointers) {
			vqa_decode_to_buffer(state, frame_codebook);
		}
		bool callback_stop = vqa_present_or_callback(state);

		state->current_frame++;
		if (callback_stop) {
			state->stats.end_time = (long)timeGetTime();
			return VQAERR_EOF;
		}
		vqa_wait_for_next_frame(state, start_time, state->current_frame);
	} while (mode == VQAMODE_RUN);

	state->stats.end_time = (long)timeGetTime();
	return VQAERR_NONE;
}

long VQA_SeekFrame(VQAHandle *handle, long frame, long fromwhere)
{
	MacVQAState *state = vqa_find(handle);
	if (!state || !state->open) {
		return VQAERR_OPEN;
	}
	long target = frame;
	if (fromwhere == SEEK_CUR) {
		target = state->current_frame + frame;
	} else if (fromwhere == SEEK_END) {
		target = (long)state->frame_offsets.size() + frame;
	}
	if (target < 0) {
		target = 0;
	}
	if (target > (long)state->frame_offsets.size()) {
		target = (long)state->frame_offsets.size();
	}
	state->current_frame = target;
	vqa_reset_adpcm(state);
	if (state->audio_open) {
		MacAudio_ClearMovieStream();
	}
	return VQAERR_NONE;
}

long VQA_SetStop(VQAHandle *handle, long stop)
{
	MacVQAState *state = vqa_find(handle);
	if (!state) {
		return VQAERR_OPEN;
	}
	if (stop < 0 || stop > (long)state->frame_offsets.size()) {
		stop = (long)state->frame_offsets.size();
	}
	state->stop_frame = stop;
	return VQAERR_NONE;
}

void VQA_PauseAudio(void)
{
	MacVQAAudioPaused = true;
	MacAudio_SetMovieStreamPaused(true);
}

void VQA_ResumeAudio(void)
{
	MacVQAAudioPaused = false;
	MacAudio_SetMovieStreamPaused(false);
}

void VQA_GetInfo(VQAHandle *handle, VQAInfo *info)
{
	if (!info) {
		return;
	}
	memset(info, 0, sizeof(*info));
	MacVQAState *state = vqa_find(handle);
	if (!state) {
		return;
	}
	info->NumFrames = state->header.Frames;
	info->ImageWidth = state->header.ImageWidth;
	info->ImageHeight = state->header.ImageHeight;
	info->ImageBuf = state->config.ImageBuf ? state->config.ImageBuf : (state->image.empty() ? 0 : &state->image[0]);
}

void VQA_GetStats(VQAHandle *handle, VQAStatistics *stats)
{
	if (!stats) {
		return;
	}
	memset(stats, 0, sizeof(*stats));
	MacVQAState *state = vqa_find(handle);
	if (!state) {
		return;
	}
	stats->StartTime = state->stats.start_time;
	stats->EndTime = state->stats.end_time;
	stats->FramesLoaded = state->stats.loaded_frames;
	stats->FramesDrawn = state->stats.drawn_frames;
	stats->FramesSkipped = state->stats.skipped_frames;
	stats->MaxFrameSize = state->stats.max_frame_size;
	stats->SamplesPlayed = state->stats.samples_played;
	stats->MemUsed = state->stats.mem_used;
}

char *VQA_Version(void)
{
	static char version[] = "Mac VQA";
	return version;
}

char *VQA_IDString(void)
{
	static char id[] = "Mac VQA Player";
	return id;
}
