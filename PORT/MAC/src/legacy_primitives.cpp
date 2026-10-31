#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <windows.h>
#include <ddraw.h>
#include <dsound.h>
#include <tile.h>

#include <VQA32/VQAPLAY.H>

#include "../../../CODE/FUNCTION.H"
#include "../../../CODE/MOVIE.H"
#include "../../../CODE/MPGSET.H"
#include <WINCOMM.H>
#include <MODEMREG.H>

static unsigned long long mac_now_msec(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (unsigned long long)tv.tv_sec * 1000ULL + (unsigned long long)tv.tv_usec / 1000ULL;
}

extern "C" unsigned char CurrentPalette[768] = {0};
static unsigned char FontColorXlat[256] = {0};
extern "C" char CPUType = 0;
extern "C" char VendorID[16] = "Unknown";
extern "C" long RandNumb = 0x12349876L;
extern "C" void Set_DD_Palette(void *palette);

extern "C" char *_ShapeBuffer = 0;
extern "C" long _ShapeBufferSize = 0;
extern "C" void *MaskPage = 0;
extern "C" void *BackGroundPage = 0;
extern BOOL UseBigShapeBuffer;
extern bool UseOldShapeDraw;
void *Get_Shape_Header_Data(void *ptr);

#ifndef RA95_NATIVE_SOCKETS
char const *EngMisStr[] = { NULL };
#endif
char ModemRXString[80] = {0};
WinModemClass *SerialPort = NULL;

LPDIRECTSOUND SoundObject = 0;
LPDIRECTSOUNDBUFFER PrimaryBufferPtr = 0;
SFX_Type SoundType = SFX_NONE;
Sample_Type SampleType = SAMPLE_NONE;
int StreamLowImpact = FALSE;
int Misc = 0;

BOOL TimerSystemOn = FALSE;
HANDLE TimerThreadHandle = 0;
int InTimerCallback = 0;
CountDownTimerClass CountDown(BT_SYSTEM, 0);
static unsigned long long MacTimerStartMsec = 0;

WinTimerClass::WinTimerClass(UINT freq, BOOL partial)
{
	Frequency = freq ? freq : 60;
	TrueRate = Frequency;
	UserRate = Frequency;
	SysTicks = 0;
	UserTicks = 0;
	TimerHandle = 1;
	MacTimerStartMsec = mac_now_msec();
	TimerSystemOn = TRUE;
	if (!partial) {
		WindowsTimer = this;
	}
}

WinTimerClass::~WinTimerClass(void)
{
	if (WindowsTimer == this) {
		WindowsTimer = 0;
	}
	TimerHandle = 0;
	TimerSystemOn = FALSE;
}

void WinTimerClass::Update_Tick_Count(void)
{
	unsigned long long elapsed = mac_now_msec() - MacTimerStartMsec;
	unsigned ticks = (unsigned)((elapsed * Frequency) / 1000ULL);
	if (ticks > SysTicks) {
		UserTicks += ticks - SysTicks;
		SysTicks = ticks;
	}
}

unsigned WinTimerClass::Get_System_Tick_Count(void)
{
	Update_Tick_Count();
	return SysTicks;
}

unsigned WinTimerClass::Get_User_Tick_Count(void)
{
	Update_Tick_Count();
	return UserTicks;
}

extern "C" long Get_System_Tick_Count(void)
{
	if (!WindowsTimer) return 0;
	return WindowsTimer->Get_System_Tick_Count();
}

extern "C" long Get_User_Tick_Count(void)
{
	if (!WindowsTimer) return 0;
	return WindowsTimer->Get_User_Tick_Count();
}

extern "C" void Timer_Interrupt_Func(void)
{
	if (WindowsTimer) {
		WindowsTimer->Update_Tick_Count();
	}
}

extern "C" void Disable_Timer_Interrupt(void)
{
}

extern "C" void Enable_Timer_Interrupt(void)
{
}

BOOL __cdecl Init_Timer_System(unsigned int freq, int partial)
{
	if (!WindowsTimer) {
		WindowsTimer = new WinTimerClass(freq, partial);
	} else {
		TimerSystemOn = TRUE;
	}
	return TRUE;
}

BOOL __cdecl Remove_Timer_System(void)
{
	if (WindowsTimer) {
		delete WindowsTimer;
		WindowsTimer = 0;
	}
	TimerSystemOn = FALSE;
	return TRUE;
}

unsigned int WinB = 0;
unsigned int WinC = 1;
unsigned int WinX = 0;
unsigned int WinY = 0;
unsigned int WinCx = 0;
unsigned int WinCy = 0;
unsigned int WinH = 25;
unsigned int WinW = 40;
unsigned int Window = 0;
int WindowColumns = 40;
int WindowLines = 25;
int WindowWidth = 40;
int MoreOn = TRUE;
char *TXT_MoreText = (char *)"--More--";
void (*Window_More_Ptr)(char const *, int, int, int) = 0;

extern "C" const char TXT_WOL_ACCEPT_DRAW[] = "Accept Proposed Draw";
extern "C" const char TXT_WOL_ACCEPT_DRAW_CONFIRM[] = "Are you sure you want to accept a draw?";
extern "C" const char TXT_WOL_AM_MISSIONS[] = "Aftermath Missions";
extern "C" const char TXT_WOL_CS_MISSIONS[] = "Counterstrike Missions";
extern "C" const char TXT_WOL_DRAW[] = "The Game is a Draw";
extern "C" const char TXT_WOL_DRAW_PROPOSED_LOCAL[] = "You have proposed that the game be declared a draw.";
extern "C" const char TXT_WOL_DRAW_PROPOSED_OTHER[] = "%s has proposed that the game be declared a draw.";
extern "C" const char TXT_WOL_DRAW_RETRACTED_LOCAL[] = "You have retracted your offer of a draw.";
extern "C" const char TXT_WOL_DRAW_RETRACTED_OTHER[] = "%s has retracted the offer of a draw.";
extern "C" const char TXT_WOL_PROPOSE_DRAW[] = "Propose Draw";
extern "C" const char TXT_WOL_PROPOSE_DRAW_CONFIRM[] = "Are you sure you want to propose a draw?";
extern "C" const char TXT_WOL_RETRACT_DRAW[] = "Retract Draw";

static int clamp_int(int value, int low, int high)
{
	if (value < low) return low;
	if (value > high) return high;
	return value;
}

static GraphicViewPortClass *as_view(void *ptr)
{
	return (GraphicViewPortClass *)ptr;
}

static GraphicViewPortClass const *as_view_const(void const *ptr)
{
	return (GraphicViewPortClass const *)ptr;
}

static unsigned char const *shape_frame_pixels(void *buffer)
{
	if (!buffer) return 0;
	if (UseBigShapeBuffer && !UseOldShapeDraw) {
		return (unsigned char const *)Get_Shape_Header_Data(buffer);
	}
	return (unsigned char const *)buffer;
}

static unsigned char apply_shape_effects(unsigned char color, unsigned char dest, unsigned char const *ghost_table, unsigned char const *fading_table, int fading_count)
{
	if (ghost_table) {
		unsigned char ghost_index = ghost_table[color];
		if (ghost_index != 0xff) {
			color = ghost_table[256 + ((int)ghost_index * 256) + dest];
		}
	}
	while (fading_table && fading_count-- > 0) {
		color = fading_table[color];
	}
	return color;
}

