#include <mmsystem.h>

#include <string.h>

namespace {

struct MacMMTimerEvent {
	UINT id;
	UINT delay;
	UINT resolution;
	UINT flags;
	DWORD user;
	DWORD next_due;
	LPTIMECALLBACK callback;
	bool active;
};

static MacMMTimerEvent MacMMTimers[16];
static UINT MacMMNextTimerId = 1;
static bool MacMMPumping = false;

static int timer_count(void)
{
	return (int)(sizeof(MacMMTimers) / sizeof(MacMMTimers[0]));
}

} // namespace

MMRESULT timeBeginPeriod(UINT)
{
	return 0;
}

MMRESULT timeEndPeriod(UINT)
{
	return 0;
}

DWORD timeGetTime(void)
{
	return GetTickCount();
}

MMRESULT timeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback, DWORD user, UINT flags)
{
	if (!callback) return 0;

	MacMMTimerEvent *slot = 0;
	for (int index = 0; index < timer_count(); ++index) {
		if (!MacMMTimers[index].active) {
			slot = &MacMMTimers[index];
			break;
		}
	}
	if (!slot) return 0;

	memset(slot, 0, sizeof(*slot));
	slot->id = MacMMNextTimerId++;
	if (MacMMNextTimerId == 0) MacMMNextTimerId = 1;
	slot->delay = delay ? delay : 1;
	slot->resolution = resolution;
	slot->flags = flags;
	slot->user = user;
	slot->callback = callback;
	slot->next_due = GetTickCount() + slot->delay;
	slot->active = true;
	return slot->id;
}

MMRESULT timeKillEvent(UINT id)
{
	for (int index = 0; index < timer_count(); ++index) {
		if (MacMMTimers[index].active && MacMMTimers[index].id == id) {
			MacMMTimers[index].active = false;
			return 0;
		}
	}
	return 0;
}

void MacMM_PumpTimers(void)
{
	if (MacMMPumping) return;
	MacMMPumping = true;

	DWORD now = GetTickCount();
	for (int index = 0; index < timer_count(); ++index) {
		MacMMTimerEvent *timer = &MacMMTimers[index];
		if (!timer->active || !timer->callback) continue;
		if ((LONG)(now - timer->next_due) < 0) continue;

		UINT id = timer->id;
		UINT flags = timer->flags;
		DWORD user = timer->user;
		LPTIMECALLBACK callback = timer->callback;

		if (flags == TIME_PERIODIC) {
			timer->next_due = now + timer->delay;
		} else {
			timer->active = false;
		}

		callback(id, 0, user, 0, 0);
	}

	MacMMPumping = false;
}
