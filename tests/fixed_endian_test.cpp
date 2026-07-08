#include "FIXED.H"

#include <stdio.h>

static int expect_scaled(fixed value, int expected, char const *message)
{
	int actual = value * 256;
	if (actual != expected) {
		fprintf(stderr, "FAIL: %s: got %d expected %d\n", message, actual, expected);
		return 1;
	}
	return 0;
}

int main(void)
{
	if (expect_scaled(fixed(1), 256, "integer one uses 8.8 fixed point scale")) return 1;
	if (expect_scaled(fixed(2), 512, "integer two uses 8.8 fixed point scale")) return 1;
	if (expect_scaled(fixed(1, 4), 64, "fraction constructor uses 8.8 fixed point scale")) return 1;
	if (expect_scaled(fixed("1.5"), 384, "ascii constructor uses 8.8 fixed point scale")) return 1;

	if ((fixed(1) * 10) != 10) {
		fprintf(stderr, "FAIL: fixed one should preserve integer multiplication\n");
		return 1;
	}

	return 0;
}