static unsigned char *view_pixels(GraphicViewPortClass *view)
{
	if (!view) return 0;
	uintptr_t offset = (uintptr_t)view->Get_Offset();
	if (offset == 0) return 0;
	return (unsigned char *)offset;
}

static unsigned char const *view_pixels_const(GraphicViewPortClass const *view)
{
	if (!view) return 0;
	uintptr_t offset = (uintptr_t)((GraphicViewPortClass *)view)->Get_Offset();
	if (offset == 0) return 0;
	return (unsigned char const *)offset;
}

static int view_pitch(GraphicViewPortClass *view)
{
	if (!view) return 0;
	int pitch = view->Get_Pitch();
	if (pitch <= 0) pitch = view->Get_Width() + view->Get_XAdd();
	if (pitch <= 0) pitch = view->Get_Width();
	return pitch;
}

static int view_pitch_const(GraphicViewPortClass const *view)
{
	return view_pitch((GraphicViewPortClass *)view);
}

static unsigned short read_u16_le(void const *ptr)
{
	unsigned char const *bytes = (unsigned char const *)ptr;
	return (unsigned short)(bytes[0] | (bytes[1] << 8));
}

static void put_delta_byte(unsigned char *&out, int &column, int width, int pitch, int do_copy, unsigned char value)
{
	if (do_copy) {
		*out = value;
	} else {
		*out ^= value;
	}
	++out;
	++column;
	if (width > 0 && column >= width) {
		out += pitch - width;
		column = 0;
	}
}

static void skip_delta_bytes(unsigned char *&out, int &column, int width, int pitch, unsigned int count)
{
	if (width <= 0) {
		out += count;
		return;
	}
	while (count--) {
		++out;
		++column;
		if (column >= width) {
			out += pitch - width;
			column = 0;
		}
	}
}

static unsigned int apply_xor_delta(unsigned char *target, unsigned char const *delta, int width, int pitch, int do_copy)
{
	if (!target || !delta) return 0;
	unsigned char *out = target;
	int column = 0;
	unsigned int total = 0;

	for (;;) {
		unsigned int opcode = *delta++;
		if (opcode > 0 && opcode < 0x80) {
			unsigned int count = opcode;
			total += count;
			while (count--) {
				put_delta_byte(out, column, width, pitch, do_copy, *delta++);
			}
			continue;
		}

		if (opcode == 0) {
			unsigned int count = *delta++;
			unsigned char value = *delta++;
			total += count;
			while (count--) {
				put_delta_byte(out, column, width, pitch, do_copy, value);
			}
			continue;
		}

		opcode -= 0x80;
		if (opcode != 0) {
			total += opcode;
			skip_delta_bytes(out, column, width, pitch, opcode);
			continue;
		}

		unsigned int word_count = (unsigned int)delta[0] | ((unsigned int)delta[1] << 8);
		delta += 2;
		if (word_count == 0) break;

		if ((word_count & 0x8000) == 0) {
			total += word_count;
			skip_delta_bytes(out, column, width, pitch, word_count);
			continue;
		}

		word_count -= 0x8000;
		if (word_count & 0x4000) {
			word_count -= 0x4000;
			unsigned char value = *delta++;
			total += word_count;
			while (word_count--) {
				put_delta_byte(out, column, width, pitch, do_copy, value);
			}
			continue;
		}

		total += word_count;
		while (word_count--) {
			put_delta_byte(out, column, width, pitch, do_copy, *delta++);
		}
	}

	return total;
}

extern "C" unsigned int __cdecl Apply_XOR_Delta(char *source_ptr, char *delta_ptr)
{
	return apply_xor_delta((unsigned char *)source_ptr, (unsigned char const *)delta_ptr, 0, 0, FALSE);
}

extern "C" void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy)
{
	apply_xor_delta((unsigned char *)target, (unsigned char const *)delta, width, nextrow, (copy & 0x01) != 0);
}

extern "C" void __cdecl Mem_Copy(void const *source, void *dest, unsigned long bytes_to_copy)
{
	if (!source || !dest || bytes_to_copy == 0) return;
	memmove(dest, source, bytes_to_copy);
}

extern "C" int __cdecl LCW_Comp(void const *source, void *dest, int length)
{
	if (!source || !dest || length <= 0) return 0;
	memmove(dest, source, (size_t)length);
	return length;
}

extern "C" long __cdecl Reverse_Long(long number)
{
	unsigned long value = (unsigned long)number;
	return (long)(((value & 0x000000ffUL) << 24) |
		((value & 0x0000ff00UL) << 8) |
		((value & 0x00ff0000UL) >> 8) |
		((value & 0xff000000UL) >> 24));
}

extern "C" short __cdecl Reverse_Short(short number)
{
	unsigned short value = (unsigned short)number;
	return (short)(((value & 0x00ffU) << 8) | ((value & 0xff00U) >> 8));
}

extern "C" long __cdecl Swap_Long(long number)
{
	return Reverse_Long(number);
}

extern "C" int __cdecl Clip_Rect(int *x, int *y, int *dw, int *dh, int width, int height)
{
	if (!x || !y || !dw || !dh) return -1;
	if (*x < 0) {
		*dw += *x;
		*x = 0;
	}
	if (*y < 0) {
		*dh += *y;
		*y = 0;
	}
	if (*x + *dw > width) *dw = width - *x;
	if (*y + *dh > height) *dh = height - *y;
	return (*dw > 0 && *dh > 0) ? 0 : -1;
}

extern "C" int __cdecl Confine_Rect(int *x, int *y, int dw, int dh, int width, int height)
{
	if (!x || !y) return 0;
	int oldx = *x;
	int oldy = *y;
	*x = clamp_int(*x, 0, width - dw);
	*y = clamp_int(*y, 0, height - dh);
	return (*x != oldx || *y != oldy) ? 1 : 0;
}

extern "C" int __cdecl Get_Random_Mask(int maxval)
{
	if (maxval <= 0) return 0;
	int mask = 1;
	while (mask < maxval) mask = (mask << 1) | 1;
	return mask;
}

extern "C" unsigned char __cdecl Random(void)
{
	RandNumb = RandNumb * 1103515245L + 12345L;
	return (unsigned char)((RandNumb >> 16) & 0xff);
}

extern "C" bool __cdecl Detect_MMX_Availability(void)
{
	return false;
}

extern "C" void __cdecl Force_VM_Page_In(void *, int)
{
}

extern "C" void __cdecl Stop_Execution(void)
{
}

extern "C" void __cdecl Stop_Profiler(void)
{
}

extern "C" void __cdecl Set_Shape_Buffer(void const *buffer, int size)
{
	_ShapeBuffer = (char *)buffer;
	_ShapeBufferSize = size;
}

extern "C" void __cdecl Set_Palette_Range(void *palette)
{
	if (palette) {
		memcpy(CurrentPalette, palette, sizeof(CurrentPalette));
		Set_DD_Palette(CurrentPalette);
	}
}

extern "C" void *__cdecl Get_Font_Palette_Ptr(void)
{
	return FontColorXlat;
}

extern "C" void __cdecl Set_Font_Palette_Range(void const *palette, INT start_idx, INT end_idx)
{
	if (!palette) return;
	if (start_idx < 0) start_idx = 0;
	if (end_idx > 255) end_idx = 255;
	if (end_idx < start_idx) return;
	unsigned char const *source = (unsigned char const *)palette;
	memcpy(&FontColorXlat[start_idx], source, (size_t)(end_idx - start_idx + 1));
}

