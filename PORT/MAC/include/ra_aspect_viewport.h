#ifndef RA_ASPECT_VIEWPORT_H
#define RA_ASPECT_VIEWPORT_H

struct RAAspectViewport {
	int x;
	int y;
	int w;
	int h;
};

static inline int RA_ClampInt(int value, int min_value, int max_value)
{
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static inline RAAspectViewport RA_CalculateAspectViewport(int source_w, int source_h, int target_w, int target_h)
{
	RAAspectViewport viewport = {0, 0, 0, 0};
	if (source_w <= 0 || source_h <= 0 || target_w <= 0 || target_h <= 0) {
		return viewport;
	}

	if ((long long)target_w * source_h > (long long)target_h * source_w) {
		viewport.h = target_h;
		viewport.w = (int)(((long long)source_w * target_h) / source_h);
		if (viewport.w < 1) viewport.w = 1;
		viewport.x = (target_w - viewport.w) / 2;
		viewport.y = 0;
	} else {
		viewport.w = target_w;
		viewport.h = (int)(((long long)source_h * target_w) / source_w);
		if (viewport.h < 1) viewport.h = 1;
		viewport.x = 0;
		viewport.y = (target_h - viewport.h) / 2;
	}
	return viewport;
}

static inline int RA_MapViewportPoint(
	RAAspectViewport viewport,
	int logical_w,
	int logical_h,
	int screen_x,
	int screen_y,
	int *logical_x,
	int *logical_y)
{
	if (viewport.w <= 0 || viewport.h <= 0 || logical_w <= 0 || logical_h <= 0) {
		if (logical_x) *logical_x = 0;
		if (logical_y) *logical_y = 0;
		return 0;
	}

	int right = viewport.x + viewport.w;
	int bottom = viewport.y + viewport.h;
	int inside = screen_x >= viewport.x && screen_x < right && screen_y >= viewport.y && screen_y < bottom;

	int clamped_x = RA_ClampInt(screen_x, viewport.x, right - 1);
	int clamped_y = RA_ClampInt(screen_y, viewport.y, bottom - 1);
	int mapped_x = (int)(((long long)(clamped_x - viewport.x) * logical_w) / viewport.w);
	int mapped_y = (int)(((long long)(clamped_y - viewport.y) * logical_h) / viewport.h);

	if (mapped_x >= logical_w) mapped_x = logical_w - 1;
	if (mapped_y >= logical_h) mapped_y = logical_h - 1;
	if (logical_x) *logical_x = mapped_x;
	if (logical_y) *logical_y = mapped_y;
	return inside;
}

#endif
