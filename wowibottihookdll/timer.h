#pragma once

#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::duration_cast;

typedef high_resolution_clock::time_point time_point;

double get_time_difference_s(const time_point &now, const time_point &then);
double get_time_difference_ms(const time_point &now, const time_point &then);

double get_time_from_s(const time_point &then);
double get_time_from_ms(const time_point &then);

class Timer {
	time_point init_time;
public:
	void start();
	double get_s() const;
	double get_ms() const;
	double get_us() const;
	Timer();
};

struct timer_interval_t {
	Timer t;
	double interval_ms;

	int passed() const;
	void reset();
	double remaining_ms() const;

	timer_interval_t(double i_ms);
};