extern "C" void Set_Palette_Register(int number, int red, int green, int blue)
{
	if (number < 0 || number >= 256) return;
	CurrentPalette[number * 3 + 0] = (unsigned char)red;
	CurrentPalette[number * 3 + 1] = (unsigned char)green;
	CurrentPalette[number * 3 + 2] = (unsigned char)blue;
	Set_DD_Palette(CurrentPalette);
}

void outportb(int, unsigned char)
{
}

void outrgb(unsigned char red, unsigned char green, unsigned char blue)
{
	static int index = 0;
	if (index < 0 || index >= 256) index = 0;
	CurrentPalette[index * 3 + 0] = red;
	CurrentPalette[index * 3 + 1] = green;
	CurrentPalette[index * 3 + 2] = blue;
	++index;
	Set_DD_Palette(CurrentPalette);
}

int ABS(int value)
{
	return value < 0 ? -value : value;
}

long ABS(long value)
{
	return value < 0 ? -value : value;
}

short MIN(short left, short right)
{
	return left < right ? left : right;
}

int MIN(int left, int right)
{
	return left < right ? left : right;
}

long MIN(long left, long right)
{
	return left < right ? left : right;
}

short MAX(short left, short right)
{
	return left > right ? left : right;
}

int MAX(int left, int right)
{
	return left > right ? left : right;
}

long MAX(long left, long right)
{
	return left > right ? left : right;
}

int Bound(int original, int minval, int maxval)
{
	if (minval > maxval) {
		int tmp = minval;
		minval = maxval;
		maxval = tmp;
	}
	return clamp_int(original, minval, maxval);
}

unsigned Bound(unsigned original, unsigned minval, unsigned maxval)
{
	if (minval > maxval) {
		unsigned tmp = minval;
		minval = maxval;
		maxval = tmp;
	}
	if (original < minval) return minval;
	if (original > maxval) return maxval;
	return original;
}

long Bound(long original, long minval, long maxval)
{
	if (minval > maxval) {
		long tmp = minval;
		minval = maxval;
		maxval = tmp;
	}
	if (original < minval) return minval;
	if (original > maxval) return maxval;
	return original;
}

void Set_Bit(void *array, int bit, int value)
{
	if (!array || bit < 0) return;
	unsigned char *bytes = (unsigned char *)array;
	unsigned char mask = (unsigned char)(1U << (bit & 7));
	if (value) {
		bytes[bit >> 3] |= mask;
	} else {
		bytes[bit >> 3] &= (unsigned char)~mask;
	}
}

int Get_Bit(void const *array, int bit)
{
	if (!array || bit < 0) return 0;
	unsigned char const *bytes = (unsigned char const *)array;
	return (bytes[bit >> 3] & (unsigned char)(1U << (bit & 7))) != 0;
}

int First_True_Bit(void const *array)
{
	if (!array) return -1;
	unsigned char const *bytes = (unsigned char const *)array;
	for (int base = 0;; base += 8, ++bytes) {
		unsigned char value = *bytes;
		if (value) {
			for (int bit = 0; bit < 8; ++bit) {
				if (value & (unsigned char)(1U << bit)) return base + bit;
			}
		}
	}
}

int First_False_Bit(void const *array)
{
	if (!array) return -1;
	unsigned char const *bytes = (unsigned char const *)array;
	for (int base = 0;; base += 8, ++bytes) {
		unsigned char value = (unsigned char)~(*bytes);
		if (value) {
			for (int bit = 0; bit < 8; ++bit) {
				if (value & (unsigned char)(1U << bit)) return base + bit;
			}
		}
	}
}

unsigned Fixed_To_Cardinal(unsigned base, unsigned fixed_value)
{
	unsigned long long value = (unsigned long long)base * (unsigned long long)fixed_value + 0x80ULL;
	if (value & 0xff000000ULL) return 0xffffU;
	return (unsigned)(value >> 8);
}

unsigned Cardinal_To_Fixed(unsigned base, unsigned cardinal)
{
	if (base == 0) return 0;
	unsigned long long value = ((unsigned long long)cardinal << 8) / base;
	return (unsigned)value;
}

int calcx(signed short trig, short distance)
{
	return (int)(((long)trig * (long)distance * 2L) >> 8);
}

int calcy(signed short trig, short distance)
{
	return -((int)(((long)trig * (long)distance * 2L) >> 8));
}

unsigned long Get_CPU_Clock(unsigned long &high)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	unsigned long long ticks = (unsigned long long)tv.tv_sec * 1000000ULL + (unsigned long long)tv.tv_usec;
	high = (unsigned long)(ticks >> 32);
	return (unsigned long)(ticks & 0xffffffffUL);
}

extern "C" void __cdecl Buffer_Put_Pixel(void *thisptr, int x, int y, unsigned char color)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels) return;
	if (x < 0 || y < 0 || x >= view->Get_Width() || y >= view->Get_Height()) return;
	pixels[y * view_pitch(view) + x] = color;
}

extern "C" int __cdecl Buffer_Get_Pixel(void *thisptr, int x, int y)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels) return 0;
	if (x < 0 || y < 0 || x >= view->Get_Width() || y >= view->Get_Height()) return 0;
	return pixels[y * view_pitch(view) + x];
}

extern "C" void __cdecl Buffer_Clear(void *thisptr, unsigned char color)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels) return;
	int pitch = view_pitch(view);
	for (int y = 0; y < view->Get_Height(); ++y) {
		memset(pixels + y * pitch, color, (size_t)view->Get_Width());
	}
}

extern "C" long __cdecl Buffer_To_Buffer(void *thisptr, int x, int y, int w, int h, void *buff, long size)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char const *pixels = view_pixels_const(as_view_const(thisptr));
	if (!view || !pixels || !buff || w <= 0 || h <= 0) return 0;
	if (x < 0 || y < 0 || x + w > view->Get_Width() || y + h > view->Get_Height()) return 0;
	if (size < (long)(w * h)) return 0;
	int pitch = view_pitch(view);
	unsigned char *out = (unsigned char *)buff;
	for (int row = 0; row < h; ++row) {
		memcpy(out + row * w, pixels + (y + row) * pitch + x, (size_t)w);
	}
	return (long)(w * h);
}

extern "C" long __cdecl Buffer_To_Page(int x, int y, int w, int h, void *Buffer, void *viewptr)
{
	GraphicViewPortClass *view = as_view(viewptr);
	unsigned char *pixels = view_pixels(view);
	unsigned char const *src = (unsigned char const *)Buffer;
	if (!view || !pixels || !src || w <= 0 || h <= 0) return 0;

	int src_x = 0;
	int src_y = 0;
	int copy_w = w;
	int copy_h = h;
	if (x < 0) {
		src_x = -x;
		copy_w += x;
		x = 0;
	}
	if (y < 0) {
		src_y = -y;
		copy_h += y;
		y = 0;
	}
	if (x + copy_w > view->Get_Width()) copy_w = view->Get_Width() - x;
	if (y + copy_h > view->Get_Height()) copy_h = view->Get_Height() - y;
	if (copy_w <= 0 || copy_h <= 0) return 0;

	int pitch = view_pitch(view);
	for (int row = 0; row < copy_h; ++row) {
		memcpy(pixels + (y + row) * pitch + x, src + (src_y + row) * w + src_x, (size_t)copy_w);
	}
	return (long)(copy_w * copy_h);
}

