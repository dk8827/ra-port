#ifndef MOBILE_TOUCH_GESTURE_H
#define MOBILE_TOUCH_GESTURE_H

enum MobileTouchGestureEventType {
	MOBILE_TOUCH_MOUSE_MOVE,
	MOBILE_TOUCH_LEFT_DOWN,
	MOBILE_TOUCH_LEFT_UP,
	MOBILE_TOUCH_RIGHT_DOWN,
	MOBILE_TOUCH_RIGHT_UP
};

struct MobileTouchGestureEvent {
	MobileTouchGestureEventType type;
	int x;
	int y;
	int update_cursor;
};

struct MobileTouchGestureOutput {
	int count;
	MobileTouchGestureEvent events[4];
};

struct MobileTouchGesture {
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

static inline int MobileTouchGesture_Abs(int value)
{
	return value < 0 ? -value : value;
}

static inline int MobileTouchGesture_MovedPastSlop(MobileTouchGesture const *touch, int x, int y)
{
	static const int drag_slop = 6;
	return MobileTouchGesture_Abs(x - touch->start_x) > drag_slop ||
		MobileTouchGesture_Abs(y - touch->start_y) > drag_slop;
}

static inline void MobileTouchGesture_ClearOutput(MobileTouchGestureOutput *out)
{
	if (out) {
		out->count = 0;
	}
}

static inline void MobileTouchGesture_Add(
	MobileTouchGestureOutput *out,
	MobileTouchGestureEventType type,
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

static inline void MobileTouchGesture_Init(MobileTouchGesture *touch)
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

static inline void MobileTouchGesture_Begin(
	MobileTouchGesture *touch,
	long long finger,
	int x,
	int y,
	unsigned long tick,
	MobileTouchGestureOutput *out)
{
	MobileTouchGesture_ClearOutput(out);
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
			MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, touch->last_x, touch->last_y, 0);
			touch->left_down = 0;
			touch->dragging = 0;
		}
	}
}

static inline void MobileTouchGesture_Move(
	MobileTouchGesture *touch,
	long long finger,
	int x,
	int y,
	MobileTouchGestureOutput *out)
{
	MobileTouchGesture_ClearOutput(out);
	if (!touch || !touch->primary_active || finger != touch->primary_finger) {
		return;
	}

	if (!touch->left_down && !touch->secondary_active && MobileTouchGesture_MovedPastSlop(touch, x, y)) {
		touch->left_down = 1;
		touch->dragging = 1;
		touch->tap_cancelled = 1;
		MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_DOWN, touch->start_x, touch->start_y, 1);
	}
	if (touch->left_down) {
		MobileTouchGesture_Add(out, MOBILE_TOUCH_MOUSE_MOVE, x, y, 1);
	}

	touch->last_x = x;
	touch->last_y = y;
}

static inline void MobileTouchGesture_Update(
	MobileTouchGesture *touch,
	unsigned long tick,
	MobileTouchGestureOutput *out)
{
	static const unsigned long long_press_ms = 650;

	MobileTouchGesture_ClearOutput(out);
	if (!touch || !touch->primary_active || touch->secondary_active ||
			touch->left_down || touch->long_press_sent || touch->tap_cancelled) {
		return;
	}
	if (MobileTouchGesture_MovedPastSlop(touch, touch->last_x, touch->last_y)) {
		return;
	}
	if ((unsigned long)(tick - touch->down_tick) < long_press_ms) {
		return;
	}

	MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_DOWN, touch->last_x, touch->last_y, 0);
	MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_UP, touch->last_x, touch->last_y, 0);
	touch->long_press_sent = 1;
}

static inline void MobileTouchGesture_End(
	MobileTouchGesture *touch,
	long long finger,
	int x,
	int y,
	MobileTouchGestureOutput *out)
{
	MobileTouchGesture_ClearOutput(out);
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
			MobileTouchGesture_Add(out, MOBILE_TOUCH_MOUSE_MOVE, x, y, 1);
		}
		MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, x, y, 0);
	} else if (!touch->long_press_sent && !touch->tap_cancelled) {
		MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_DOWN, x, y, 0);
		MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, x, y, 0);
	}

	MobileTouchGesture_Init(touch);
}

static inline int MobileTouchGesture_IsPanFinger(MobileTouchGesture const *touch, long long finger)
{
	return touch && touch->secondary_active &&
		(finger == touch->primary_finger || finger == touch->secondary_finger);
}

#endif
