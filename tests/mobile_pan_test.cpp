#include "mobile_pan.h"
#include "windows.h"

#include <assert.h>

struct PanCapture {
	int count;
	int keys[16];
};

static void capture_key(void *context, int vk)
{
	PanCapture *capture = (PanCapture *)context;
	assert(capture->count < (int)(sizeof(capture->keys) / sizeof(capture->keys[0])));
	capture->keys[capture->count++] = vk;
}

static void test_vertical_drag_up_scrolls_south(void)
{
	MobilePanState pan;
	PanCapture capture = {0, {0}};

	MobilePan_Init(&pan);
	MobilePan_EmitFingerDelta(&pan, 0.0f, -0.07f, 640, 400, capture_key, &capture);

	assert(capture.count > 0);
	assert(capture.keys[0] == VK_DOWN);
}

static void test_vertical_drag_down_scrolls_north(void)
{
	MobilePanState pan;
	PanCapture capture = {0, {0}};

	MobilePan_Init(&pan);
	MobilePan_EmitFingerDelta(&pan, 0.0f, 0.07f, 640, 400, capture_key, &capture);

	assert(capture.count > 0);
	assert(capture.keys[0] == VK_UP);
}

static void test_multigesture_center_coordinates_are_not_pan_deltas(void)
{
	MobilePanState pan;
	PanCapture capture = {0, {0}};

	MobilePan_Init(&pan);
	MobilePan_IgnoreMultiGestureCenter(&pan, 0.5f, 0.5f, 640, 400, capture_key, &capture);

	assert(capture.count == 0);
}

int main(void)
{
	test_vertical_drag_up_scrolls_south();
	test_vertical_drag_down_scrolls_north();
	test_multigesture_center_coordinates_are_not_pan_deltas();
	return 0;
}
