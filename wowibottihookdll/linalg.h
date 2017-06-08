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