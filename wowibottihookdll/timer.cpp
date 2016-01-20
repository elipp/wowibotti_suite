#include "timer.h"

bool Timer::init() {
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li)) {
		printf("Timer initialization failed.\n");
		return false;
	}
	cpu_freq = double(li.QuadPart);	// in Hz. this is subject to dynamic frequency scaling, though
	
	Timer::initialized = 1;
	return true;
}

void Timer::start() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	counter_start = li.QuadPart;
}

__int64 Timer::get() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

bool Timer::initialized = false;
double Timer::cpu_freq = 0;
