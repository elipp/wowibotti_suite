#include "timer.h"
#include "defs.h"

double get_time_difference_s(const time_point &now, const time_point &then) {
    duration<double> took = duration_cast<duration<double>>(now - then);
    return took.count();
}

double get_time_difference_ms(const time_point &now, const time_point &then) {
    return 1000.0 * get_time_difference_s(now, then);
}

double get_time_from_s(const time_point &then) {
    return get_time_difference_s(high_resolution_clock::now(), then);
}

double get_time_from_ms(const time_point &then) {
    return get_time_difference_ms(high_resolution_clock::now(), then);
}

void Timer::start() {
	init_time = high_resolution_clock::now();	
}

double Timer::get_s() const {
	return get_time_from_s(init_time);
}

double Timer::get_ms() const {
	return get_time_from_ms(init_time);
}

double Timer::get_us() const {
	return 1000.0 * get_time_from_ms(init_time);
}

Timer::Timer() {
	start();
}

int timer_interval_t::passed() const {
	return t.get_ms() > interval_ms;
}

void timer_interval_t::reset() {
	t.start();
}

timer_interval_t::timer_interval_t(double i_ms) : interval_ms(i_ms) {
	t.start();
}

double timer_interval_t::remaining_ms() const {
	return interval_ms - t.get_ms();
}