#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int CallbackCount = 0;

static void CALLBACK timer_callback(UINT, UINT, DWORD, DWORD, DWORD)
{
	++CallbackCount;
}

static void fail(char const *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}

int main()
{
	// Windows permits fresh critical-section storage to contain arbitrary bytes.
	CRITICAL_SECTION section;
	memset(&section, 0xa5, sizeof(section));
	InitializeCriticalSection(&section);
	if (pthread_mutex_trylock(&section.mutex) != 0) fail("critical section was not initialized");
	if (pthread_mutex_trylock(&section.mutex) != 0) fail("critical section is not recursive");
	LeaveCriticalSection(&section);
	LeaveCriticalSection(&section);
	DeleteCriticalSection(&section);
	if (section.initialized) fail("critical section was not deleted");

	// Future deadlines must remain in the future on LP64 hosts too.
	MMRESULT future = timeSetEvent(10000, 1, timer_callback, 0, TIME_ONESHOT);
	MacMM_PumpTimers();
	timeKillEvent(future);
	if (CallbackCount) fail("timer fired before its deadline");

	MMRESULT timer = timeSetEvent(10, 1, timer_callback, 0, TIME_PERIODIC);
	if (timer == 0) fail("timeSetEvent returned zero");

	for (int index = 0; index < 6; ++index) {
		usleep(10000);
		MacMM_PumpTimers();
	}
	timeKillEvent(timer);

	if (CallbackCount < 2) fail("timer callback did not fire");
	return 0;
}
