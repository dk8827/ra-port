#include "FIXED.H"
#include "DEFINES.H"
#include "FACE.H"
#include "RANDOM.H"

#include <stdlib.h>

#define ICON_PIXEL_W 24
#define ICON_LEPTON_W 256
#define CELL_LEPTON_W ICON_LEPTON_W
#define ARRAY_SIZE(x) int(sizeof(x) / sizeof((x)[0]))

class CoordCellTestScenario {
public:
	int RandomNumber(int, int);
};

extern CoordCellTestScenario Scen;
extern unsigned char const Facing16[256];
extern unsigned char const Facing32[256];
extern COORDINATE const AdjacentCoord[FACING_COUNT];
extern CELL const AdjacentCell[FACING_COUNT];
extern char const *NameOverride[25];
extern char const *SystemStrings;
extern char const *DebugStrings;

DirType Desired_Facing256(int, int, int, int);
DirType Desired_Facing8(int, int, int, int);
char *Extract_String(void const *, int);

#include "INLINE.H"

#include <stdio.h>

static int fail(char const *message)
{
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

static int expect_int(int actual, int expected, char const *message)
{
	if (actual != expected) {
		fprintf(stderr, "FAIL: %s: got %d expected %d\n", message, actual, expected);
		return 1;
	}
	return 0;
}

static int expect_coord(COORDINATE actual, COORDINATE expected, char const *message)
{
	if (actual != expected) {
		fprintf(stderr, "FAIL: %s: got 0x%08lx expected 0x%08lx\n", message, (unsigned long)actual, (unsigned long)expected);
		return 1;
	}
	return 0;
}

int main(void)
{
	if (sizeof(CELL_COMPOSITE) != sizeof(CELL)) {
		return fail("CELL_COMPOSITE must stay packed to CELL size");
	}
	if (sizeof(COORD_COMPOSITE) != sizeof(COORDINATE)) {
		return fail("COORD_COMPOSITE must stay packed to COORDINATE size");
	}

	COORDINATE crash_coord = 0x00cf012fUL;
	if (expect_int(Coord_XCell(crash_coord), 1, "diagnostic crash coordinate x cell")) return 1;
	if (expect_int(Coord_YCell(crash_coord), 0, "diagnostic crash coordinate y cell")) return 1;
	if (expect_int(Coord_XLepton(crash_coord), 0x2f, "diagnostic crash coordinate x lepton")) return 1;
	if (expect_int(Coord_YLepton(crash_coord), 0xcf, "diagnostic crash coordinate y lepton")) return 1;

	for (int y = 0; y < MAP_CELL_H; y += 17) {
		for (int x = 0; x < MAP_CELL_W; x += 19) {
			CELL cell = XY_Cell(x, y);
			COORDINATE coord = Cell_Coord(cell);
			if (expect_int(Cell_X(cell), x, "cell x round trip")) return 1;
			if (expect_int(Cell_Y(cell), y, "cell y round trip")) return 1;
			if (expect_int(Coord_XLepton(coord), CELL_LEPTON_W / 2, "cell coord x center")) return 1;
			if (expect_int(Coord_YLepton(coord), CELL_LEPTON_W / 2, "cell coord y center")) return 1;
		}
	}

	if (expect_coord(Coord_Whole(crash_coord), 0x00000100UL, "whole coordinate keeps cells only")) return 1;
	if (expect_coord(Coord_Fraction(crash_coord), 0x00cf002fUL, "fraction coordinate keeps leptons only")) return 1;

	return 0;
}
