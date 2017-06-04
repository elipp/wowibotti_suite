#include "linalg.h"

vec4 operator-(const vec4 &v) {
	return vec4(-v.x, -v.y, -v.z, 1); // don't negate w for now
}


static vec4 mat4_getcolumn(const mat4 &m, int column) {
	vec4 r;

	for (int i = 0; i < 4; ++i) {
		r.c[i] = m.m[i][column];
	}

	return r;
}

static vec4 mat4_getrow(const mat4 &m, int row) {
	vec4 r;

	for (int i = 0; i < 4; ++i) {
		r.c[i] = m.m[row][i];
	}

	return r;
}

float vec4_dot(const vec4 &a, const vec4 &b) {
	return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}


mat4 mat4_construct(
	float m11, float m12, float m13, float m14,
	float m21, float m22, float m23, float m24,
	float m31, float m32, float m33, float m34,
	float m41, float m42, float m43, float m44) {

	mat4 m;
	memset(&m, 0x0, sizeof(m));

	m._11 = m11;
	m._12 = m12;
	m._13 = m13;
	m._14 = m14;

	m._21 = m21;
	m._22 = m22;
	m._23 = m23;
	m._24 = m24;

	m._31 = m31;
	m._32 = m32;
	m._33 = m33;
	m._34 = m34;

	m._41 = m41;
	m._42 = m42;
	m._43 = m43;
	m._44 = m44;

	return m;
}

void mat4_identity(mat4 *m) {

	memset(m, 0x0, sizeof(mat4));

	m->_11 = 1;
	m->_22 = 1;
	m->_33 = 1;
	m->_44 = 1;
}

void mat4_ortho_lh(mat4 *m, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf) {

	memset(m, 0x0, sizeof(mat4));
	m->_11 = 2.0 / w;
	m->_22 = 2.0 / h;
	m->_33 = 1.0 / (zf - zn);
	m->_44 = 1.0;
	m->_34 = zn / (zn - zf);

}

mat4 mat4_translate(const vec4 &v) {
	mat4 m;
	mat4_identity(&m);

	m._14 = v.x;
	m._24 = v.y;
	m._34 = v.z;
	m._44 = v.w;

	return m;
}

mat4 mat4_translate(const vec3 &v) {
	mat4 m;
	mat4_identity(&m);

	m._14 = v.x;
	m._24 = v.y;
	m._34 = v.z;

	return m;
}

int get_wow_proj_matrix(mat4 *m) {
	memset(m, 0, sizeof(m));

	wow_camera_t *c = (wow_camera_t*)get_wow_camera();

	if (!c) return 0;

	float ys = 1.0 / tan(c->fov / 2.0);
	float xs = ys / c->aspect;

	float &n = c->zNear;
	float &f = c->zFar;

	*m = mat4_construct(
		xs, 0, 0, 0,
		0, ys, 0, 0,
		0, 0, (f + n) / (n - f), -1,
		0, 0, 2*f*n / (n - f), 0);

	return 1;

}

int get_wow_view_matrix(mat4 *m) {

	wow_camera_t *c = (wow_camera_t*)get_wow_camera();
	if (!c) return 0;

	D3DMATRIX pos = mat4_construct(
		1, 0, 0, -c->x,
		0, 1, 0, -c->y,
		0, 0, 1, -c->z,
		0, 0, 0, 1);

	float(&r)[3][3] = c->rot;

	D3DMATRIX rot = mat4_construct(
		r[0][0], r[0][1], r[0][2], 0,
		r[1][0], r[1][1], r[1][2], 0,
		r[2][0], r[2][1], r[2][2], 1,
		0, 0, 0, 1);

	*m = rot * pos;
	mat4_dump(*m);
	*m = pos * rot;
	mat4_dump(*m);

	return 1;

}


DWORD get_wow_camera() {
	DWORD c1 = DEREF(Camera_static);
	if (!c1) return 0;
	DWORD camera = DEREF(c1 + 0x7E20);

	return camera;
}

void mat4_dump(const mat4 &m) {
	for (int r = 0; r < 4; ++r) {
		vec4 row = mat4_getrow(m, r);
		PRINT("(%.3f, %.3f, %.3f, %.3f)\n", row.x, row.y, row.z, row.w);
	}
	PRINT("\n");
}

void mat4_dump_raw(const mat4 &m) {
	for (int i = 0; i < 16; ++i) {
		PRINT("%.3f ", *(&m.m[0][0] + i));
	}

	PRINT("\n");
}

void vec4_dump(const vec4 &v) {
	PRINT("(%.3f, %.3f, %.3f, %.3f)\n\n", v.x, v.y, v.z, v.w);
}

vec4 operator*(const mat4 &m, const vec4 &v) {
	vec4 r;

	r.x = vec4_dot(v, mat4_getrow(m, 0));
	r.y = vec4_dot(v, mat4_getrow(m, 1));
	r.z = vec4_dot(v, mat4_getrow(m, 2));
	r.w = vec4_dot(v, mat4_getrow(m, 3));

	return r;
}


mat4 operator*(const mat4 &m1, const mat4 &m2) {
	mat4 m;

	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			//PRINT("row = %d, column =%d\n", r, c);
			m.m[r][c] = vec4_dot(mat4_getrow(m1, r), mat4_getcolumn(m2, c));
		}
	}

	return m;

}