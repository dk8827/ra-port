#ifndef MOBILE_RUBBER_BAND_H
#define MOBILE_RUBBER_BAND_H

static inline int MobileRubberBand_Abs(int value)
{
	return value < 0 ? -value : value;
}

static inline int MobileRubberBand_ShouldCommitOnRelease(
		int is_rubber_band,
		int is_tentative,
		int start_x,
	int start_y,
	int release_x,
	int release_y)
{
	static const int rubber_band_slop = 4;
	if (is_rubber_band || !is_tentative) {
		return 0;
	}
	return MobileRubberBand_Abs(release_x - start_x) > rubber_band_slop ||
		MobileRubberBand_Abs(release_y - start_y) > rubber_band_slop;
}

static inline int MobileRubberBand_ShouldCommitCandidateOnRelease(
		int is_rubber_band,
		int is_tentative,
		int is_mobile_candidate,
		int start_x,
		int start_y,
		int release_x,
		int release_y)
{
	return MobileRubberBand_ShouldCommitOnRelease(
		is_rubber_band,
		is_tentative || is_mobile_candidate,
		start_x,
		start_y,
		release_x,
		release_y);
}

#endif