extern "C" BOOL __cdecl Linear_Blit_To_Linear(void *thisptr, void *destptr, int x_pixel, int y_pixel, int dx_pixel, int dy_pixel, int pixel_width, int pixel_height, BOOL trans)
{
	GraphicViewPortClass *source = as_view(thisptr);
	GraphicViewPortClass *dest = as_view(destptr);
	unsigned char const *src = view_pixels_const(source);
	unsigned char *dst = view_pixels(dest);
	if (!source || !dest || !src || !dst) return FALSE;
	if (pixel_width <= 0 || pixel_height <= 0) return TRUE;

	int sx = x_pixel;
	int sy = y_pixel;
	int dx = dx_pixel;
	int dy = dy_pixel;
	int width = pixel_width;
	int height = pixel_height;

	if (sx < 0) {
		dx -= sx;
		width += sx;
		sx = 0;
	}
	if (sy < 0) {
		dy -= sy;
		height += sy;
		sy = 0;
	}
	if (dx < 0) {
		sx -= dx;
		width += dx;
		dx = 0;
	}
	if (dy < 0) {
		sy -= dy;
		height += dy;
		dy = 0;
	}
	if (sx + width > source->Get_Width()) width = source->Get_Width() - sx;
	if (dx + width > dest->Get_Width()) width = dest->Get_Width() - dx;
	if (sy + height > source->Get_Height()) height = source->Get_Height() - sy;
	if (dy + height > dest->Get_Height()) height = dest->Get_Height() - dy;
	if (width <= 0 || height <= 0) return TRUE;

	int src_pitch = view_pitch_const(source);
	int dst_pitch = view_pitch(dest);
	unsigned char const *src_start = src + sy * src_pitch + sx;
	unsigned char const *src_end = src + (sy + height - 1) * src_pitch + sx + width;
	unsigned char *dst_start = dst + dy * dst_pitch + dx;
	unsigned char *dst_end = dst + (dy + height - 1) * dst_pitch + dx + width;
	uintptr_t src_begin = (uintptr_t)src_start;
	uintptr_t src_limit = (uintptr_t)src_end;
	uintptr_t dst_begin = (uintptr_t)dst_start;
	uintptr_t dst_limit = (uintptr_t)dst_end;
	BOOL overlap = (dst_begin < src_limit && dst_limit > src_begin);

	int row_start = 0;
	int row_end = height;
	int row_step = 1;
	if (overlap && dst_begin > src_begin) {
		row_start = height - 1;
		row_end = -1;
		row_step = -1;
	}

	for (int row = row_start; row != row_end; row += row_step) {
		unsigned char const *src_row = src + (sy + row) * src_pitch + sx;
		unsigned char *dst_row = dst + (dy + row) * dst_pitch + dx;
		if (!trans) {
			memmove(dst_row, src_row, (size_t)width);
			continue;
		}

		uintptr_t src_row_addr = (uintptr_t)src_row;
		uintptr_t dst_row_addr = (uintptr_t)dst_row;
		if (overlap && dst_row_addr > src_row_addr && dst_row_addr < src_row_addr + (uintptr_t)width) {
			for (int col = width - 1; col >= 0; --col) {
				unsigned char value = src_row[col];
				if (value != 0) dst_row[col] = value;
			}
		} else {
			for (int col = 0; col < width; ++col) {
				unsigned char value = src_row[col];
				if (value != 0) dst_row[col] = value;
			}
		}
	}
	return TRUE;
}

extern "C" BOOL __cdecl Linear_Scale_To_Linear(void *thisptr, void *destptr, int src_x, int src_y, int dst_x, int dst_y, int src_w, int src_h, int dst_w, int dst_h, BOOL trans, char *remap)
{
	GraphicViewPortClass *source = as_view(thisptr);
	GraphicViewPortClass *dest = as_view(destptr);
	unsigned char const *src = view_pixels_const(source);
	unsigned char *dst = view_pixels(dest);
	if (!source || !dest || !src || !dst || src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0) return FALSE;

	for (int y = 0; y < dst_h; ++y) {
		int sy = src_y + (y * src_h) / dst_h;
		int dy = dst_y + y;
		if (sy < 0 || dy < 0 || sy >= source->Get_Height() || dy >= dest->Get_Height()) continue;
		for (int x = 0; x < dst_w; ++x) {
			int sx = src_x + (x * src_w) / dst_w;
			int dx = dst_x + x;
			if (sx < 0 || dx < 0 || sx >= source->Get_Width() || dx >= dest->Get_Width()) continue;
			unsigned char value = src[sy * view_pitch_const(source) + sx];
			if (remap) value = (unsigned char)remap[value];
			if (!trans || value != 0) dst[dy * view_pitch(dest) + dx] = value;
		}
	}
	return TRUE;
}

#ifndef FONTINFOBLOCK
#define FONTINFOBLOCK 4
#define FONTOFFSETBLOCK 6
#define FONTWIDTHBLOCK 8
#define FONTDATABLOCK 10
#define FONTHEIGHTBLOCK 12
#define FONTINFOMAXHEIGHT 4
#define FONTINFOMAXWIDTH 5
#endif

extern "C" LONG __cdecl Buffer_Print(void *thisptr, const char *string, int x_pixel, int y_pixel, int fcolor, int bcolor)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels || !string || !FontPtr) return 0;

	char const *font = (char const *)FontPtr;
	char const *info = font + read_u16_le(font + FONTINFOBLOCK);
	unsigned short const *offsets = (unsigned short const *)(font + read_u16_le(font + FONTOFFSETBLOCK));
	unsigned char const *widths = (unsigned char const *)(font + read_u16_le(font + FONTWIDTHBLOCK));
	unsigned char const *heights = (unsigned char const *)(font + read_u16_le(font + FONTHEIGHTBLOCK));
	int maxheight = (unsigned char)info[FONTINFOMAXHEIGHT];
	int pitch = view_pitch(view);
	int original_x = x_pixel;
	int cursor_x = x_pixel;
	int cursor_y = y_pixel;

	unsigned char color_xlat[256];
	memcpy(color_xlat, FontColorXlat, sizeof(color_xlat));
	color_xlat[0] = (unsigned char)bcolor;
	color_xlat[1] = (unsigned char)fcolor;
	color_xlat[16] = (unsigned char)fcolor;

	while (*string) {
		unsigned char ch = (unsigned char)*string++;

		if (ch == '\n' || ch == '\r') {
			cursor_y += maxheight + FontYSpacing;
			if (cursor_y + maxheight > view->Get_Height()) return 0;
			cursor_x = (ch == '\n') ? 0 : original_x;
			continue;
		}

		int width = widths[ch];
		if (cursor_x + width > view->Get_Width()) {
			--string;
			cursor_y += maxheight + FontYSpacing;
			if (cursor_y + maxheight > view->Get_Height()) return 0;
			cursor_x = 0;
			continue;
		}
		if (cursor_y + maxheight > view->Get_Height()) return 0;

		unsigned short offset = read_u16_le(&offsets[ch]);
		unsigned char const *glyph = (unsigned char const *)(font + offset);
		int topblank = heights[ch * 2 + 0];
		int charheight = heights[ch * 2 + 1];
		int bottomblank = maxheight - (topblank + charheight);
		if (bottomblank < 0) bottomblank = 0;

		unsigned char background = color_xlat[0];
		if (background) {
			for (int row = 0; row < topblank; ++row) {
				int yy = cursor_y + row;
				if (yy >= 0 && yy < view->Get_Height()) {
					for (int col = 0; col < width; ++col) {
						int xx = cursor_x + col;
						if (xx >= 0 && xx < view->Get_Width()) pixels[yy * pitch + xx] = background;
					}
				}
			}
		}

		int bytes_per_row = (width + 1) / 2;
		for (int row = 0; row < charheight; ++row) {
			int yy = cursor_y + topblank + row;
			for (int col = 0; col < width; ++col) {
				unsigned char packed = glyph[row * bytes_per_row + (col / 2)];
				unsigned char nibble = (col & 1) ? (unsigned char)(packed >> 4) : (unsigned char)(packed & 0x0F);
				unsigned char color = color_xlat[nibble];
				if (color && yy >= 0 && yy < view->Get_Height()) {
					int xx = cursor_x + col;
					if (xx >= 0 && xx < view->Get_Width()) {
						pixels[yy * pitch + xx] = color;
					}
				}
			}
		}

		if (background && bottomblank) {
			for (int row = 0; row < bottomblank; ++row) {
				int yy = cursor_y + topblank + charheight + row;
				if (yy >= 0 && yy < view->Get_Height()) {
					for (int col = 0; col < width; ++col) {
						int xx = cursor_x + col;
						if (xx >= 0 && xx < view->Get_Width()) pixels[yy * pitch + xx] = background;
					}
				}
			}
		}

		cursor_x += width + FontXSpacing;
	}

	if (cursor_x < 0 || cursor_y < 0 || cursor_x >= view->Get_Width() || cursor_y >= view->Get_Height()) {
		return 0;
	}
	return (LONG)(uintptr_t)(pixels + cursor_y * pitch + cursor_x);
}

