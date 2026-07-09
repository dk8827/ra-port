#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../include/mac_sdl_runtime.h"
#include "mobile_key_message.h"
#include "mobile_pan.h"
#include "mobile_touch_gesture.h"
#include "ra_aspect_viewport.h"
#include <mmsystem.h>

static SDL_Window *MacWindow = 0;
static SDL_Renderer *MacRenderer = 0;
static SDL_Texture *MacTexture = 0;
static uint32_t *MacFrame = 0;
static int MacFramePixels = 0;
static int MacWidth = 0;
static int MacHeight = 0;
static bool MacSDLReady = false;
static bool MacQuitRequested = false;
static bool MacFullscreen = false;
static SDL_threadID MacMainThread = 0;
static uint32_t MacPalette[256];
static MSG MacMessageQueue[512];
static int MacMessageHead = 0;
static int MacMessageTail = 0;
static unsigned char MacKeyState[256];
static unsigned char MacToggleState[256];
static POINT MacMousePoint = {0, 0};
static bool MacUnmodifiedKeyDispatch = false;

#if defined(RA_MOBILE_TOUCH)
static MobileTouchGesture MobileTouch;
static MobilePanState MobilePan;
static bool MobileTouchCursorHidden = true;
static bool MobilePointerDragCandidate = false;
static int MobilePointerDragStartX = 0;
static int MobilePointerDragStartY = 0;
#endif

LRESULT FAR PASCAL _export Windows_Procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static uint32_t mac_argb(unsigned char red, unsigned char green, unsigned char blue)
{
	return 0xFF000000U | ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
}

static void mac_default_palette(void)
{
	for (int index = 0; index < 256; ++index) {
		MacPalette[index] = mac_argb((unsigned char)index, (unsigned char)index, (unsigned char)index);
	}
}

static DWORD mac_now_ms(void)
{
	return (DWORD)SDL_GetTicks();
}

static bool mac_fullscreen_env_requested(void)
{
	char const *value = getenv("RA_FULLSCREEN");
	if (!value || !value[0]) {
		return false;
	}
	return strcmp(value, "0") != 0 && strcmp(value, "false") != 0 && strcmp(value, "FALSE") != 0 &&
		strcmp(value, "no") != 0 && strcmp(value, "NO") != 0;
}

static LPARAM mac_pack_xy(int x, int y)
{
	return (LPARAM)(((y & 0xFFFF) << 16) | (x & 0xFFFF));
}

static RAAspectViewport mac_window_viewport(void)
{
	int window_w = MacWidth;
	int window_h = MacHeight;
	if (MacWindow) {
		SDL_GetWindowSize(MacWindow, &window_w, &window_h);
	}
	return RA_CalculateAspectViewport(MacWidth, MacHeight, window_w, window_h);
}

static RAAspectViewport mac_renderer_viewport(int source_w, int source_h)
{
	int output_w = source_w;
	int output_h = source_h;
	if (MacRenderer) {
		SDL_GetRendererOutputSize(MacRenderer, &output_w, &output_h);
	}
	return RA_CalculateAspectViewport(source_w, source_h, output_w, output_h);
}

static bool mac_to_logical_point(int *x, int *y)
{
	if (!x || !y || !MacWindow || MacWidth <= 0 || MacHeight <= 0) {
		return false;
	}
	RAAspectViewport viewport = mac_window_viewport();
	return RA_MapViewportPoint(viewport, MacWidth, MacHeight, *x, *y, x, y) != 0;
}

static BOOL mac_queue_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	int next = (MacMessageTail + 1) % (int)(sizeof(MacMessageQueue) / sizeof(MacMessageQueue[0]));
	if (next == MacMessageHead) {
		return FALSE;
	}
	MSG *msg = &MacMessageQueue[MacMessageTail];
	memset(msg, 0, sizeof(*msg));
	msg->hwnd = hwnd;
	msg->message = message;
	msg->wParam = wparam;
	msg->lParam = lparam;
	msg->time = mac_now_ms();
	msg->pt = MacMousePoint;
	MacMessageTail = next;
	return TRUE;
}

