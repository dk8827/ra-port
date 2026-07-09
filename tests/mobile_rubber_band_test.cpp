#include "mobile_rubber_band.h"

#include <assert.h>

static void test_fast_touch_drag_commits_selection_on_release(void)
{
	assert(MobileRubberBand_ShouldCommitOnRelease(0, 1, 100, 120, 112, 120));
	assert(MobileRubberBand_ShouldCommitOnRelease(0, 1, 100, 120, 100, 128));
}

static void test_tap_and_small_motion_do_not_commit_selection_box(void)
{
	assert(!MobileRubberBand_ShouldCommitOnRelease(0, 1, 100, 120, 104, 124));
	assert(!MobileRubberBand_ShouldCommitOnRelease(0, 0, 100, 120, 112, 120));
}

static void test_mobile_candidate_survives_cleared_tentative_state(void)
{
	assert(MobileRubberBand_ShouldCommitCandidateOnRelease(0, 0, 1, 100, 120, 112, 120));
}

static void test_active_rubber_band_uses_existing_release_path(void)
{
	assert(!MobileRubberBand_ShouldCommitOnRelease(1, 1, 100, 120, 112, 120));
}

int main(void)
{
	test_fast_touch_drag_commits_selection_on_release();
	test_tap_and_small_motion_do_not_commit_selection_box();
	test_mobile_candidate_survives_cleared_tentative_state();
	test_active_rubber_band_uses_existing_release_path();
	return 0;
}
