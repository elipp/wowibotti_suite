#pragma once

#include <Windows.h>
#include <cstdio>

class Timer {

	static double cpu_freq;	// in kHz
	
	static bool initialized;

	__int64 counter_start;
	__int64 get();

public:

	static bool init();
	
	void start();

	inline double get_s() {
		return double(get() - counter_start) / Timer::cpu_freq;
	}

	inline double get_ms() {
		return double(1000 * get_s());
	}

	inline double get_us() {
		return double(1000000 * get_s());
	}

	Timer() {
		if (!Timer::initialized) {
			Timer::init();
		}
		counter_start = 0;

	}

};