static void mac_queue_mouse_motion(int x, int y)
{
#if defined(RA_MOBILE_TOUCH)
	MobileTouchCursorHidden = false;
#endif
	MacMousePoint.x = x;
	MacMousePoint.y = y;
	mac_queue_message((HWND)(intptr_t)1, WM_MOUSEMOVE, 0, mac_pack_xy(x, y));
}

static void mac_queue_mouse_button_with_cursor(int vk, bool down, int x, int y, bool update_cursor)
{
	MacMousePoint.x = x;
	MacMousePoint.y = y;
#if defined(RA_MOBILE_TOUCH)
	if (vk == VK_LBUTTON && down) {
		MobilePointerDragCandidate = true;
		MobilePointerDragStartX = x;
		MobilePointerDragStartY = y;
	} else if (vk != VK_LBUTTON && down) {
		MobilePointerDragCandidate = false;
	}
#endif
	if (update_cursor) {
#if defined(RA_MOBILE_TOUCH)
		MobileTouchCursorHidden = false;
#endif
	}
	MacKeyState[vk & 0xFF] = down ? 1 : 0;
	UINT message = down ? WM_LBUTTONDOWN : WM_LBUTTONUP;
	if (vk == VK_RBUTTON) {
		message = down ? WM_RBUTTONDOWN : WM_RBUTTONUP;
	} else if (vk == VK_MBUTTON) {
		message = down ? WM_MBUTTONDOWN : WM_MBUTTONUP;
	}
	mac_queue_message((HWND)(intptr_t)1, message, (WPARAM)vk, mac_pack_xy(x, y));
#if defined(RA_MOBILE_TOUCH)
	if (!update_cursor && !down) {
		MobileTouchCursorHidden = true;
	}
#endif
}

static void mac_queue_mouse_button(int vk, bool down, int x, int y)
{
	mac_queue_mouse_button_with_cursor(vk, down, x, y, true);
}

static void mac_queue_key_pulse(int vk)
{
	MacKeyState[vk & 0xFF] = 1;
	mac_queue_message((HWND)(intptr_t)1, WM_KEYDOWN, (WPARAM)vk, 0);
	MacKeyState[vk & 0xFF] = 0;
	mac_queue_message((HWND)(intptr_t)1, WM_KEYUP, (WPARAM)vk, 0);
}

static void mac_queue_unmodified_key_pulse(int vk)
{
	MacKeyState[vk & 0xFF] = 1;
	mac_queue_message((HWND)(intptr_t)1, WM_KEYDOWN, (WPARAM)vk, MOBILE_KEY_UNMODIFIED_LPARAM);
	MacKeyState[vk & 0xFF] = 0;
	mac_queue_message((HWND)(intptr_t)1, WM_KEYUP, (WPARAM)vk, MOBILE_KEY_UNMODIFIED_LPARAM);
}

static int mac_pop_message(MSG *msg, bool remove)
{
	if (MacMessageHead == MacMessageTail) {
		return FALSE;
	}
	if (msg) {
		*msg = MacMessageQueue[MacMessageHead];
	}
	if (remove) {
		MacMessageHead = (MacMessageHead + 1) % (int)(sizeof(MacMessageQueue) / sizeof(MacMessageQueue[0]));
	}
	return TRUE;
}

static bool mac_has_pending_messages(void)
{
	return MacMessageHead != MacMessageTail;
}

#if defined(RA_MOBILE_TOUCH)
static void mobile_idle_delay(bool allow_idle_delay, bool saw_sdl_event)
{
	if (allow_idle_delay && !saw_sdl_event && !mac_has_pending_messages()) {
		SDL_Delay(1);
	}
}
#endif

