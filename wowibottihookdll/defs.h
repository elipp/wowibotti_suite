#pragma once

#include <Windows.h>
#include <string>
#include <vector>

typedef unsigned long long GUID_t;
typedef unsigned int uint; // looks kinda messy with all the "unsigned int"s

#define MKEY_G 0x47

inline LPARAM get_KEYDOWN_LPARAM(int vk) {
	LPARAM lparam;
	UINT scan = MapVirtualKey(vk, 0);
	lparam = 0x00000001 | (LPARAM)(scan << 16);         // Scan code, repeat=1
	return lparam;
}

inline LPARAM get_KEYUP_LPARAM(int vk) {
	LPARAM lparam = get_KEYDOWN_LPARAM(vk);
	lparam = lparam | 0xC0000000; // the last 2 bits are always 1 for WM_KEYUP
	return lparam;
}


enum {
	CTM_HUNTER_AIMED = 0x0,
	CTM_FOLLOW = 0x3,
	CTM_MOVE = 0x4,
	CTM_TALK_NPC = 0x5,
	CTM_LOOT = 0x6,
	CTM_MOVE_AND_ATTACK = 0xA,
	CTM_DONE = 0xD // methinks
};

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

inline SIZE_T writeAddr(unsigned int mem, const void *data, size_t bytes) {
	SIZE_T read;
	WriteProcessMemory(glhProcess, (LPVOID)mem, data, bytes, &read);
	return read;
}

inline void tokenize_string(const std::string& str, const std::string& delim, std::vector<std::string>& tokens) {
	size_t start, end = 0;
	while (end < str.size()) {
		start = end;
		while (start < str.size() && (delim.find(str[start]) != std::string::npos)) {
			start++;  // skip initial whitespace
		}
		end = start;
		while (end < str.size() && (delim.find(str[end]) == std::string::npos)) {
			end++; // skip to end of word
		}
		if (end - start != 0) {  // just ignore zero-length strings.
			tokens.push_back(std::string(str, start, end - start));
		}
	}
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

