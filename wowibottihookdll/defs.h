#pragma once

#include <Windows.h>

typedef unsigned long long GUID_t;
typedef unsigned int uint; // looks kinda messy with all the "unsigned int"s

#define MKEY_G 0x47

extern HANDLE glhProcess;

#define DEREF(x) *(uint*)(x)

struct vec3;
inline float dot(const vec3 &a, const vec3 &b);
inline vec3 operator*(float  f, const vec3 &v);

struct vec3 {
	float x, y, z;
	vec3 operator+(const vec3 &p) { return vec3(x + p.x, y + p.y, z + p.z); }
	vec3 operator-(const vec3 &p) { return vec3(x - p.x, y - p.y, z - p.z); }

	float length() const { return sqrt(dot(*this, *this)); };

	vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {};
	vec3() : x(0), y(0), z(0) {};
	vec3 unit() const { 
		float l_recip = 1.0/this->length();
		return l_recip*vec3(*this);
	}
};

inline float dot(const vec3 &a, const vec3 &b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline vec3 operator*(float f, const vec3& v) {
	return vec3(f*v.x, f*v.y, f*v.z);
}

struct point_timestamp {
	vec3 p;
	LARGE_INTEGER ticks;
	point_timestamp() {};
	point_timestamp(float x, float y, float z) : p(x, y, z) {
		QueryPerformanceCounter(&ticks);
	}

	point_timestamp(vec3 v) : p(v) {
		QueryPerformanceCounter(&ticks);
	}
};


inline SIZE_T readAddr(unsigned int mem, void *to, size_t bytes) {
	SIZE_T read;
	ReadProcessMemory(glhProcess, (LPVOID)mem, to, bytes, &read);
	return read;
}

inline SIZE_T writeAddr(unsigned int mem, void *data, size_t bytes) {
	SIZE_T read;
	WriteProcessMemory(glhProcess, (LPVOID)mem, data, bytes, &read);
	return read;
}


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