static int mac_vk_from_sdl(SDL_Keycode key)
{
	if (key >= SDLK_a && key <= SDLK_z) {
		return 'A' + (int)(key - SDLK_a);
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return '0' + (int)(key - SDLK_0);
	}
	if (key >= SDLK_F1 && key <= SDLK_F12) {
		return VK_F1 + (int)(key - SDLK_F1);
	}

	switch (key) {
		case SDLK_RETURN: return VK_RETURN;
		case SDLK_KP_ENTER: return VK_RETURN;
		case SDLK_ESCAPE: return VK_ESCAPE;
		case SDLK_SPACE: return VK_SPACE;
		case SDLK_BACKSPACE: return VK_BACK;
		case SDLK_TAB: return VK_TAB;
		case SDLK_INSERT: return VK_INSERT;
		case SDLK_DELETE: return VK_DELETE;
		case SDLK_HOME: return VK_HOME;
		case SDLK_END: return VK_END;
		case SDLK_PAGEUP: return VK_PRIOR;
		case SDLK_PAGEDOWN: return VK_NEXT;
		case SDLK_LEFT: return VK_LEFT;
		case SDLK_RIGHT: return VK_RIGHT;
		case SDLK_UP: return VK_UP;
		case SDLK_DOWN: return VK_DOWN;
		case SDLK_PAUSE: return VK_PAUSE;
		case SDLK_CAPSLOCK: return VK_CAPITAL;
		case SDLK_NUMLOCKCLEAR: return VK_NUMLOCK;
		case SDLK_KP_0: return VK_NUMPAD0;
		case SDLK_KP_1: return VK_NUMPAD1;
		case SDLK_KP_2: return VK_NUMPAD2;
		case SDLK_KP_3: return VK_NUMPAD3;
		case SDLK_KP_4: return VK_NUMPAD4;
		case SDLK_KP_5: return VK_NUMPAD5;
		case SDLK_KP_6: return VK_NUMPAD6;
		case SDLK_KP_7: return VK_NUMPAD7;
		case SDLK_KP_8: return VK_NUMPAD8;
		case SDLK_KP_9: return VK_NUMPAD9;
		case SDLK_KP_MULTIPLY: return VK_MULTIPLY;
		case SDLK_KP_PLUS: return VK_ADD;
		case SDLK_KP_MINUS: return VK_SUBTRACT;
		case SDLK_KP_PERIOD: return VK_DECIMAL;
		case SDLK_KP_DIVIDE: return VK_DIVIDE;
		case SDLK_SEMICOLON: return VK_NONE_BA;
		case SDLK_EQUALS: return VK_NONE_BB;
		case SDLK_COMMA: return VK_NONE_BC;
		case SDLK_MINUS: return VK_NONE_BD;
		case SDLK_PERIOD: return VK_NONE_BE;
		case SDLK_SLASH: return VK_NONE_BF;
		case SDLK_BACKQUOTE: return VK_NONE_C0;
		case SDLK_LEFTBRACKET: return VK_NONE_DB;
		case SDLK_BACKSLASH: return VK_NONE_DC;
		case SDLK_RIGHTBRACKET: return VK_NONE_DD;
		case SDLK_QUOTE: return VK_NONE_DE;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT: return VK_SHIFT;
		case SDLK_LCTRL:
		case SDLK_RCTRL: return VK_CONTROL;
		case SDLK_LALT:
		case SDLK_RALT:
		case SDLK_LGUI:
		case SDLK_RGUI: return VK_MENU;
		default: break;
	}
	return 0;
}

static void mac_update_modifier_state(void)
{
	SDL_Keymod mods = SDL_GetModState();
	MacKeyState[VK_SHIFT] = (mods & KMOD_SHIFT) ? 1 : 0;
	MacKeyState[VK_CONTROL] = (mods & KMOD_CTRL) ? 1 : 0;
	MacKeyState[VK_MENU] = (mods & KMOD_ALT) ? 1 : 0;
	MacToggleState[VK_CAPITAL] = (mods & KMOD_CAPS) ? 1 : 0;
	MacToggleState[VK_NUMLOCK] = (mods & KMOD_NUM) ? 1 : 0;
}

