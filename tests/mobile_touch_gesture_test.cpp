#include "mobile_touch_gesture.h"

#include <assert.h>

static void test_tap_clicks_on_release(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 1, 100, 120, 1000, &out);
	assert(out.count == 0);

	MobileTouchGesture_End(&touch, 1, 100, 120, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
	assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
	assert(out.events[0].x == 100);
	assert(out.events[0].y == 120);
	assert(out.events[0].update_cursor == 0);
	assert(out.events[1].update_cursor == 0);
}

static void test_drag_holds_left_button_after_slop(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 4, 50, 60, 2000, &out);
	MobileTouchGesture_Move(&touch, 4, 70, 60, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
	assert(out.events[0].x == 50);
	assert(out.events[0].y == 60);
	assert(out.events[0].update_cursor == 1);
	assert(out.events[1].type == MOBILE_TOUCH_MOUSE_MOVE);
	assert(out.events[1].x == 70);
	assert(out.events[1].y == 60);

	MobileTouchGesture_End(&touch, 4, 75, 65, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_MOUSE_MOVE);
	assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
	assert(out.events[1].update_cursor == 0);
}

static void test_two_finger_pan_releases_drag_without_lingering_cursor(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 5, 50, 60, 2500, &out);
	MobileTouchGesture_Move(&touch, 5, 70, 60, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
	assert(out.events[0].update_cursor == 1);

	MobileTouchGesture_Begin(&touch, 6, 80, 90, 2510, &out);
	assert(out.count == 1);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_UP);
	assert(out.events[0].update_cursor == 0);
}

static void test_long_press_is_right_click(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 7, 30, 40, 3000, &out);
	MobileTouchGesture_Update(&touch, 3650, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_RIGHT_DOWN);
	assert(out.events[1].type == MOBILE_TOUCH_RIGHT_UP);
	assert(out.events[0].update_cursor == 0);
	assert(out.events[1].update_cursor == 0);

	MobileTouchGesture_End(&touch, 7, 30, 40, &out);
	assert(out.count == 0);
}

static void test_delayed_drag_after_long_press_starts_selection(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 11, 30, 40, 6000, &out);
	MobileTouchGesture_Update(&touch, 6650, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_RIGHT_DOWN);
	assert(out.events[1].type == MOBILE_TOUCH_RIGHT_UP);

	MobileTouchGesture_Move(&touch, 11, 90, 110, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
	assert(out.events[0].x == 30);
	assert(out.events[0].y == 40);
	assert(out.events[0].update_cursor == 1);
	assert(out.events[1].type == MOBILE_TOUCH_MOUSE_MOVE);
	assert(out.events[1].x == 90);
	assert(out.events[1].y == 110);

	MobileTouchGesture_End(&touch, 11, 100, 120, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_MOUSE_MOVE);
	assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
}

static void test_small_motion_does_not_hover(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 8, 10, 20, 4000, &out);
	MobileTouchGesture_Move(&touch, 8, 13, 23, &out);
	assert(out.count == 0);

	MobileTouchGesture_End(&touch, 8, 13, 23, &out);
	assert(out.count == 2);
	assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
	assert(out.events[0].update_cursor == 0);
	assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
	assert(out.events[1].update_cursor == 0);
}

static void test_two_finger_pan_cancels_tap(void)
{
	MobileTouchGesture touch;
	MobileTouchGestureOutput out;

	MobileTouchGesture_Init(&touch);
	MobileTouchGesture_Begin(&touch, 9, 40, 50, 5000, &out);
	MobileTouchGesture_Begin(&touch, 10, 80, 90, 5010, &out);
	assert(out.count == 0);

	MobileTouchGesture_End(&touch, 9, 42, 52, &out);
	assert(out.count == 0);

	MobileTouchGesture_Update(&touch, 5700, &out);
	assert(out.count == 0);

	MobileTouchGesture_End(&touch, 10, 82, 92, &out);
	assert(out.count == 0);
}

int main(void)
{
	test_tap_clicks_on_release();
	test_drag_holds_left_button_after_slop();
	test_two_finger_pan_releases_drag_without_lingering_cursor();
	test_long_press_is_right_click();
	test_delayed_drag_after_long_press_starts_selection();
	test_small_motion_does_not_hover();
	test_two_finger_pan_cancels_tap();
	return 0;
}
