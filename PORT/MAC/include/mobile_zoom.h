#ifndef MOBILE_ZOOM_H
#define MOBILE_ZOOM_H

#include <math.h>

/*
 * Pinch-to-magnify for touch builds. The game still renders its full logical
 * frame; this only chooses which part of that frame is shown on screen and
 * maps touch points back into it. Scale 1 shows the whole frame.
 */

struct MobileZoomState {
	float scale;
	float center_x;
	float center_y;
};

struct MobileZoomRect {
	int x;
	int y;
	int w;
	int h;
};

/*
 * The last known position of up to two fingers, in window pixels, plus what
 * is needed to recognise a quick two- or three-finger tap.
 */
struct MobilePinchState {
	int active[2];
	long long finger[2];
	float x[2];
	float y[2];
	int pair_valid;
	unsigned long pair_tick;
	float pair_travel;
	int down_count;
	int max_down;
};

static const float MobileZoom_MaxScale = 3.0f;
static const float MobileZoom_SnapScale = 1.05f;
static const unsigned long MobilePinch_TapMs = 300;
static const float MobilePinch_TapTravel = 20.0f;

static inline float MobileZoom_ClampFloat(float value, float min_value, float max_value)
{
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static inline void MobileZoom_Init(MobileZoomState *zoom)
{
	if (!zoom) {
		return;
	}
	zoom->scale = 1.0f;
	zoom->center_x = 0.0f;
	zoom->center_y = 0.0f;
}

static inline int MobileZoom_IsZoomed(MobileZoomState const *zoom)
{
	return zoom && zoom->scale > 1.0f;
}

static inline void MobileZoom_Clamp(MobileZoomState *zoom, int logical_w, int logical_h)
{
	if (!zoom || logical_w <= 0 || logical_h <= 0) {
		return;
	}
	zoom->scale = MobileZoom_ClampFloat(zoom->scale, 1.0f, MobileZoom_MaxScale);
	float half_w = (float)logical_w / (2.0f * zoom->scale);
	float half_h = (float)logical_h / (2.0f * zoom->scale);
	zoom->center_x = MobileZoom_ClampFloat(zoom->center_x, half_w, (float)logical_w - half_w);
	zoom->center_y = MobileZoom_ClampFloat(zoom->center_y, half_h, (float)logical_h - half_h);
}

/* The part of the logical frame that is visible, in whole logical pixels. */
static inline MobileZoomRect MobileZoom_SourceRect(MobileZoomState const *zoom, int logical_w, int logical_h)
{
	MobileZoomRect rect = {0, 0, logical_w, logical_h};
	if (!MobileZoom_IsZoomed(zoom) || logical_w <= 0 || logical_h <= 0) {
		return rect;
	}
	rect.w = (int)((float)logical_w / zoom->scale + 0.5f);
	rect.h = (int)((float)logical_h / zoom->scale + 0.5f);
	if (rect.w < 1) rect.w = 1;
	if (rect.h < 1) rect.h = 1;
	if (rect.w > logical_w) rect.w = logical_w;
	if (rect.h > logical_h) rect.h = logical_h;
	rect.x = (int)(zoom->center_x - (float)rect.w / 2.0f + 0.5f);
	rect.y = (int)(zoom->center_y - (float)rect.h / 2.0f + 0.5f);
	if (rect.x < 0) rect.x = 0;
	if (rect.y < 0) rect.y = 0;
	if (rect.x + rect.w > logical_w) rect.x = logical_w - rect.w;
	if (rect.y + rect.h > logical_h) rect.y = logical_h - rect.h;
	return rect;
}

/*
 * Map a point given in unzoomed logical coordinates (where it would land if
 * the whole frame were shown) to the logical frame point shown under it.
 */
static inline void MobileZoom_MapPoint(
	MobileZoomState const *zoom,
	int logical_w,
	int logical_h,
	int screen_x,
	int screen_y,
	int *logical_x,
	int *logical_y)
{
	MobileZoomRect rect = MobileZoom_SourceRect(zoom, logical_w, logical_h);
	int x = screen_x;
	int y = screen_y;
	if (logical_w > 0 && logical_h > 0) {
		x = rect.x + (int)(((long long)screen_x * rect.w) / logical_w);
		y = rect.y + (int)(((long long)screen_y * rect.h) / logical_h);
		if (x >= logical_w) x = logical_w - 1;
		if (y >= logical_h) y = logical_h - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	}
	if (logical_x) *logical_x = x;
	if (logical_y) *logical_y = y;
}

/*
 * Multiply the scale by ratio, keeping the frame point under focus (given in
 * unzoomed logical coordinates) in the same place on screen.
 */
static inline void MobileZoom_Pinch(
	MobileZoomState *zoom,
	float ratio,
	float focus_x,
	float focus_y,
	int logical_w,
	int logical_h)
{
	if (!zoom || ratio <= 0.0f || logical_w <= 0 || logical_h <= 0) {
		return;
	}
	if (!MobileZoom_IsZoomed(zoom)) {
		zoom->center_x = (float)logical_w / 2.0f;
		zoom->center_y = (float)logical_h / 2.0f;
	}
	float old_scale = zoom->scale;
	float new_scale = MobileZoom_ClampFloat(old_scale * ratio, 1.0f, MobileZoom_MaxScale);

	float content_x = zoom->center_x - (float)logical_w / (2.0f * old_scale) + focus_x / old_scale;
	float content_y = zoom->center_y - (float)logical_h / (2.0f * old_scale) + focus_y / old_scale;
	zoom->scale = new_scale;
	zoom->center_x = content_x - focus_x / new_scale + (float)logical_w / (2.0f * new_scale);
	zoom->center_y = content_y - focus_y / new_scale + (float)logical_h / (2.0f * new_scale);
	MobileZoom_Clamp(zoom, logical_w, logical_h);
}

/* Drop back to the whole frame when a pinch ends barely zoomed in. */
static inline void MobileZoom_SnapIfNearlyUnzoomed(MobileZoomState *zoom)
{
	if (zoom && zoom->scale < MobileZoom_SnapScale) {
		zoom->scale = 1.0f;
	}
}

/*
 * Move the magnified view with the fingers. dx/dy are the finger movement in
 * unzoomed logical pixels. Whatever movement the view can't absorb because it
 * reached the frame edge is returned in remaining_dx/dy (same units), so the
 * caller can scroll the map with it. Unzoomed, everything is returned.
 */
static inline void MobileZoom_Pan(
	MobileZoomState *zoom,
	float dx,
	float dy,
	int logical_w,
	int logical_h,
	float *remaining_dx,
	float *remaining_dy)
{
	float rest_x = dx;
	float rest_y = dy;
	if (MobileZoom_IsZoomed(zoom) && logical_w > 0 && logical_h > 0) {
		float wanted_x = zoom->center_x - dx / zoom->scale;
		float wanted_y = zoom->center_y - dy / zoom->scale;
		zoom->center_x = wanted_x;
		zoom->center_y = wanted_y;
		MobileZoom_Clamp(zoom, logical_w, logical_h);
		rest_x = -(wanted_x - zoom->center_x) * zoom->scale;
		rest_y = -(wanted_y - zoom->center_y) * zoom->scale;
	}
	if (remaining_dx) *remaining_dx = rest_x;
	if (remaining_dy) *remaining_dy = rest_y;
}

static inline void MobilePinch_Init(MobilePinchState *pinch)
{
	if (!pinch) {
		return;
	}
	for (int index = 0; index < 2; ++index) {
		pinch->active[index] = 0;
		pinch->finger[index] = 0;
		pinch->x[index] = 0.0f;
		pinch->y[index] = 0.0f;
	}
	pinch->pair_valid = 0;
	pinch->pair_tick = 0;
	pinch->pair_travel = 0.0f;
	pinch->down_count = 0;
	pinch->max_down = 0;
}

static inline int MobilePinch_Slot(MobilePinchState const *pinch, long long finger)
{
	for (int index = 0; index < 2; ++index) {
		if (pinch->active[index] && pinch->finger[index] == finger) {
			return index;
		}
	}
	return -1;
}

static inline int MobilePinch_BothActive(MobilePinchState const *pinch)
{
	return pinch && pinch->active[0] && pinch->active[1];
}

static inline void MobilePinch_Down(MobilePinchState *pinch, long long finger, float x, float y, unsigned long tick)
{
	if (!pinch || MobilePinch_Slot(pinch, finger) >= 0) {
		return;
	}
	pinch->down_count++;
	if (pinch->down_count > pinch->max_down) {
		pinch->max_down = pinch->down_count;
	}
	for (int index = 0; index < 2; ++index) {
		if (!pinch->active[index]) {
			pinch->active[index] = 1;
			pinch->finger[index] = finger;
			pinch->x[index] = x;
			pinch->y[index] = y;
			if (MobilePinch_BothActive(pinch)) {
				pinch->pair_valid = 1;
				pinch->pair_tick = tick;
				pinch->pair_travel = 0.0f;
			}
			return;
		}
	}
}

/*
 * Call when the first finger of a multi-finger touch lifts, before
 * MobilePinch_Up. If the touch was a quick tap (fingers down briefly and
 * barely moving), returns how many fingers it used: 2 or 3. Otherwise 0.
 */
static inline int MobilePinch_EndTapFingers(MobilePinchState *pinch, unsigned long tick)
{
	if (!pinch || !MobilePinch_BothActive(pinch) || !pinch->pair_valid) {
		return 0;
	}
	pinch->pair_valid = 0;
	if ((unsigned long)(tick - pinch->pair_tick) > MobilePinch_TapMs ||
			pinch->pair_travel > MobilePinch_TapTravel) {
		return 0;
	}
	return pinch->max_down >= 3 ? 3 : 2;
}

static inline void MobilePinch_Up(MobilePinchState *pinch, long long finger)
{
	if (!pinch) {
		return;
	}
	int slot = MobilePinch_Slot(pinch, finger);
	if (slot >= 0) {
		pinch->active[slot] = 0;
	}
	if (pinch->down_count > 0) {
		pinch->down_count--;
	}
	if (pinch->down_count == 0) {
		pinch->max_down = 0;
	}
}

/*
 * Record a finger move. When both fingers are down, returns 1 and reports how
 * the pair changed: the distance ratio (new / old), the old midpoint, and how
 * far the midpoint moved, all in the same units as the positions.
 */
static inline int MobilePinch_Move(
	MobilePinchState *pinch,
	long long finger,
	float x,
	float y,
	float *ratio,
	float *mid_x,
	float *mid_y,
	float *mid_dx,
	float *mid_dy)
{
	if (!pinch) {
		return 0;
	}
	int slot = MobilePinch_Slot(pinch, finger);
	if (slot < 0) {
		return 0;
	}
	if (!MobilePinch_BothActive(pinch)) {
		pinch->x[slot] = x;
		pinch->y[slot] = y;
		return 0;
	}

	int other = 1 - slot;
	float old_dx = pinch->x[slot] - pinch->x[other];
	float old_dy = pinch->y[slot] - pinch->y[other];
	float new_dx = x - pinch->x[other];
	float new_dy = y - pinch->y[other];
	float old_dist_sq = old_dx * old_dx + old_dy * old_dy;
	float new_dist_sq = new_dx * new_dx + new_dy * new_dy;
	float old_mid_x = (pinch->x[slot] + pinch->x[other]) / 2.0f;
	float old_mid_y = (pinch->y[slot] + pinch->y[other]) / 2.0f;

	float distance_ratio = 1.0f;
	if (old_dist_sq > 1.0f && new_dist_sq > 1.0f) {
		distance_ratio = sqrtf(new_dist_sq / old_dist_sq);
	}

	float moved_x = x - pinch->x[slot];
	float moved_y = y - pinch->y[slot];
	pinch->pair_travel += (moved_x < 0.0f ? -moved_x : moved_x) + (moved_y < 0.0f ? -moved_y : moved_y);
	pinch->x[slot] = x;
	pinch->y[slot] = y;
	if (ratio) *ratio = distance_ratio;
	if (mid_x) *mid_x = old_mid_x;
	if (mid_y) *mid_y = old_mid_y;
	if (mid_dx) *mid_dx = (x + pinch->x[other]) / 2.0f - old_mid_x;
	if (mid_dy) *mid_dy = (y + pinch->y[other]) / 2.0f - old_mid_y;
	return 1;
}

#endif