static bool mac_state_down(BYTE const *state, int key)
{
	if (state) {
		return (state[key & 0xFF] & 0x80) != 0;
	}
	return MacKeyState[key & 0xFF] != 0;
}

static bool mac_state_toggled(BYTE const *state, int key)
{
	if (state) {
		return (state[key & 0xFF] & 0x09) != 0;
	}
	return MacToggleState[key & 0xFF] != 0;
}

static int mac_write_ascii(LPWORD out, unsigned char ch)
{
	if (!out) {
		return 0;
	}
	*out = (WORD)ch;
	return 1;
}

static int mac_ascii_from_vk(UINT virt_key, BYTE const *state, LPWORD out)
{
	virt_key &= 0xFF;
	bool shift = mac_state_down(state, VK_SHIFT);
	bool caps = mac_state_toggled(state, VK_CAPITAL);

	if (virt_key >= 'A' && virt_key <= 'Z') {
		unsigned char base = (unsigned char)('a' + (virt_key - 'A'));
		if (shift ^ caps) {
			base = (unsigned char)('A' + (virt_key - 'A'));
		}
		return mac_write_ascii(out, base);
	}

	if (virt_key >= '0' && virt_key <= '9') {
		static const char shifted[] = ")!@#$%^&*(";
		unsigned char ch = shift ? (unsigned char)shifted[virt_key - '0'] : (unsigned char)virt_key;
		return mac_write_ascii(out, ch);
	}

	if (virt_key >= VK_NUMPAD0 && virt_key <= VK_NUMPAD9) {
		return mac_write_ascii(out, (unsigned char)('0' + (virt_key - VK_NUMPAD0)));
	}

	switch (virt_key) {
		case VK_SPACE: return mac_write_ascii(out, ' ');
		case VK_TAB: return mac_write_ascii(out, '\t');
		case VK_RETURN: return mac_write_ascii(out, '\r');
		case VK_BACK: return mac_write_ascii(out, '\b');
		case VK_MULTIPLY: return mac_write_ascii(out, '*');
		case VK_ADD: return mac_write_ascii(out, '+');
		case VK_SEPARATOR: return mac_write_ascii(out, ',');
		case VK_SUBTRACT: return mac_write_ascii(out, '-');
		case VK_DECIMAL: return mac_write_ascii(out, '.');
		case VK_DIVIDE: return mac_write_ascii(out, '/');
		case VK_NONE_BA: return mac_write_ascii(out, shift ? ':' : ';');
		case VK_NONE_BB: return mac_write_ascii(out, shift ? '+' : '=');
		case VK_NONE_BC: return mac_write_ascii(out, shift ? '<' : ',');
		case VK_NONE_BD: return mac_write_ascii(out, shift ? '_' : '-');
		case VK_NONE_BE: return mac_write_ascii(out, shift ? '>' : '.');
		case VK_NONE_BF: return mac_write_ascii(out, shift ? '?' : '/');
		case VK_NONE_C0: return mac_write_ascii(out, shift ? '~' : '`');
		case VK_NONE_DB: return mac_write_ascii(out, shift ? '{' : '[');
		case VK_NONE_DC: return mac_write_ascii(out, shift ? '|' : '\\');
		case VK_NONE_DD: return mac_write_ascii(out, shift ? '}' : ']');
		case VK_NONE_DE: return mac_write_ascii(out, shift ? '"' : '\'');
		default: return 0;
	}
}

static int mac_vk_from_button(unsigned char button)
{
	switch (button) {
		case SDL_BUTTON_LEFT: return VK_LBUTTON;
		case SDL_BUTTON_RIGHT: return VK_RBUTTON;
		case SDL_BUTTON_MIDDLE: return VK_MBUTTON;
		default: return 0;
	}
}

