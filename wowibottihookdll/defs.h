#pragma once

#define WIN32_LEAN_AND_MEAN

#include <optional>
#include <Windows.h>
#include <d3d9.h>
#include <string>
#include <vector>

extern HANDLE glhProcess;

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


#ifdef _DEBUG
#define DEBUG_CONSOLE
#endif

//#define DEBUG_CONSOLE
//#define DEBUG_LOG

#ifdef DEBUG_CONSOLE
#define PRINT(...) printf(__VA_ARGS__)
#endif

#ifdef DEBUG_LOG
#define PRINT(...) do{\
FILE *fp;\
fopen_s(&fp, "log.txt", "a");\
fprintf(fp, "[%d] ", GetTickCount());\
fprintf(fp, __VA_ARGS__);\
fclose(fp); }\
while(0)
#endif

#if !defined(DEBUG_CONSOLE) && !defined(DEBUG_LOG)
#define PRINT(...) 0
#endif



enum {
	CTM_HUNTER_AIMED = 0x0,
	CTM_FACE = 0x2,
	CTM_FOLLOW = 0x3,
	CTM_MOVE = 0x4,
	CTM_TALK_NPC = 0x5,
	CTM_LOOT = 0x6,
	CTM_MOVE_AND_ATTACK = 0xA,
	CTM_DONE = 0xD // methinks
};

extern HWND wow_hWnd;

template <typename T> T DEREF(DWORD addr) {
	return *(T*)(addr);
};

struct vec2;
struct vec3;

inline float dot(const vec3 &a, const vec3 &b);

inline vec3 operator*(float f, const vec3 &v);

struct vec3 {
	union {
		float data[3];
		struct {
			float x, y, z;
		};
	};

	vec3 operator+(const vec3 &p) const { return vec3(x + p.x, y + p.y, z + p.z); }
	vec3 operator-(const vec3 &p) const { return vec3(x - p.x, y - p.y, z - p.z); }

	float length() const { return sqrt(dot(*this, *this)); };

	vec3 rotated_2d(float angle);

	constexpr vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {};
	constexpr vec3() : x(0), y(0), z(0) {};
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


template <typename T>
inline void readAddr(unsigned int mem, T* to) {
	memcpy(to, (const void*)mem, sizeof(T));
}


template <typename T>
inline SIZE_T writeAddr(unsigned int mem, const T &data) {
	SIZE_T written;
	WriteProcessMemory(glhProcess, (LPVOID)mem, &data, sizeof(T), &written);
	return written;
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
		if (end - start > 0) {  // just ignore zero-length strings.
			tokens.push_back(std::string(str, start, end - start));
		}
	}
}


inline int find_stuff_between(const std::string &in_str, char cbegin, char cend, std::string &out_str) {
	unsigned d_begin = in_str.find(cbegin);
	unsigned d_end = in_str.find_last_of(cend);

	if (d_begin == std::string::npos || d_end == std::string::npos || d_begin == d_end) {
		PRINT("find_stuff_between: couldn't find content between delimiters '%c' and '%c' in string \"%s\".\n", cbegin, cend, in_str.c_str());
		return 0;
	}

	out_str = in_str.substr(d_begin + 1, d_end - d_begin - 1);

	return 1;

}

inline std::optional<GUID_t> convert_str_to_GUID(const std::string &GUID_str) {
	std::string GUID_numstr(GUID_str.substr(2, 16)); // better make a copy of it. the GUID_str still has the "0x" prefix in it 

	char *end;
	GUID_t GUID = strtoull(GUID_numstr.c_str(), &end, 16);

	if (end != GUID_numstr.c_str() + GUID_numstr.length()) {
		PRINT("[WARNING]: convert_str_to_GUID: couldn't wholly convert GUID string argument (strtoull(\"%s\", &end, 16) failed, bailing out\n", GUID_numstr.c_str());
		return std::nullopt;
	}

	return GUID;
}

inline std::string convert_GUID_to_str(GUID_t g) {

	char buf[32];
	sprintf_s(buf, "0x%16llX", g);

	return std::string(buf);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

