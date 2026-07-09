#ifndef MOBILE_KEY_MESSAGE_H
#define MOBILE_KEY_MESSAGE_H

#include "windows.h"

#define MOBILE_KEY_UNMODIFIED_LPARAM ((LPARAM)0x52414D50L)

static inline int MobileKeyMessage_IsUnmodified(UINT message, LPARAM lparam)
{
	return (message == WM_KEYDOWN || message == WM_KEYUP ||
			message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) &&
		lparam == MOBILE_KEY_UNMODIFIED_LPARAM;
}

static inline int MobileKeyMessage_IsModifier(int key)
{
	key &= 0xFF;
	return key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU ||
		key == VK_CAPITAL || key == VK_NUMLOCK;
}

static inline short MobileKeyMessage_FilterKeyState(int unmodified, int key, short state)
{
	if (unmodified && MobileKeyMessage_IsModifier(key)) {
		return 0;
	}
	return state;
}

#endif