#if defined(RA_MOBILE_TOUCH)
static bool mobile_logical_point(float normalized_x, float normalized_y, int *logical_x, int *logical_y)
{
	if (!MacWindow || MacWidth <= 0 || MacHeight <= 0) {
		if (logical_x) *logical_x = 0;
		if (logical_y) *logical_y = 0;
		return false;
	}

	int window_w = MacWidth;
	int window_h = MacHeight;
	SDL_GetWindowSize(MacWindow, &window_w, &window_h);
	if (window_w <= 0 || window_h <= 0) {
		if (logical_x) *logical_x = 0;
		if (logical_y) *logical_y = 0;
		return false;
	}

	int screen_x = (int)(normalized_x * (float)window_w);
	int screen_y = (int)(normalized_y * (float)window_h);
	screen_x = RA_ClampInt(screen_x, 0, window_w - 1);
	screen_y = RA_ClampInt(screen_y, 0, window_h - 1);

	RAAspectViewport viewport = RA_CalculateAspectViewport(MacWidth, MacHeight, window_w, window_h);
	return RA_MapViewportPoint(viewport, MacWidth, MacHeight, screen_x, screen_y, logical_x, logical_y) != 0;
}

static void mobile_emit_pan_key(void *, int vk)
{
	mac_queue_unmodified_key_pulse(vk);
}

static void mobile_emit_pan(float dx, float dy)
{
	MobilePan_EmitFingerDelta(&MobilePan, dx, dy, MacWidth, MacHeight, mobile_emit_pan_key, 0);
}

static void mobile_queue_touch_output(MobileTouchGestureOutput const *out)
{
	if (!out) {
		return;
	}
	for (int index = 0; index < out->count; ++index) {
		MobileTouchGestureEvent const *touch_event = &out->events[index];
		switch (touch_event->type) {
			case MOBILE_TOUCH_MOUSE_MOVE:
				mac_queue_mouse_motion(touch_event->x, touch_event->y);
				break;
			case MOBILE_TOUCH_LEFT_DOWN:
				mac_queue_mouse_button_with_cursor(VK_LBUTTON, true, touch_event->x, touch_event->y, touch_event->update_cursor != 0);
				break;
			case MOBILE_TOUCH_LEFT_UP:
				mac_queue_mouse_button_with_cursor(VK_LBUTTON, false, touch_event->x, touch_event->y, touch_event->update_cursor != 0);
				break;
			case MOBILE_TOUCH_RIGHT_DOWN:
				mac_queue_mouse_button_with_cursor(VK_RBUTTON, true, touch_event->x, touch_event->y, touch_event->update_cursor != 0);
				break;
			case MOBILE_TOUCH_RIGHT_UP:
				mac_queue_mouse_button_with_cursor(VK_RBUTTON, false, touch_event->x, touch_event->y, touch_event->update_cursor != 0);
				break;
		}
	}
}

static void mobile_maybe_send_long_press(void)
{
	MobileTouchGestureOutput out;
	MobileTouchGesture_Update(&MobileTouch, mac_now_ms(), &out);
	mobile_queue_touch_output(&out);
}
#endif

static void mac_destroy_video_objects(void)
{
	if (MacTexture) {
		SDL_DestroyTexture(MacTexture);
		MacTexture = 0;
	}
	if (MacRenderer) {
		SDL_DestroyRenderer(MacRenderer);
		MacRenderer = 0;
	}
	if (MacWindow) {
		SDL_DestroyWindow(MacWindow);
		MacWindow = 0;
	}
}

