#include "ra_aspect_viewport.h"

#include <stdio.h>

static int expect_int(int actual, int expected, char const *message)
{
	if (actual != expected) {
		fprintf(stderr, "FAIL: %s: got %d expected %d\n", message, actual, expected);
		return 1;
	}
	return 0;
}

int main(void)
{
	RAAspectViewport tablet = RA_CalculateAspectViewport(640, 400, 2550, 1490);
	if (expect_int(tablet.x, 83, "wide tablet viewport x")) return 1;
	if (expect_int(tablet.y, 0, "wide tablet viewport y")) return 1;
	if (expect_int(tablet.w, 2384, "wide tablet viewport width")) return 1;
	if (expect_int(tablet.h, 1490, "wide tablet viewport height")) return 1;

	RAAspectViewport phone = RA_CalculateAspectViewport(640, 400, 1920, 1080);
	if (expect_int(phone.x, 96, "wide phone viewport x")) return 1;
	if (expect_int(phone.y, 0, "wide phone viewport y")) return 1;
	if (expect_int(phone.w, 1728, "wide phone viewport width")) return 1;
	if (expect_int(phone.h, 1080, "wide phone viewport height")) return 1;

	RAAspectViewport tall = RA_CalculateAspectViewport(640, 400, 1200, 1000);
	if (expect_int(tall.x, 0, "tall viewport x")) return 1;
	if (expect_int(tall.y, 125, "tall viewport y")) return 1;
	if (expect_int(tall.w, 1200, "tall viewport width")) return 1;
	if (expect_int(tall.h, 750, "tall viewport height")) return 1;

	int logical_x = -1;
	int logical_y = -1;
	if (!RA_MapViewportPoint(tablet, 640, 400, 83, 0, &logical_x, &logical_y)) {
		fprintf(stderr, "FAIL: tablet top-left should map inside viewport\n");
		return 1;
	}
	if (expect_int(logical_x, 0, "tablet top-left logical x")) return 1;
	if (expect_int(logical_y, 0, "tablet top-left logical y")) return 1;

	if (!RA_MapViewportPoint(tablet, 640, 400, 2466, 1489, &logical_x, &logical_y)) {
		fprintf(stderr, "FAIL: tablet bottom-right should map inside viewport\n");
		return 1;
	}
	if (expect_int(logical_x, 639, "tablet bottom-right logical x")) return 1;
	if (expect_int(logical_y, 399, "tablet bottom-right logical y")) return 1;

	if (RA_MapViewportPoint(tablet, 640, 400, 82, 100, &logical_x, &logical_y)) {
		fprintf(stderr, "FAIL: left pillarbox should report outside viewport\n");
		return 1;
	}
	if (expect_int(logical_x, 0, "left pillarbox clamps to logical x")) return 1;
	if (expect_int(logical_y, 26, "left pillarbox preserves logical y")) return 1;

	return 0;
}