extern "C" VOID __cdecl Buffer_Draw_Line(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	int x0 = sx;
	int y0 = sy;
	int x1 = dx;
	int y1 = dy;
	int adx = ABS(x1 - x0);
	int sx_step = x0 < x1 ? 1 : -1;
	int ady = -ABS(y1 - y0);
	int sy_step = y0 < y1 ? 1 : -1;
	int err = adx + ady;

	for (;;) {
		Buffer_Put_Pixel(thisptr, x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		int twice = 2 * err;
		if (twice >= ady) {
			err += ady;
			x0 += sx_step;
		}
		if (twice <= adx) {
			err += adx;
			y0 += sy_step;
		}
	}
}

extern "C" VOID __cdecl Buffer_Fill_Rect(void *thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels) return;
	if (sx > dx) {
		int tmp = sx;
		sx = dx;
		dx = tmp;
	}
	if (sy > dy) {
		int tmp = sy;
		sy = dy;
		dy = tmp;
	}
	sx = clamp_int(sx, 0, view->Get_Width() - 1);
	dx = clamp_int(dx, 0, view->Get_Width() - 1);
	sy = clamp_int(sy, 0, view->Get_Height() - 1);
	dy = clamp_int(dy, 0, view->Get_Height() - 1);
	int pitch = view_pitch(view);
	for (int row = sy; row <= dy; ++row) {
		memset(pixels + row * pitch + sx, color, (size_t)(dx - sx + 1));
	}
}

extern "C" VOID __cdecl Buffer_Remap(void *thisptr, int sx, int sy, int width, int height, void *remap)
{
	GraphicViewPortClass *view = as_view(thisptr);
	unsigned char *pixels = view_pixels(view);
	unsigned char const *table = (unsigned char const *)remap;
	if (!view || !pixels || !table) return;
	int pitch = view_pitch(view);
	for (int y = 0; y < height; ++y) {
		int yy = sy + y;
		if (yy < 0 || yy >= view->Get_Height()) continue;
		for (int x = 0; x < width; ++x) {
			int xx = sx + x;
			if (xx < 0 || xx >= view->Get_Width()) continue;
			unsigned char &value = pixels[yy * pitch + xx];
			value = table[value];
		}
	}
}

static unsigned char const *iconset_bytes(IControl_Type const *iconset, int offset)
{
	if (!iconset || offset <= 0) return 0;
	if (iconset->Size > 0 && offset >= iconset->Size) return 0;
	return ((unsigned char const *)iconset) + offset;
}

static void buffer_draw_stamp_common(void const *thisptr, void const *icondata, int icon, int x_pixel, int y_pixel, void const *remap, int min_x, int min_y, int width, int height, int clipped)
{
	GraphicViewPortClass *view = as_view((void *)thisptr);
	unsigned char *pixels = view_pixels(view);
	if (!view || !pixels || !icondata || icon < 0) return;

	IControl_Type const *iconset = (IControl_Type const *)icondata;
	int icon_width = iconset->Width;
	int icon_height = iconset->Height;
	int icon_count = iconset->Count;
	if (icon_width <= 0 || icon_height <= 0 || icon_count <= 0) return;
	if (icon >= icon_count) return;

	unsigned char const *map = iconset_bytes(iconset, iconset->Map);
	int actual_icon = icon;
	if (map) {
		actual_icon = map[icon];
		if (actual_icon == 0xff || actual_icon >= icon_count) return;
	}

	unsigned char const *icon_pixels = iconset_bytes(iconset, iconset->Icons);
	if (!icon_pixels) return;
	icon_pixels += (size_t)actual_icon * (size_t)icon_width * (size_t)icon_height;

	int clip_left = 0;
	int clip_top = 0;
	int clip_right = view->Get_Width();
	int clip_bottom = view->Get_Height();
	if (clipped) {
		clip_left = min_x;
		clip_top = min_y;
		clip_right = min_x + width;
		clip_bottom = min_y + height;
		x_pixel += min_x;
		y_pixel += min_y;
	}

	if (clip_left < 0) clip_left = 0;
	if (clip_top < 0) clip_top = 0;
	if (clip_right > view->Get_Width()) clip_right = view->Get_Width();
	if (clip_bottom > view->Get_Height()) clip_bottom = view->Get_Height();

	int source_x = 0;
	int source_y = 0;
	int draw_width = icon_width;
	int draw_height = icon_height;

	if (x_pixel < clip_left) {
		source_x = clip_left - x_pixel;
		draw_width -= source_x;
		x_pixel = clip_left;
	}
	if (y_pixel < clip_top) {
		source_y = clip_top - y_pixel;
		draw_height -= source_y;
		y_pixel = clip_top;
	}
	if (x_pixel + draw_width > clip_right) draw_width = clip_right - x_pixel;
	if (y_pixel + draw_height > clip_bottom) draw_height = clip_bottom - y_pixel;
	if (draw_width <= 0 || draw_height <= 0) return;

	unsigned char const *trans_flags = iconset_bytes(iconset, iconset->TransFlag);
	int transparent = remap != 0;
	if (!transparent && trans_flags && actual_icon < icon_count) {
		transparent = trans_flags[actual_icon] != 0;
	}

	unsigned char const *remap_table = (unsigned char const *)remap;
	int pitch = view_pitch(view);
	for (int row = 0; row < draw_height; ++row) {
		unsigned char const *src = icon_pixels + (source_y + row) * icon_width + source_x;
		unsigned char *dst = pixels + (y_pixel + row) * pitch + x_pixel;
		if (!remap_table && !transparent) {
			memcpy(dst, src, (size_t)draw_width);
			continue;
		}
		for (int col = 0; col < draw_width; ++col) {
			unsigned char color = src[col];
			if (remap_table) {
				color = remap_table[color];
			}
			if (!transparent || color != 0) {
				dst[col] = color;
			}
		}
	}
}

