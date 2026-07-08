#ifndef ANDROID_TOUCH_GESTURE_H
#define ANDROID_TOUCH_GESTURE_H

enum AndroidTouchGestureEventType {
	ANDROID_TOUCH_MOUSE_MOVE,
	ANDROID_TOUCH_LEFT_DOWN,
	ANDROID_TOUCH_LEFT_UP,
	ANDROID_TOUCH_RIGHT_DOWN,
	ANDROID_TOUCH_RIGHT_UP
};

struct AndroidTouchGestureEvent {
	AndroidTouchGestureEventType type;
	int x;
	int y;
	int update_cursor;
};

struct AndroidTouchGestureOutput {
	int count;
	AndroidTouchGestureEvent events[4];
};

struct AndroidTouchGesture {
	int primary_active;
	int secondary_active;
	int left_down;
	int dragging;
	int long_press_sent;
	int tap_cancelled;
	long long primary_finger;
	long long secondary_finger;
	int start_x;
	int start_y;
	int last_x;
	int last_y;
	unsigned long down_tick;
};

static inline int AndroidTouchGesture_Abs(int value)
{
	return value < 0 ? -value : value;
}

static inline int AndroidTouchGesture_MovedPastSlop(AndroidTouchGesture const *touch, int x, int y)
{
	static const int drag_slop = 6;
	return AndroidTouchGesture_Abs(x - touch->start_x) > drag_slop ||
		AndroidTouchGesture_Abs(y - touch->start_y) > drag_slop;
}

static inline void AndroidTouchGesture_ClearOutput(AndroidTouchGestureOutput *out)
{
	if (out) {
		out->count = 0;
	}
}

static inline void AndroidTouchGesture_Add(
	AndroidTouchGestureOutput *out,
	AndroidTouchGestureEventType type,
	int x,
	int y,
	int update_cursor)
{
	if (!out || out->count >= (int)(sizeof(out->events) / sizeof(out->events[0]))) {
		return;
	}
	out->events[out->count].type = type;
	out->events[out->count].x = x;
	out->events[out->count].y = y;
	out->events[out->count].update_cursor = update_cursor;
	out->count++;
}

static inline void AndroidTouchGesture_Init(AndroidTouchGesture *touch)
{
	if (!touch) {
		return;
	}
	touch->primary_active = 0;
	touch->secondary_active = 0;
	touch->left_down = 0;
	touch->dragging = 0;
	touch->long_press_sent = 0;
	touch->tap_cancelled = 0;
	touch->primary_finger = 0;
	touch->secondary_finger = 0;
	touch->start_x = 0;
	touch->start_y = 0;
	touch->last_x = 0;
	touch->last_y = 0;
	touch->down_tick = 0;
}

static inline void AndroidTouchGesture_Begin(
	AndroidTouchGesture *touch,
	long long finger,
	int x,
	int y,
	unsigned long tick,
	AndroidTouchGestureOutput *out)
{
	AndroidTouchGesture_ClearOutput(out);
	if (!touch) {
		return;
	}

	if (!touch->primary_active) {
		touch->primary_active = 1;
		touch->primary_finger = finger;
		touch->start_x = x;
		touch->start_y = y;
		touch->last_x = x;
		touch->last_y = y;
		touch->down_tick = tick;
		touch->left_down = 0;
		touch->dragging = 0;
		touch->long_press_sent = 0;
		touch->tap_cancelled = 0;
		return;
	}

	if (!touch->secondary_active) {
		touch->secondary_active = 1;
		touch->secondary_finger = finger;
		touch->tap_cancelled = 1;
		if (touch->left_down) {
			AndroidTouchGesture_Add(out, ANDROID_TOUCH_LEFT_UP, touch->last_x, touch->last_y, 1);
			touch->left_down = 0;
			touch->dragging = 0;
		}
	}
}

static inline void AndroidTouchGesture_Move(
	AndroidTouchGesture *touch,
	long long finger,
	int x,
	int y,
	AndroidTouchGestureOutput *out)
{
	AndroidTouchGesture_ClearOutput(out);
	if (!touch || !touch->primary_active || finger != touch->primary_finger) {
		return;
	}

	if (!touch->long_press_sent) {
		if (!touch->left_down && AndroidTouchGesture_MovedPastSlop(touch, x, y)) {
			touch->left_down = 1;
			touch->dragging = 1;
			AndroidTouchGesture_Add(out, ANDROID_TOUCH_LEFT_DOWN, touch->start_x, touch->start_y, 1);
		}
		if (touch->left_down) {
			AndroidTouchGesture_Add(out, ANDROID_TOUCH_MOUSE_MOVE, x, y, 1);
		}
	}

	touch->last_x = x;
	touch->last_y = y;
}

static inline void AndroidTouchGesture_Update(
	AndroidTouchGesture *touch,
	unsigned long tick,
	AndroidTouchGestureOutput *out)
{
	static const unsigned long long_press_ms = 650;

	AndroidTouchGesture_ClearOutput(out);
	if (!touch || !touch->primary_active || touch->secondary_active ||
			touch->left_down || touch->long_press_sent || touch->tap_cancelled) {
		return;
	}
	if (AndroidTouchGesture_MovedPastSlop(touch, touch->last_x, touch->last_y)) {
		return;
	}
	if ((unsigned long)(tick - touch->down_tick) < long_press_ms) {
		return;
	}

	AndroidTouchGesture_Add(out, ANDROID_TOUCH_RIGHT_DOWN, touch->last_x, touch->last_y, 0);
	AndroidTouchGesture_Add(out, ANDROID_TOUCH_RIGHT_UP, touch->last_x, touch->last_y, 0);
	touch->long_press_sent = 1;
}

static inline void AndroidTouchGesture_End(
	AndroidTouchGesture *touch,
	long long finger,
	int x,
	int y,
	AndroidTouchGestureOutput *out)
{
	AndroidTouchGesture_ClearOutput(out);
	if (!touch) {
		return;
	}

	if (touch->secondary_active && finger == touch->secondary_finger) {
		touch->secondary_active = 0;
		touch->secondary_finger = 0;
		return;
	}

	if (!touch->primary_active || finger != touch->primary_finger) {
		return;
	}

	if (touch->left_down) {
		if (x != touch->last_x || y != touch->last_y) {
			AndroidTouchGesture_Add(out, ANDROID_TOUCH_MOUSE_MOVE, x, y, 1);
		}
		AndroidTouchGesture_Add(out, ANDROID_TOUCH_LEFT_UP, x, y, 1);
	} else if (!touch->long_press_sent && !touch->tap_cancelled) {
		AndroidTouchGesture_Add(out, ANDROID_TOUCH_LEFT_DOWN, x, y, 0);
		AndroidTouchGesture_Add(out, ANDROID_TOUCH_LEFT_UP, x, y, 0);
	}

	AndroidTouchGesture_Init(touch);
}

static inline int AndroidTouchGesture_IsPanFinger(AndroidTouchGesture const *touch, long long finger)
{
	return touch && touch->secondary_active &&
		(finger == touch->primary_finger || finger == touch->secondary_finger);
}

#endif
