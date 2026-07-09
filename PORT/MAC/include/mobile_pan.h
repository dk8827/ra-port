#ifndef MOBILE_PAN_H
#define MOBILE_PAN_H

#include "windows.h"

struct MobilePanState {
	float accumulator_x;
	float accumulator_y;
};

typedef void (*MobilePanEmitKey)(void *context, int vk);

static inline void MobilePan_Init(MobilePanState *pan)
{
	if (!pan) {
		return;
	}
	pan->accumulator_x = 0.0f;
	pan->accumulator_y = 0.0f;
}

static inline void MobilePan_Reset(MobilePanState *pan)
{
	MobilePan_Init(pan);
}

static inline void MobilePan_EmitKey(MobilePanEmitKey emit_key, void *context, int vk)
{
	if (emit_key) {
		emit_key(context, vk);
	}
}

static inline void MobilePan_EmitFingerDelta(
	MobilePanState *pan,
	float dx,
	float dy,
	int logical_width,
	int logical_height,
	MobilePanEmitKey emit_key,
	void *context)
{
	static const float threshold = 24.0f;
	if (!pan || logical_width <= 0 || logical_height <= 0) {
		return;
	}

	pan->accumulator_x += dx * (float)logical_width;
	pan->accumulator_y += dy * (float)logical_height;

	while (pan->accumulator_x >= threshold) {
		MobilePan_EmitKey(emit_key, context, VK_LEFT);
		pan->accumulator_x -= threshold;
	}
	while (pan->accumulator_x <= -threshold) {
		MobilePan_EmitKey(emit_key, context, VK_RIGHT);
		pan->accumulator_x += threshold;
	}
	while (pan->accumulator_y >= threshold) {
		MobilePan_EmitKey(emit_key, context, VK_UP);
		pan->accumulator_y -= threshold;
	}
	while (pan->accumulator_y <= -threshold) {
		MobilePan_EmitKey(emit_key, context, VK_DOWN);
		pan->accumulator_y += threshold;
	}
}

static inline void MobilePan_IgnoreMultiGestureCenter(
	MobilePanState *,
	float,
	float,
	int,
	int,
	MobilePanEmitKey,
	void *)
{
}

#endif