extern "C" void __cdecl Buffer_Draw_Stamp(void const *thisptr, void const *icondata, int icon, int x_pixel, int y_pixel, void const *remap)
{
	buffer_draw_stamp_common(thisptr, icondata, icon, x_pixel, y_pixel, remap, 0, 0, 0, 0, FALSE);
}

extern "C" void __cdecl Buffer_Draw_Stamp_Clip(void const *thisptr, void const *icondata, int icon, int x_pixel, int y_pixel, void const *remap, int min_x, int min_y, int width, int height)
{
	buffer_draw_stamp_common(thisptr, icondata, icon, x_pixel, y_pixel, remap, min_x, min_y, width, height, TRUE);
}

extern "C" long __cdecl Buffer_Frame_To_Page(int x, int y, int w, int h, void *Buffer, GraphicViewPortClass &view, int flags, ...)
{
	static const int MAC_SHAPE_TRANS = 0x40;

	unsigned char *pixels = view_pixels(&view);
	unsigned char const *src = shape_frame_pixels(Buffer);
	if (!pixels || !src || w <= 0 || h <= 0) return 0;

	unsigned char const *ghost_table = 0;
	unsigned char const *fading_table = 0;
	int fading_count = 0;
	va_list args;
	va_start(args, flags);
	if (flags & SHAPE_GHOST) {
		ghost_table = (unsigned char const *)va_arg(args, void const *);
	}
	if (flags & SHAPE_FADING) {
		fading_table = (unsigned char const *)va_arg(args, void const *);
		fading_count = va_arg(args, int) & 0x3f;
		if (fading_count == 0) {
			fading_table = 0;
		}
	}
	va_end(args);

	if (flags & SHAPE_CENTER) {
		x -= w / 2;
		y -= h / 2;
	}

	int src_x = 0;
	int src_y = 0;
	int copy_w = w;
	int copy_h = h;
	if (x < 0) {
		src_x = -x;
		copy_w += x;
		x = 0;
	}
	if (y < 0) {
		src_y = -y;
		copy_h += y;
		y = 0;
	}
	if (x + copy_w > view.Get_Width()) copy_w = view.Get_Width() - x;
	if (y + copy_h > view.Get_Height()) copy_h = view.Get_Height() - y;
	if (copy_w <= 0 || copy_h <= 0) return 0;

	int pitch = view_pitch(&view);
	bool transparent = (flags & MAC_SHAPE_TRANS) != 0;
	bool effects = ghost_table || fading_table;
	for (int row = 0; row < copy_h; ++row) {
		unsigned char const *row_src = src + (src_y + row) * w + src_x;
		unsigned char *row_dst = pixels + (y + row) * pitch + x;
		if (!transparent && !effects) {
			memcpy(row_dst, row_src, (size_t)copy_w);
			continue;
		}
		for (int col = 0; col < copy_w; ++col) {
			unsigned char color = row_src[col];
			if (transparent && color == 0) {
				continue;
			}
			if (effects) {
				color = apply_shape_effects(color, row_dst[col], ghost_table, fading_table, fading_count);
			}
			row_dst[col] = color;
		}
	}
	return (long)(copy_w * copy_h);
}

extern "C" void Cache_Copy_Icon(void const *, void *, int)
{
}

extern "C" int Is_Icon_Cached(void const *, int)
{
	return -1;
}

extern "C" void __cdecl ModeX_Blit(GraphicBufferClass *)
{
}

static void mac_expand_interpolated_line(unsigned char const *src, unsigned char *dst, int width)
{
	if (!src || !dst || width <= 0) return;
	for (int x = 0; x < width; ++x) {
		unsigned char value = src[x];
		dst[x * 2] = value;
		dst[x * 2 + 1] = (x + 1 < width) ? PaletteInterpolationTable[value][src[x + 1]] : value;
	}
}

extern "C" void __cdecl Asm_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	if (!src_ptr || !dest_ptr) return;
	int row_pitch = dest_width / 2;
	if (lines <= 0 || src_width <= 0 || row_pitch <= 0) return;
	for (int y = 0; y < lines; ++y) {
		unsigned char const *src = src_ptr + y * src_width;
		unsigned char *dst0 = dest_ptr + y * dest_width;
		unsigned char *dst1 = dst0 + row_pitch;
		mac_expand_interpolated_line(src, dst0, src_width);
		mac_expand_interpolated_line(src, dst1, src_width);
	}
}

extern "C" void __cdecl Asm_Interpolate_Line_Double(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	Asm_Interpolate(src_ptr, dest_ptr, lines, src_width, dest_width);
}

extern "C" void __cdecl Asm_Interpolate_Line_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	Asm_Interpolate(src_ptr, dest_ptr, lines, src_width, dest_width);
}

struct MacMouseLayout {
	char *MouseCursor;
	int MouseXHot;
	int MouseYHot;
	int CursorWidth;
	int CursorHeight;
	char *MouseBuffer;
	int MouseBuffX;
	int MouseBuffY;
	int MaxWidth;
	int MaxHeight;
	int MouseCXLeft;
	int MouseCYUpper;
	int MouseCXRight;
	int MouseCYLower;
	char MCFlags;
	char MCCount;
	GraphicViewPortClass *Screen;
	char *PrevCursor;
	int MouseUpdate;
	int State;
	char *EraseBuffer;
	int EraseBuffX;
	int EraseBuffY;
	int EraseBuffHotX;
	int EraseBuffHotY;
	int EraseFlags;
	CRITICAL_SECTION MouseCriticalSection;
	unsigned TimerHandle;
};

static MacMouseLayout *mouse_layout(void *thisptr)
{
	return (MacMouseLayout *)thisptr;
}

static int shape_header_size(void const *cursor)
{
	if (!cursor) return 0;
	unsigned short type = read_u16_le(cursor);
	return (type & MAKESHAPE_COMPACT) ? 26 : 10;
}

static bool unpack_cursor_shape(void const *cursor, unsigned char *dest, int width, int height)
{
	if (!cursor || !dest || width <= 0 || height <= 0) return false;

	Shape_Type const *shape = (Shape_Type const *)cursor;
	unsigned short type = shape->ShapeType;
	int header_size = shape_header_size(cursor);
	unsigned char const *shape_data = (unsigned char const *)cursor;
	unsigned char *temporary = 0;

	if ((type & MAKESHAPE_NOCOMP) == 0) {
		if (!_ShapeBuffer || _ShapeBufferSize <= shape->DataLength + header_size) return false;
		memcpy(_ShapeBuffer, cursor, (size_t)header_size);
		((Shape_Type *)_ShapeBuffer)->ShapeType = type | MAKESHAPE_NOCOMP;
		LCW_Uncompress((void *)(shape_data + header_size), _ShapeBuffer + header_size, shape->DataLength);
		shape_data = (unsigned char *)_ShapeBuffer;
		type |= MAKESHAPE_NOCOMP;
		temporary = (unsigned char *)_ShapeBuffer;
		(void)temporary;
	}

	unsigned char const *src = shape_data + header_size;
	unsigned char const *remap = 0;
	if (type & MAKESHAPE_COMPACT) {
		remap = shape_data + 10;
		src = shape_data + 26;
	}

	int pixels = width * height;
	int out = 0;
	while (out < pixels) {
		unsigned char value = *src++;
		if (value == 0) {
			int count = *src++;
			while (count-- > 0 && out < pixels) {
				dest[out++] = 0;
			}
		} else {
			dest[out++] = remap ? remap[value] : value;
		}
	}
	return true;
}

