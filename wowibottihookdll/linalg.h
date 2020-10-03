#pragma once

#include <d3d9.h>
#include "defs.h"
#include "addrs.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

typedef D3DMATRIX mat4;

typedef struct vec4 {
	union {
		struct {
			float x, y, z, w;
		};
		float c[4];
	};

	vec4() {}
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

} vec4;

vec4 operator-(const vec4 &v);

float vec4_dot(const vec4 &a, const vec4 &b);


mat4 mat4_construct(
	float m11, float m12, float m13, float m14,
	float m21, float m22, float m23, float m24,
	float m31, float m32, float m33, float m34,
	float m41, float m42, float m43, float m44);

void mat4_identity(mat4 *m);
void mat4_ortho_lh(mat4 *m, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf);

mat4 mat4_translate(const vec4 &v);
mat4 mat4_translate(const vec3 &v);

vec4 operator*(const mat4 &m, const vec4 &v);
mat4 operator*(const mat4 &m1, const mat4 &m2);


void mat4_dump(const mat4 &m);
void mat4_dump_raw(const mat4 &m);

void vec4_dump(const vec4 &v);

typedef struct wow_camera_t {
	DWORD unk1, unk2;
	float x, y, z;
	float rot[3][3];
	float zNear, zFar;
	float fov;
	float aspect;
} wow_camera_t;

DWORD get_wow_camera();
int get_wow_proj_matrix(mat4 *m);
int get_wow_view_matrix(mat4 *m);

void dump_glm_mat4(const glm::mat4 &m);
void dump_glm_mat4_raw(const glm::mat4 &m);
void dump_glm_vec4(const glm::vec4 &v);

void get_wow_rot_raw(float *out);
glm::mat4 get_wow_rot();
float get_wow_rot_angle();
glm::mat4 get_corresponding_RH_rot();

glm::vec4 wow2glm(const glm::vec4 v);
glm::vec3 wow2glm(const glm::vec3 v);
vec3 wow2glm(const vec3& v);

glm::vec4 glm2wow(const glm::vec4 v);
glm::vec3 glm2wow(const glm::vec3 v);
vec3 glm2wow(const vec3& v);

void set_wow_rot(const glm::mat4 &rot);

typedef struct vec2_t {
	float x; float y;
	constexpr vec2_t(float xx, float yy) : x(xx), y(yy) {}
	constexpr vec2_t() : x(0), y(0) {}
} vec2_t;

constexpr vec2_t vec2(float x, float y) {
	return { x, y };
}

constexpr vec2_t operator+(const vec2_t& a, const vec2_t& b) {
	return { a.x + b.x, a.y + b.y };
}

constexpr vec2_t operator-(const vec2_t& a, const vec2_t& b) {
	return { a.x - b.x, a.y - b.y };
}

constexpr vec2_t operator-(const vec2_t &a) {
	return { -a.x, -a.y };
}

constexpr vec2_t operator*(float d, const vec2_t& v) {
	return { d * v.x, d * v.y };
}

constexpr bool operator==(const vec2_t& a, const vec2_t& b) {
	return a.x == b.x && a.y == b.y;
}


vec2_t unit(const vec2_t& v);
vec2_t perp(const vec2_t& v);

vec2_t rotate(const vec2_t &v, float angle);

vec2_t rotate90_cw(const vec2_t& v);
vec2_t rotate90_ccw(const vec2_t& v);

vec2_t avg(const vec2_t& a, const vec2_t& b);

float angle_between(const vec2_t &a, const vec2_t &b);
float cw_angle_between(const vec2_t &a, const vec2_t &b);
float ccw_angle_between(const vec2_t &a, const vec2_t &b); 

inline float length(const vec2_t& v) {
	return sqrt(v.x * v.x + v.y * v.y);
}

constexpr vec2_t unitvec2_from_rot(float rot) {
	return vec2_t(std::cos(rot), std::sin(rot));
}

inline constexpr float dot(const vec2_t& a, const vec2_t& b) {
	return a.x * b.x + a.y * b.y;
}

typedef struct tri_t {
	vec2_t v[3];
} tri_t;

typedef struct vec2i_t {
	int x; int y;
} vec2i_t;

typedef struct pixinfo_t {
	vec2i_t pos;
	BYTE value;
} pixinfo_t;
