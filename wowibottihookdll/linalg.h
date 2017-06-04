#pragma once

#include <d3d9.h>
#include "defs.h"
#include "addrs.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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



};

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
};

DWORD get_wow_camera();
int get_wow_proj_matrix(mat4 *m);
int get_wow_view_matrix(mat4 *m);