extern "C" void __cdecl Mouse_Shadow_Buffer(void *thisptr, GraphicViewPortClass *srcdst, void *buffer, int x, int y, int hotx, int hoty, int store)
{
	MacMouseLayout *mouse = mouse_layout(thisptr);
	unsigned char *pixels = view_pixels(srcdst);
	unsigned char *shadow = (unsigned char *)buffer;
	if (!mouse || !pixels || !shadow || mouse->CursorWidth <= 0 || mouse->CursorHeight <= 0) return;

	int x0 = x - hotx;
	int y0 = y - hoty;
	int src_x = 0;
	int src_y = 0;
	int copy_w = mouse->CursorWidth;
	int copy_h = mouse->CursorHeight;

	if (x0 < 0) {
		src_x = -x0;
		copy_w += x0;
		x0 = 0;
	}
	if (y0 < 0) {
		src_y = -y0;
		copy_h += y0;
		y0 = 0;
	}
	if (x0 + copy_w > srcdst->Get_Width()) copy_w = srcdst->Get_Width() - x0;
	if (y0 + copy_h > srcdst->Get_Height()) copy_h = srcdst->Get_Height() - y0;
	if (copy_w <= 0 || copy_h <= 0) return;

	int pitch = view_pitch(srcdst);
	for (int row = 0; row < copy_h; ++row) {
		unsigned char *page = pixels + (y0 + row) * pitch + x0;
		unsigned char *saved = shadow + (src_y + row) * mouse->CursorWidth + src_x;
		if (store == 1) {
			memcpy(saved, page, (size_t)copy_w);
		} else {
			memcpy(page, saved, (size_t)copy_w);
		}
	}
}

extern "C" void __cdecl Draw_Mouse(void *thisptr, GraphicViewPortClass *srcdst, int x, int y)
{
	MacMouseLayout *mouse = mouse_layout(thisptr);
	unsigned char *pixels = view_pixels(srcdst);
	if (!mouse || !pixels || !mouse->MouseCursor || mouse->CursorWidth <= 0 || mouse->CursorHeight <= 0) return;

	int x0 = x - mouse->MouseXHot;
	int y0 = y - mouse->MouseYHot;
	int src_x = 0;
	int src_y = 0;
	int copy_w = mouse->CursorWidth;
	int copy_h = mouse->CursorHeight;

	if (x0 < 0) {
		src_x = -x0;
		copy_w += x0;
		x0 = 0;
	}
	if (y0 < 0) {
		src_y = -y0;
		copy_h += y0;
		y0 = 0;
	}
	if (x0 + copy_w > srcdst->Get_Width()) copy_w = srcdst->Get_Width() - x0;
	if (y0 + copy_h > srcdst->Get_Height()) copy_h = srcdst->Get_Height() - y0;
	if (copy_w <= 0 || copy_h <= 0) return;

	int pitch = view_pitch(srcdst);
	for (int row = 0; row < copy_h; ++row) {
		unsigned char const *src = (unsigned char const *)mouse->MouseCursor + (src_y + row) * mouse->CursorWidth + src_x;
		unsigned char *dst = pixels + (y0 + row) * pitch + x0;
		for (int col = 0; col < copy_w; ++col) {
			if (src[col] != 0) dst[col] = src[col];
		}
	}
}

extern "C" void *__cdecl ASM_Set_Mouse_Cursor(void *thisptr, int hotspotx, int hotspoty, void *cursor)
{
	MacMouseLayout *mouse = mouse_layout(thisptr);
	if (!mouse || !cursor) return cursor;

	char *previous = mouse->PrevCursor;
	mouse->PrevCursor = (char *)cursor;

	int width = Get_Shape_Width(cursor);
	int height = Get_Shape_Original_Height(cursor);
	if (width <= 0 || height <= 0 || width > mouse->MaxWidth || height > mouse->MaxHeight) {
		return previous;
	}

	if (!unpack_cursor_shape(cursor, (unsigned char *)mouse->MouseCursor, width, height)) {
		return previous;
	}

	mouse->MouseXHot = hotspotx;
	mouse->MouseYHot = hotspoty;
	mouse->CursorWidth = width;
	mouse->CursorHeight = height;
	return previous;
}

void Window_Hide_Mouse(int)
{
}

void Window_Show_Mouse(void)
{
}

int Change_Window(int windnum)
{
	int oldwindow = (int)Window;
	Window = (unsigned int)windnum;
	int *data = &WindowList[windnum][0];
	WinX = (unsigned int)*data++;
	WinY = (unsigned int)*data++;
	WinW = (unsigned int)*data++;
	WinH = (unsigned int)*data++;
	WinC = (unsigned int)*data++;
	WinB = (unsigned int)*data++;
	WinCx = (unsigned int)*data++;
	WinCy = (unsigned int)*data++;
	WindowWidth = (int)WinW;
	WindowColumns = (int)(WinW / 8);
	WindowLines = (int)(WinH / 8);
	return oldwindow;
}

int Change_New_Window(int windnum)
{
	int oldwindow = Change_Window(windnum);
	New_Window();
	return oldwindow;
}

void New_Window(void)
{
	if (LogicPage) {
		LogicPage->Fill_Rect((int)WinX, (int)WinY, (int)(WinX + WinW - 1), (int)(WinY + WinH - 1), (unsigned char)WinB);
	}
}

void Window_Int_Print(int)
{
}

void Window_Print(char const [], ...)
{
}

void Standard_More_Prompt(char const *, int, int, int)
{
}

void Set_More_Prompt(char const *prompt, int, int, int)
{
	TXT_MoreText = (char *)(prompt ? prompt : "");
}

void Set_More_On(void)
{
	MoreOn = TRUE;
}

void Set_More_Off(void)
{
	MoreOn = FALSE;
}

char *Extract_String(void const *data, int string)
{
	if (!data || string < 0) return 0;
	unsigned short const *offsets = (unsigned short const *)data;
	return ((char *)data) + offsets[string];
}

#ifndef RA95_NATIVE_SOCKETS
unsigned long Compute_Name_CRC(char *name)
{
	if (!name) return 0;
	CRCEngine crc;
	for (char *ptr = name; *ptr; ++ptr) {
		char value = (char)toupper((unsigned char)*ptr);
		crc(&value, 1);
	}
	return (unsigned long)(long)crc;
}

bool Init_Network(void)
{
	return false;
}

void Shutdown_Network(void)
{
}

bool Remote_Connect(void)
{
	return false;
}

void Destroy_Connection(int, int)
{
}

bool Process_Global_Packet(GlobalPacketType *, IPXAddressClass *)
{
	return false;
}

void Net_Reconnect_Dialog(int, int, int, unsigned long)
{
}
#endif

bool bSpecialAftermathScenario(char const *)
{
	return false;
}

WinModemClass::WinModemClass(void) :
	FramingErrors(0),
	IOErrors(0),
	BufferOverruns(0),
	InBufferOverflows(0),
	ParityErrors(0),
	OutBufferOverflows(0),
	InQueue(0),
	OutQueue(0),
	SerialBuffer(0),
	WaitingForSerialCharRead(FALSE),
	WaitingForSerialCharWrite(FALSE),
	SerialBufferReadPtr(0),
	SerialBufferWritePtr(0),
	PortHandle(0),
	DialingMethod(WC_TOUCH_TONE),
	EchoFunction(0),
	AbortFunction(0)
{
	memset(&ReadOverlap, 0, sizeof(ReadOverlap));
	memset(&WriteOverlap, 0, sizeof(WriteOverlap));
	memset(TempSerialBuffer, 0, sizeof(TempSerialBuffer));
}

