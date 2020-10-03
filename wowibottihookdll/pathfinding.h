#pragma once

#include <array>
#include <optional>
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
	constexpr vec2_t diff() const { return end - start; }
	line_segment translated(vec2_t by) { return line_segment(start + by, end + by); }
};

struct tangent_points {
	vec2_t left;
	vec2_t right;
};

struct tangent_paths {
	std::array<vec2_t, 2> left;
	std::array<vec2_t, 2> right;
};

struct circle {
    vec2_t center;
    float radius;
    circle(vec2_t c, float r) : center(c), radius(r) {}
	circle(float x, float y, float r) : center(vec2_t(x, y)), radius(r) {}
	tangent_points find_tangent_points(const vec2_t &from) const {
		vec2_t d = from - center;
		float alpha = acosf(radius/length(d));

		vec2_t r1 = unit(rotate(d, alpha));
		vec2_t r2 = unit(rotate(d, -alpha));
		vec2_t cwr = center + radius * r1;
		vec2_t ccwr = center + radius * r2;
		return {cwr, ccwr};
	}

	tangent_paths get_tangent_paths(const line_segment &ls) const {
		tangent_paths r;
		auto b = find_tangent_points(ls.start);
		auto e = find_tangent_points(ls.end);
		r.left = {b.left, e.right};
		r.right = {b.right, e.left};
		return r;
	}
};

template <typename T>
constexpr bool BETWEEN(T x, float min, float max) { return static_cast<float>(x) >= min && static_cast<float>(x) <= max; }

struct intersect_data {
	bool valid1, valid2;
    float t1, t2;
    vec2_t i1, i2;
};

std::optional<intersect_data> intersection(const line_segment& ln, const circle& crc);

bool intersection(const circle &a, const circle &b);

bool inside(const vec2_t& p, const circle& crc);

constexpr bool operator==(const circle& a, const circle& b) {
    return a.center.x == b.center.x && a.radius == b.radius;
}