#include "mobile_zoom.h"

#include <assert.h>

static int approx(float value, float expected)
{
	float difference = value - expected;
	return difference > -0.01f && difference < 0.01f;
}

static void test_unzoomed_maps_points_unchanged(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);

	MobileZoomRect rect = MobileZoom_SourceRect(&zoom, 640, 400);
	assert(rect.x == 0 && rect.y == 0 && rect.w == 640 && rect.h == 400);

	int x = 0;
	int y = 0;
	MobileZoom_MapPoint(&zoom, 640, 400, 123, 321, &x, &y);
	assert(x == 123 && y == 321);
}

static void test_pinch_keeps_focus_point_in_place(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);

	MobileZoom_Pinch(&zoom, 2.0f, 160.0f, 100.0f, 640, 400);
	assert(approx(zoom.scale, 2.0f));

	int x = 0;
	int y = 0;
	MobileZoom_MapPoint(&zoom, 640, 400, 160, 100, &x, &y);
	assert(x == 160 && y == 100);

	MobileZoomRect rect = MobileZoom_SourceRect(&zoom, 640, 400);
	assert(rect.w == 320 && rect.h == 200);
	assert(rect.x == 80 && rect.y == 50);
}

static void test_scale_is_clamped(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);

	MobileZoom_Pinch(&zoom, 10.0f, 320.0f, 200.0f, 640, 400);
	assert(approx(zoom.scale, MobileZoom_MaxScale));

	MobileZoom_Pinch(&zoom, 0.01f, 320.0f, 200.0f, 640, 400);
	assert(approx(zoom.scale, 1.0f));
	assert(!MobileZoom_IsZoomed(&zoom));
}

static void test_pinch_near_edge_stays_inside_frame(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);

	MobileZoom_Pinch(&zoom, 2.0f, 639.0f, 399.0f, 640, 400);
	MobileZoomRect rect = MobileZoom_SourceRect(&zoom, 640, 400);
	assert(rect.x >= 0 && rect.y >= 0);
	assert(rect.x + rect.w <= 640 && rect.y + rect.h <= 400);
	assert(rect.x == 320 && rect.y == 200);
}

static void test_unzoomed_pan_passes_everything_through(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);

	float rest_x = 0.0f;
	float rest_y = 0.0f;
	MobileZoom_Pan(&zoom, 12.0f, -7.0f, 640, 400, &rest_x, &rest_y);
	assert(approx(rest_x, 12.0f) && approx(rest_y, -7.0f));
}

static void test_zoomed_pan_moves_view_then_overflows(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);
	MobileZoom_Pinch(&zoom, 2.0f, 320.0f, 200.0f, 640, 400);

	/* Fingers move right by 40 screen units: the view moves left by 20 frame pixels, nothing left over. */
	float rest_x = 0.0f;
	float rest_y = 0.0f;
	MobileZoom_Pan(&zoom, 40.0f, 0.0f, 640, 400, &rest_x, &rest_y);
	assert(approx(zoom.center_x, 300.0f));
	assert(approx(rest_x, 0.0f) && approx(rest_y, 0.0f));

	/* The view can only move 140 more frame pixels left (to center 160); the rest scrolls the map. */
	MobileZoom_Pan(&zoom, 400.0f, 0.0f, 640, 400, &rest_x, &rest_y);
	assert(approx(zoom.center_x, 160.0f));
	assert(approx(rest_x, 120.0f));
}

static void test_snap_resets_tiny_zoom(void)
{
	MobileZoomState zoom;
	MobileZoom_Init(&zoom);
	MobileZoom_Pinch(&zoom, 1.02f, 320.0f, 200.0f, 640, 400);
	assert(MobileZoom_IsZoomed(&zoom));
	MobileZoom_SnapIfNearlyUnzoomed(&zoom);
	assert(!MobileZoom_IsZoomed(&zoom));
}

static void test_pinch_reports_spread_and_midpoint(void)
{
	MobilePinchState pinch;
	MobilePinch_Init(&pinch);

	float ratio = 0.0f;
	float mid_x = 0.0f;
	float mid_y = 0.0f;
	float mid_dx = 0.0f;
	float mid_dy = 0.0f;

	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	assert(!MobilePinch_Move(&pinch, 1, 100.0f, 100.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy));

	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1000);
	assert(MobilePinch_BothActive(&pinch));

	/* Finger 2 moves 100 further away: distance 100 -> 200, midpoint 150 -> 200. */
	assert(MobilePinch_Move(&pinch, 2, 300.0f, 100.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy));
	assert(approx(ratio, 2.0f));
	assert(approx(mid_x, 150.0f) && approx(mid_y, 100.0f));
	assert(approx(mid_dx, 50.0f) && approx(mid_dy, 0.0f));

	/* Both fingers move down together: no spread change, midpoint follows. */
	assert(MobilePinch_Move(&pinch, 1, 100.0f, 130.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy));
	assert(MobilePinch_Move(&pinch, 2, 300.0f, 130.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy));
	assert(approx(mid_dy, 15.0f));

	MobilePinch_Up(&pinch, 1);
	assert(!MobilePinch_BothActive(&pinch));
	assert(!MobilePinch_Move(&pinch, 2, 310.0f, 130.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy));
}

static void test_quick_still_two_finger_tap_is_recognised(void)
{
	MobilePinchState pinch;
	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1020);
	assert(MobilePinch_EndIsTap(&pinch, 1200));
	/* Only the first lift of the pair counts. */
	assert(!MobilePinch_EndIsTap(&pinch, 1210));
}

static void test_slow_or_moving_pair_is_not_a_tap(void)
{
	MobilePinchState pinch;
	float ratio, mid_x, mid_y, mid_dx, mid_dy;

	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1000);
	assert(!MobilePinch_EndIsTap(&pinch, 1000 + MobilePinch_TapMs + 1));

	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1000);
	MobilePinch_Move(&pinch, 2, 260.0f, 100.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy);
	assert(!MobilePinch_EndIsTap(&pinch, 1100));
}

int main(void)
{
	test_quick_still_two_finger_tap_is_recognised();
	test_slow_or_moving_pair_is_not_a_tap();
	test_unzoomed_maps_points_unchanged();
	test_pinch_keeps_focus_point_in_place();
	test_scale_is_clamped();
	test_pinch_near_edge_stays_inside_frame();
	test_unzoomed_pan_passes_everything_through();
	test_zoomed_pan_moves_view_then_overflows();
	test_snap_resets_tiny_zoom();
	test_pinch_reports_spread_and_midpoint();
	return 0;
}