WinModemClass::~WinModemClass(void)
{
}

HANDLE WinModemClass::Serial_Port_Open(char *, int, int, int, int, int)
{
	PortHandle = 0;
	return 0;
}

void WinModemClass::Serial_Port_Close(void)
{
	PortHandle = 0;
}

int WinModemClass::Read_From_Serial_Port(unsigned char *, int)
{
	return 0;
}

void WinModemClass::Write_To_Serial_Port(unsigned char *, int)
{
}

void WinModemClass::Wait_For_Serial_Write(void)
{
}

void WinModemClass::Set_Modem_Dial_Type(WinCommDialMethodType method)
{
	DialingMethod = method;
}

unsigned WinModemClass::Get_Modem_Status(void)
{
	return 0;
}

void WinModemClass::Set_Serial_DTR(BOOL)
{
}

int WinModemClass::Get_Modem_Result(int, char *buffer, int buffer_len)
{
	if (buffer && buffer_len > 0) {
		buffer[0] = '\0';
	}
	return MODEM_CMD_ERROR;
}

void WinModemClass::Dial_Modem(char *)
{
}

int WinModemClass::Send_Command_To_Modem(char *, char, char *buffer, int buffer_len, int, int)
{
	if (buffer && buffer_len > 0) {
		buffer[0] = '\0';
	}
	return MODEM_CMD_ERROR;
}

void WinModemClass::Set_Echo_Function(void (*func)(char))
{
	EchoFunction = func;
}

void WinModemClass::Set_Abort_Function(int (*func)(void))
{
	AbortFunction = func;
}

HANDLE WinModemClass::Get_Port_Handle(void)
{
	return PortHandle;
}

BOOL WinModemClass::Read_Serial_Chars(void)
{
	return FALSE;
}

ModemRegistryEntryClass::ModemRegistryEntryClass(int) :
	ModemName(0),
	ModemDeviceName(0),
	ErrorCorrectionEnable(0),
	ErrorCorrectionDisable(0),
	CompressionEnable(0),
	CompressionDisable(0),
	HardwareFlowControl(0),
	NoFlowControl(0)
{
}

ModemRegistryEntryClass::~ModemRegistryEntryClass(void)
{
}

HKEY Get_Registry_Sub_Key(HKEY, char *, BOOL)
{
	return 0;
}

extern "C" void __stdcall MpgPlay(const char *, IDirectDraw *, IDirectDrawSurface *, RECT *)
{
}

extern "C" void __stdcall MpgPause(void)
{
}

extern "C" void __stdcall MpgResume(void)
{
}

extern "C" void __stdcall MpgSetCallback(LPMPGCALLBACK, LPVOID)
{
}

MPGSettings::MPGSettings(const char *deviceName) : mDeviceName(0)
{
	SetDeviceName(deviceName);
}

MPGSettings::MPGSettings(FileClass &) : mDeviceName(0)
{
	SetDeviceName(0);
}

MPGSettings::~MPGSettings(void)
{
	delete[] mDeviceName;
}

void MPGSettings::SetDeviceName(const char *deviceName)
{
	delete[] mDeviceName;
	mDeviceName = 0;
	if (!deviceName) return;
	size_t length = strlen(deviceName) + 1;
	mDeviceName = new char[length];
	memcpy(mDeviceName, deviceName, length);
}

bool MPGSettings::Save(FileClass &)
{
	return true;
}

void MPGSettings::Dialog(void)
{
}

void (*NullModemClass::OrigAbortModemFunc)(int) = 0;
KeyNumType NullModemClass::Input = KN_NONE;
GadgetClass *NullModemClass::Commands = 0;

NullModemClass::NullModemClass(int numsend, int numreceive, int maxlen, unsigned short magicnum) :
	BuildBuf(0),
	MaxLen(maxlen),
	EchoBuf(0),
	EchoSize(0),
	EchoCount(0),
	OldIRQPri(0),
	ModemVerboseOn(0),
	ModemEchoOn(0),
	ModemWaitCarrier(0),
	ModemCarrierDetect(0),
	ModemCarrierLoss(0),
	ModemHangupDelay(0),
	ModemGuardTime(0),
	ModemEscapeCode('+'),
	Connection(0),
	NumConnections(0),
	PortHandle(0),
	NumSend(numsend),
	NumReceive(numreceive),
	MagicNum(magicnum),
	RXBuf(0),
	RXSize(0),
	RXCount(0),
	RetryDelta(0),
	MaxRetries(0),
	Timeout(0),
	SendOverflows(0),
	ReceiveOverflows(0),
	CRCErrors(0)
{
}

NullModemClass::~NullModemClass()
{
}

int NullModemClass::Init(int, int, char *, int, char, int, int, int)
{
	return 0;
}

int NullModemClass::Delete_Connection(void)
{
	NumConnections = 0;
	return 0;
}

int NullModemClass::Num_Connections(void)
{
	return NumConnections;
}

int NullModemClass::Init_Send_Queue(void)
{
	return 0;
}

void NullModemClass::Shutdown(void)
{
	NumConnections = 0;
}

void NullModemClass::Set_Timing(unsigned long retrydelta, unsigned long maxretries, unsigned long timeout)
{
	RetryDelta = retrydelta;
	MaxRetries = maxretries;
	Timeout = timeout;
}

int NullModemClass::Send_Message(void *, int, int)
{
	return 0;
}

int NullModemClass::Get_Message(void *, int *)
{
	return 0;
}

int NullModemClass::Service(void)
{
	return 0;
}

int NullModemClass::Num_Send(void)
{
	return 0;
}

int NullModemClass::Num_Receive(void)
{
	return 0;
}

unsigned long NullModemClass::Response_Time(void)
{
	return 0;
}

void NullModemClass::Reset_Response_Time(void)
{
}

void *NullModemClass::Oldest_Send(void)
{
	return 0;
}

void NullModemClass::Configure_Debug(int, int, int, char **, int, int)
{
}

DetectPortType NullModemClass::Detect_Port(SerialSettingsType *)
{
	return PORT_INVALID;
}

int NullModemClass::Detect_Modem(SerialSettingsType *, int)
{
	return 0;
}

DialStatusType NullModemClass::Dial_Modem(char *, DialMethodType, int)
{
	return DIAL_ERROR;
}

DialStatusType NullModemClass::Answer_Modem(int)
{
	return DIAL_ERROR;
}

int NullModemClass::Hangup_Modem(void)
{
	return 0;
}

void NullModemClass::Setup_Modem_Echo(void (*)(char))
{
}

void NullModemClass::Remove_Modem_Echo(void)
{
}

void NullModemClass::Print_EchoBuf(void)
{
}

void NullModemClass::Reset_EchoBuf(void)
{
	EchoCount = 0;
}

int NullModemClass::Abort_Modem(PORT *)
{
	return 0;
}

void NullModemClass::Setup_Abort_Modem(void)
{
}

void NullModemClass::Remove_Abort_Modem(void)
{
}

int NullModemClass::Change_IRQ_Priority(int)
{
	return 0;
}

int NullModemClass::Get_Modem_Status(void)
{
	return 0;
}

int NullModemClass::Send_Modem_Command(char *, char, char *, int, int, int)
{
	return MODEM_CMD_ERROR;
}

int NullModemClass::Verify_And_Convert_To_Int(char *)
{
	return 0;
}
