#pragma once

#include "linalg.h"

template <typename T, int S>
struct heaparray {
	std::array<T, S>* p;
	heaparray() : p(new std::array<T, S>()) {}
	~heaparray() { delete p; }
	T& operator[](int flatindex) {
		return (*p)[flatindex];
	}
	std::array<T, S>& operator*() {
		return *p;
	}

	std::size_t size() const {
		return p->size();
	}

	auto begin() {
		return p->begin();
	}

	auto end() {
		return p->end();
	}

	constexpr T* get() {
		return &(*p)[0];
	}
};

extern "C" bool astar(
    const float* weights, const int h, const int w,
    const int start, const int goal, bool diag_ok,
    int* paths);


struct line_segment {
    vec2_t start;
    vec2_t end;
    float length() const {
        float dx = end.x - start.x;
        float dy = end.y - end.y;
        return sqrt(dx * dx + dy * dy);
    }
    line_segment(vec2_t s, vec2_t e) : start(s), end(e) {}
};

struct circle {
    vec2_t center;
    float radius;
    circle(vec2_t c, float r) : center(c), radius(r) {}
};

bool intersection(const line_segment& ln, const circle& c);
bool inside(const vec2_t& p, const circle& crc);

constexpr bool operator==(const circle& a, const circle& b) {
    return a.center.x == b.center.x && a.radius == b.radius;
}