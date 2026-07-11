#include "FIXED.H"
#include "DEFINES.H"
#include "TRIGGERWIDTH.H"

#include <assert.h>

int main(void)
{
	assert(Legacy_Trigger_Byte(-235) == 21);
	assert(Legacy_Trigger_Byte(-246) == 10);
	assert(Legacy_Trigger_Byte(-247) == 9);
	assert(Normalize_Trigger_Data(NEED_SPEECH, -191) == 65);
	assert(Normalize_Trigger_Data(NEED_HOUSE, -247) == 9);
	assert(Normalize_Trigger_Data(NEED_SOUND, -65437) == 99);
	assert(Normalize_Trigger_Data(NEED_NUMBER, 500) == 500);
	return 0;
}
