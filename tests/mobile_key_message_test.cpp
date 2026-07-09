#include "mobile_key_message.h"
#include "windows.h"

#include <assert.h>

static void test_unmodified_pan_message_is_recognized(void)
{
	assert(MobileKeyMessage_IsUnmodified(WM_KEYDOWN, MOBILE_KEY_UNMODIFIED_LPARAM));
	assert(MobileKeyMessage_IsUnmodified(WM_KEYUP, MOBILE_KEY_UNMODIFIED_LPARAM));
	assert(!MobileKeyMessage_IsUnmodified(WM_MOUSEMOVE, MOBILE_KEY_UNMODIFIED_LPARAM));
	assert(!MobileKeyMessage_IsUnmodified(WM_KEYDOWN, 0));
}

static void test_unmodified_pan_message_filters_host_modifiers(void)
{
	assert(MobileKeyMessage_FilterKeyState(1, VK_SHIFT, (short)0x8000) == 0);
	assert(MobileKeyMessage_FilterKeyState(1, VK_CONTROL, (short)0x8000) == 0);
	assert(MobileKeyMessage_FilterKeyState(1, VK_MENU, (short)0x8000) == 0);
	assert(MobileKeyMessage_FilterKeyState(1, VK_CAPITAL, (short)0x0009) == 0);
	assert(MobileKeyMessage_FilterKeyState(1, VK_NUMLOCK, (short)0x0009) == 0);
}

static void test_unmodified_pan_message_preserves_non_modifier_keys(void)
{
	assert(MobileKeyMessage_FilterKeyState(1, VK_DOWN, (short)0x8000) == (short)0x8000);
	assert(MobileKeyMessage_FilterKeyState(0, VK_SHIFT, (short)0x8000) == (short)0x8000);
}

int main(void)
{
	test_unmodified_pan_message_is_recognized();
	test_unmodified_pan_message_filters_host_modifiers();
	test_unmodified_pan_message_preserves_non_modifier_keys();
	return 0;
}
