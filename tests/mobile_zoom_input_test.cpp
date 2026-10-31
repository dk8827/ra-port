// Send SDL touch events through the same handler used by Android and iOS.
#define RA_MOBILE_TOUCH 1
#include "../PORT/MAC/src/mac_sdl_runtime.cpp"
#include <assert.h>

bool InMovie = false;
LRESULT FAR PASCAL _export Windows_Procedure(HWND, UINT, WPARAM, LPARAM) { return 0; }

static void touch(Uint32 type, SDL_FingerID id, float x, float y)
{
	SDL_Event event;
	memset(&event, 0, sizeof(event));
	event.type = type;
	event.tfinger.fingerId = id;
	event.tfinger.x = x;
	event.tfinger.y = y;
	assert(SDL_PushEvent(&event) == 1);
	MacSDL_PumpEvents();
}

static int right_clicks()
{
	MSG message;
	int count = 0;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		if (message.message == WM_RBUTTONDOWN) ++count;
	return count;
}

static void pair_tap()
{
	touch(SDL_FINGERDOWN, 1, .4f, .5f);
	touch(SDL_FINGERDOWN, 2, .6f, .5f);
	touch(SDL_FINGERUP, 1, .4f, .5f);
	touch(SDL_FINGERUP, 2, .6f, .5f);
}

int main()
{
	SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
	SDL_setenv("RA_FULLSCREEN", "0", 1);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE);
	assert(MacSDL_SetMode(640, 400));
	touch(SDL_FINGERDOWN, 1, .4f, .5f);
	touch(SDL_FINGERDOWN, 2, .6f, .5f);
	touch(SDL_FINGERMOTION, 1, .3f, .5f);
	touch(SDL_FINGERMOTION, 2, .7f, .5f);
	touch(SDL_FINGERUP, 1, .3f, .5f);
	touch(SDL_FINGERUP, 2, .7f, .5f);
	assert(MobileZoom.scale > 1.9f && MobileZoom.scale < 2.1f);
	assert(right_clicks() == 0);
	pair_tap();
	assert(MobileZoom.scale > 1.9f);
	assert(right_clicks() == 1);
	pair_tap();
	assert(MobileZoom.scale == 1.0f);
	assert(right_clicks() == 0);

	MobileZoom.scale = 2.0f;
	pair_tap();
	touch(SDL_FINGERDOWN, 3, .5f, .5f);
	touch(SDL_FINGERUP, 3, .5f, .5f);
	pair_tap();
	assert(MobileZoom.scale == 2.0f); // Single-finger input breaks a double tap.
	SDL_Delay(MobilePinch_DoubleTapMs + 1);
	pair_tap();
	assert(MobileZoom.scale == 2.0f);
	MacSDL_Shutdown();
	return 0;
}
