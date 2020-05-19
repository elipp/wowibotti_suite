#pragma once

#include <Windows.h>
#include <cstdio>

// TODO use std::chrono instead

class Timer {

	static double cpu_freq;	// in kHz
	
	static bool initialized;

	__int64 counter_start;
	__int64 get() const;

public:

	static bool init();
	
	void start();

	inline double get_s() const {
		return double(get() - counter_start) / Timer::cpu_freq;
	}

	inline double get_ms() const {
		return double(1000 * get_s());
	}

	inline double get_us() const {
		return double(1000000 * get_s());
	}

	Timer() {
		if (!Timer::initialized) {
			Timer::init();
		}
		counter_start = 0;

	}

};

struct timer_interval_t {
	Timer t;
	int interval;

	int passed() {
		return t.get_ms() > interval;
	}

	void reset() {
		t.start();
	}

	timer_interval_t(int interval_ms) : interval(interval_ms) {
		t.init();
		t.start();
	}
};

