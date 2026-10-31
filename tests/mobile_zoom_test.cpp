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
	assert(MobilePinch_EndTapFingers(&pinch, 1200) == 2);
	/* Only the first lift of the pair counts. */
	assert(MobilePinch_EndTapFingers(&pinch, 1210) == 0);
}

static void test_three_finger_tap_is_counted(void)
{
	MobilePinchState pinch;
	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1010);
	MobilePinch_Down(&pinch, 3, 300.0f, 100.0f, 1030);
	assert(MobilePinch_EndTapFingers(&pinch, 1200) == 3);

	/* Lifting every finger starts the count afresh for the next touch. */
	MobilePinch_Up(&pinch, 3);
	MobilePinch_Up(&pinch, 2);
	MobilePinch_Up(&pinch, 1);
	MobilePinch_Down(&pinch, 4, 100.0f, 100.0f, 2000);
	MobilePinch_Down(&pinch, 5, 200.0f, 100.0f, 2010);
	assert(MobilePinch_EndTapFingers(&pinch, 2100) == 2);
}

static void test_slow_or_moving_pair_is_not_a_tap(void)
{
	MobilePinchState pinch;
	float ratio, mid_x, mid_y, mid_dx, mid_dy;

	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1000);
	assert(MobilePinch_EndTapFingers(&pinch, 1000 + MobilePinch_TapMs + 1) == 0);

	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100.0f, 100.0f, 1000);
	MobilePinch_Down(&pinch, 2, 200.0f, 100.0f, 1000);
	MobilePinch_Move(&pinch, 2, 260.0f, 100.0f, &ratio, &mid_x, &mid_y, &mid_dx, &mid_dy);
	assert(MobilePinch_EndTapFingers(&pinch, 1100) == 0);
}

static void test_parallel_drag_preserves_scale_at_both_limits(void)
{
	for (int initial_scale = 1; initial_scale <= 3; ++initial_scale) {
		for (int direction = -1; direction <= 1; direction += 2) {
			for (int first = 0; first <= 1; ++first) {
				MobileZoomState zoom;
				MobileZoom_Init(&zoom);
				MobileZoom_Pinch(&zoom, (float)initial_scale, 320, 200, 640, 400);
				MobilePinchState pinch;
				MobilePinch_Init(&pinch);
				MobilePinch_Down(&pinch, 1, 100, 100, 0, zoom.scale);
				MobilePinch_Down(&pinch, 2, 200, 100, 0, zoom.scale);
				for (int step = 1; step <= 5; ++step) {
					for (int event = 0; event < 2; ++event) {
						int finger = ((first + event) % 2) + 1;
						float scale, x, y, dx, dy, rest_x, rest_y;
						assert(MobilePinch_Move(&pinch, finger, 100 * finger + direction * 10 * step, 100,
							&scale, &x, &y, &dx, &dy));
						MobileZoom_Pinch(&zoom, scale / zoom.scale, x, y, 640, 400);
						MobileZoom_Pan(&zoom, dx, dy, 640, 400, &rest_x, &rest_y);
					}
					assert(approx(zoom.scale, (float)initial_scale));
				}
			}
		}
	}
}

static void test_new_pair_uses_current_scale(void)
{
	MobilePinchState pinch;
	MobilePinch_Init(&pinch);
	MobilePinch_Down(&pinch, 1, 100, 100, 0, 2.0f);
	MobilePinch_Down(&pinch, 2, 200, 100, 0, 2.0f);
	float scale;
	MobilePinch_Move(&pinch, 2, 250, 100, &scale, 0, 0, 0, 0);
	assert(approx(scale, 3.0f));
	MobilePinch_Move(&pinch, 2, 200, 100, &scale, 0, 0, 0, 0);
	assert(approx(scale, 2.0f));
	MobilePinch_Up(&pinch, 2);
	MobilePinch_Down(&pinch, 3, 200, 100, 1000, scale);
	MobilePinch_Move(&pinch, 3, 150, 100, &scale, 0, 0, 0, 0);
	assert(approx(scale, 1.0f));
}

int main(void)
{
	test_parallel_drag_preserves_scale_at_both_limits();
	test_new_pair_uses_current_scale();
	test_quick_still_two_finger_tap_is_recognised();
	test_three_finger_tap_is_counted();
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