bool MacSDL_SetFullscreen(bool enabled)
{
	MacFullscreen = enabled;
	if (!MacWindow) {
		return true;
	}
	if (SDL_SetWindowFullscreen(MacWindow, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
		return false;
	}
	MacFullscreen = enabled;
	return true;
}

bool MacSDL_GetFullscreen(void)
{
	return MacFullscreen;
}

bool MacSDL_SetMode(int width, int height)
{
	if (width <= 0 || height <= 0) {
		return false;
	}

	if (!MacSDLReady) {
		SDL_SetMainReady();
#if defined(RA_MOBILE_TOUCH)
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
#else
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#endif
		SDL_SetHint(SDL_HINT_RENDER_LOGICAL_SIZE_MODE, "letterbox");
#if defined(RA_IOS)
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
		SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
#endif
#if defined(RA_MOBILE_TOUCH)
		SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
		SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif
		if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
			return false;
		}
		MacSDLReady = true;
		MacMainThread = SDL_ThreadID();
		mac_update_modifier_state();
		mac_default_palette();
#if defined(RA_MOBILE_TOUCH)
		MobileTouchGesture_Init(&MobileTouch);
		MobilePan_Init(&MobilePan);
#endif
	}

	if (MacWindow && MacWidth == width && MacHeight == height) {
		if (mac_fullscreen_env_requested() && !MacFullscreen) {
			MacSDL_SetFullscreen(true);
		}
		return true;
	}

	mac_destroy_video_objects();
	MacWidth = width;
	MacHeight = height;

	if (mac_fullscreen_env_requested()) {
		MacFullscreen = true;
	}
	Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if defined(RA_MOBILE_TOUCH)
	window_flags |= SDL_WINDOW_BORDERLESS;
#endif
#if defined(RA_IOS)
	window_flags &= ~SDL_WINDOW_RESIZABLE;
#endif
	if (MacFullscreen) {
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	MacWindow = SDL_CreateWindow("Command & Conquer: Red Alert", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
	if (!MacWindow) {
		return false;
	}

	Uint32 renderer_flags = SDL_RENDERER_SOFTWARE;
#if defined(RA_MOBILE_TOUCH)
	renderer_flags = SDL_RENDERER_ACCELERATED;
#endif
	MacRenderer = SDL_CreateRenderer(MacWindow, -1, renderer_flags);
	if (!MacRenderer) {
		mac_destroy_video_objects();
		return false;
	}

	SDL_RendererInfo renderer_info;
	if (SDL_GetRendererInfo(MacRenderer, &renderer_info) == 0) {
		SDL_Log("Red Alert renderer=%s flags=0x%08x", renderer_info.name ? renderer_info.name : "(unknown)", renderer_info.flags);
	}
	SDL_SetRenderDrawColor(MacRenderer, 0, 0, 0, 255);
	MacTexture = SDL_CreateTexture(MacRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	return MacTexture != 0;
}

void MacSDL_Shutdown(void)
{
	if (MacFrame) {
		free(MacFrame);
		MacFrame = 0;
		MacFramePixels = 0;
	}
	mac_destroy_video_objects();
	if (MacSDLReady) {
		SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
		MacSDLReady = false;
		MacMainThread = 0;
	}
}

void MacSDL_SetPalette(PALETTEENTRY const *entries, int count)
{
	if (!entries) {
		return;
	}
	if (count > 256) {
		count = 256;
	}
	for (int index = 0; index < count; ++index) {
		MacPalette[index] = mac_argb(entries[index].peRed, entries[index].peGreen, entries[index].peBlue);
	}
}

static void mac_sdl_pump_events(bool allow_idle_delay)
{
	if (!MacSDLReady) {
		return;
	}
	if (MacMainThread && SDL_ThreadID() != MacMainThread) {
		return;
	}
	MacMM_PumpTimers();
#if defined(RA_MOBILE_TOUCH)
	mobile_maybe_send_long_press();
#endif
	SDL_Event event;
	bool saw_sdl_event = false;
	while (SDL_PollEvent(&event)) {
		saw_sdl_event = true;
		switch (event.type) {
			case SDL_QUIT:
				MacQuitRequested = true;
				mac_queue_message((HWND)(intptr_t)1, WM_DESTROY, 0, 0);
				break;

			case SDL_MOUSEMOTION: {
				int x = event.motion.x;
				int y = event.motion.y;
				mac_to_logical_point(&x, &y);
				mac_queue_mouse_motion(x, y);
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				int vk = mac_vk_from_button(event.button.button);
				if (!vk) {
					break;
				}
				int x = event.button.x;
				int y = event.button.y;
				mac_to_logical_point(&x, &y);
				mac_queue_mouse_button(vk, event.type == SDL_MOUSEBUTTONDOWN, x, y);
				break;
			}

#if defined(RA_MOBILE_TOUCH)
			case SDL_FINGERDOWN: {
				int x = 0;
				int y = 0;
				if (!mobile_logical_point(event.tfinger.x, event.tfinger.y, &x, &y)) {
					break;
				}
				MobileTouchGestureOutput out;
				MobileTouchGesture_Begin(&MobileTouch, (long long)event.tfinger.fingerId, x, y, mac_now_ms(), &out);
				if (MobileTouch.secondary_active && MobileTouch.secondary_finger == (long long)event.tfinger.fingerId) {
					MobilePan_Reset(&MobilePan);
				}
				mobile_queue_touch_output(&out);
				break;
			}

			case SDL_FINGERMOTION: {
				int x = 0;
				int y = 0;
				mobile_logical_point(event.tfinger.x, event.tfinger.y, &x, &y);
				if (MobileTouchGesture_IsPanFinger(&MobileTouch, (long long)event.tfinger.fingerId)) {
					mobile_emit_pan(event.tfinger.dx, event.tfinger.dy);
					break;
				}
				MobileTouchGestureOutput out;
				MobileTouchGesture_Move(&MobileTouch, (long long)event.tfinger.fingerId, x, y, &out);
				mobile_queue_touch_output(&out);
				break;
			}

			case SDL_FINGERUP: {
				int x = 0;
				int y = 0;
				mobile_logical_point(event.tfinger.x, event.tfinger.y, &x, &y);
				MobileTouchGestureOutput out;
				MobileTouchGesture_End(&MobileTouch, (long long)event.tfinger.fingerId, x, y, &out);
				mobile_queue_touch_output(&out);
				break;
			}

			case SDL_MULTIGESTURE:
				if (MobileTouch.secondary_active) {
					MobilePan_IgnoreMultiGestureCenter(&MobilePan, event.mgesture.x, event.mgesture.y, MacWidth, MacHeight, mobile_emit_pan_key, 0);
				}
				break;
#endif

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				if (event.type == SDL_KEYDOWN && !event.key.repeat &&
						(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) &&
						(event.key.keysym.mod & (KMOD_GUI | KMOD_ALT))) {
					MacSDL_SetFullscreen(!MacFullscreen);
					break;
				}
				int vk = mac_vk_from_sdl(event.key.keysym.sym);
				if (!vk) {
					break;
				}
				MacKeyState[vk & 0xFF] = (event.type == SDL_KEYDOWN) ? 1 : 0;
				mac_update_modifier_state();
				mac_queue_message((HWND)(intptr_t)1, event.type == SDL_KEYDOWN ? WM_KEYDOWN : WM_KEYUP, (WPARAM)vk, 0);
				break;
			}

			default:
				break;
		}
	}
#if defined(RA_MOBILE_TOUCH)
	mobile_idle_delay(allow_idle_delay, saw_sdl_event);
#endif
}

void MacSDL_PumpEvents(void)
{
	mac_sdl_pump_events(true);
}

bool MacSDL_QuitRequested(void)
{
	return MacQuitRequested;
}

bool MacSDL_TouchCursorHidden(void)
{
#if defined(RA_MOBILE_TOUCH)
	return MobileTouchCursorHidden;
#else
	return false;
#endif
}

int MacSDL_ConsumeMobilePointerDrag(int *x, int *y)
{
#if defined(RA_MOBILE_TOUCH)
	if (!MobilePointerDragCandidate) {
		return 0;
	}
	MobilePointerDragCandidate = false;
	if (x) {
		*x = MobilePointerDragStartX;
	}
	if (y) {
		*y = MobilePointerDragStartY;
	}
	return 1;
#else
	(void)x;
	(void)y;
	return 0;
#endif
}

void MacSDL_Present8(unsigned char const *pixels, int width, int height, int pitch)
{
	if (!pixels || width <= 0 || height <= 0) {
		return;
	}
	if (MacMainThread && SDL_ThreadID() != MacMainThread) {
		return;
	}

	mac_sdl_pump_events(false);
	if (!MacRenderer || !MacTexture) {
		return;
	}

	int needed = width * height;
	if (needed > MacFramePixels) {
		uint32_t *new_frame = (uint32_t *)realloc(MacFrame, (size_t)needed * sizeof(uint32_t));
		if (!new_frame) {
			return;
		}
		MacFrame = new_frame;
		MacFramePixels = needed;
	}

	for (int y = 0; y < height; ++y) {
		unsigned char const *src = pixels + (y * pitch);
		uint32_t *dst = MacFrame + (y * width);
		for (int x = 0; x < width; ++x) {
			dst[x] = MacPalette[src[x]];
		}
	}

	SDL_UpdateTexture(MacTexture, 0, MacFrame, width * (int)sizeof(uint32_t));
	RAAspectViewport viewport = mac_renderer_viewport(width, height);
	SDL_Rect destination;
	destination.x = viewport.x;
	destination.y = viewport.y;
	destination.w = viewport.w;
	destination.h = viewport.h;
	if (destination.w <= 0 || destination.h <= 0) {
		return;
	}
	SDL_SetRenderDrawColor(MacRenderer, 0, 0, 0, 255);
	SDL_RenderClear(MacRenderer);
	SDL_RenderCopy(MacRenderer, MacTexture, 0, &destination);
	SDL_RenderPresent(MacRenderer);
}

extern "C" BOOL MacWin_PeekMessage(MSG *msg, HWND, UINT, UINT, UINT remove)
{
	mac_sdl_pump_events(true);
	return mac_pop_message(msg, (remove & PM_REMOVE) != 0);
}

extern "C" BOOL MacWin_GetMessage(MSG *msg, HWND, UINT, UINT)
{
	mac_sdl_pump_events(true);
	if (!mac_pop_message(msg, true)) {
		return FALSE;
	}
	return msg && msg->message == WM_QUIT ? FALSE : TRUE;
}

extern "C" BOOL MacWin_TranslateMessage(MSG const *)
{
	return TRUE;
}

extern "C" LRESULT MacWin_DispatchMessage(MSG const *msg)
{
	if (!msg) {
		return 0;
	}
	bool previous = MacUnmodifiedKeyDispatch;
	MacUnmodifiedKeyDispatch = MobileKeyMessage_IsUnmodified(msg->message, msg->lParam) != 0;
	LRESULT result = Windows_Procedure(msg->hwnd, msg->message, msg->wParam, msg->lParam);
	MacUnmodifiedKeyDispatch = previous;
	return result;
}

extern "C" BOOL MacWin_PostMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	return mac_queue_message(hwnd, message, wparam, lparam);
}

extern "C" void MacWin_PostQuitMessage(int code)
{
	mac_queue_message((HWND)(intptr_t)1, WM_QUIT, (WPARAM)code, 0);
}

extern "C" short MacWin_GetAsyncKeyState(int key)
{
	key &= 0xFF;
	return MacKeyState[key] ? (short)0x8000 : 0;
}

extern "C" short MacWin_GetKeyState(int key)
{
	key &= 0xFF;
	short state = MacKeyState[key] ? (short)0x8000 : 0;
	if (MacToggleState[key]) {
		state |= (short)0x0009;
	}
	return MobileKeyMessage_FilterKeyState(MacUnmodifiedKeyDispatch, key, state);
}

extern "C" int MacWin_ToAscii(UINT virt_key, UINT, BYTE const *key_state, LPWORD trans_key, UINT)
{
	if (mac_state_down(key_state, VK_CONTROL) || mac_state_down(key_state, VK_MENU)) {
		return 0;
	}
	return mac_ascii_from_vk(virt_key, key_state, trans_key);
}

extern "C" BOOL MacWin_GetCursorPos(LPPOINT point)
{
	if (point) {
		*point = MacMousePoint;
	}
	return TRUE;
}
