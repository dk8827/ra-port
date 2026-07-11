#ifndef TRIGGERWIDTH_H
#define TRIGGERWIDTH_H

#include <stdint.h>

static inline long Legacy_Trigger_Byte(long value)
{
	return (long)(int8_t)value;
}

static inline long Legacy_Trigger_Word(long value)
{
	return (long)(int16_t)value;
}

static inline long Normalize_Trigger_Data(NeedType need, long value)
{
	switch (need) {
		case NEED_THEME:
		case NEED_MOVIE:
		case NEED_SPEECH:
		case NEED_HOUSE:
		case NEED_SPECIAL:
		case NEED_QUARRY:
		case NEED_BOOL:
			return Legacy_Trigger_Byte(value);

		case NEED_SOUND:
			return Legacy_Trigger_Word(value);

		default:
			return value;
	}
}

#endif
