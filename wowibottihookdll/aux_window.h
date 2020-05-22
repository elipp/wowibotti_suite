#pragma once

#include <Windows.h>
#include <gl/gl.h>

#include "hooks.h"
#include "linalg.h"
#include "wowmem.h"

#define HMAP_SIZE 256
#define ARENA_SIZE 140.0f // yards

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_GEOMETRY_VERTICES_OUT          0x8916
#define GL_GEOMETRY_INPUT_TYPE            0x8917
#define GL_GEOMETRY_OUTPUT_TYPE           0x8918
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88

#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87

#include "pathfinding.h"

void hotness_convert_grid_astar(float* r);

inline constexpr int HMAP_FLAT_INDEX(int x, int y) {
	return (y * HMAP_SIZE + x);
}

struct ivec2 {
	int x; int y;
};

inline constexpr ivec2 HMAP_UNRAVEL_FLATINDEX(int flatindex) {
	return { flatindex % HMAP_SIZE, flatindex / HMAP_SIZE };
}

int HMAP_get_flatindex_from_worldpos(const vec3& pos);

typedef struct arena_t {
	float size;
	vec2_t middle;
	float z;
} arena_t;

typedef struct arena_impassable_t {
	tri_t tri;
	arena_impassable_t(vec2_t p, vec2_t n);
} arena_impassable_t;

struct avoid_point_t {
	vec2_t pos;
	float radius;
	GLint falloff;
	constexpr avoid_point_t(float x, float y, float r, GLint fo)
		: pos{ x, y }, radius(r), falloff(fo) {}
};

#define BUFFER_OFFSET(i) ((void*)(i))

#define FALLOFF_CONSTANT 0
#define FALLOFF_LINEAR 1
#define FALLOFF_QUADRATIC 2
#define FALLOFF_CUBIC 3

class avoid_t {
public:
	float radius;
	GLint falloff;
	avoid_t(float r, GLint fo) : radius(r), falloff(fo) {};
	virtual std::vector<avoid_point_t> get_points() const = 0;
};

class avoid_npc_t : public avoid_t {
public:
	std::string name;
	avoid_npc_t(float radius, GLint falloff, const std::string& npcname) : avoid_t(radius, falloff), name(npcname) {}
	std::vector<avoid_point_t> get_points() const;
};

class avoid_spellobject_t : public avoid_t {
public:
	uint spellID;
	avoid_spellobject_t(float radius, GLint falloff, uint sID) : avoid_t(radius, falloff), spellID(sID) {}
	std::vector<avoid_point_t> get_points() const;
};

class avoid_units_t : public avoid_t {
public:
	avoid_units_t(float radius, GLint falloff) : avoid_t(radius, falloff) {}
	std::vector<avoid_point_t> get_points() const;
};

typedef struct rev_target_t {
	vec2_t pos;
	float radius;
} rev_target_t;

enum { // TODO: also add REV_HEALERS?
	REV_SELF = 0x1,
	REV_BOSS = 0x2,
	REV_TARGET = 0x4,
	REV_FOCUS = 0x8,
	REV_DEFAULT = 0x3,
};

class hconfig_t {
public:
	std::vector<avoid_t*> avoid;
	std::vector<arena_impassable_t> impassable;
	std::string bossname;
	int rev_flags;
	hconfig_t() : rev_flags(0) {};
	hconfig_t(const std::string &boss_name, const std::vector<avoid_t*>& avoid_stuff, int rflags, const std::vector<arena_impassable_t> &imp)
		: bossname(boss_name), avoid(avoid_stuff), rev_flags(rflags), impassable(imp) {};
	std::vector<rev_target_t> get_rev_targets() const;
};

typedef struct hotness_status_t {
	vec2i_t best_pixel;
	BYTE best_hotness;
	vec3 best_world_pos;
	BYTE current_hotness;
} hotness_status_t;

const hcache_t& get_hcache();

int create_aux_window(const char *title, int width, int height);

void aux_draw();
void aux_hide();
void aux_show();

void opengl_cleanup();

int hotness_enabled();
void hotness_enable(bool state);
int hconfig_set(const std::string& confname);
hotness_status_t hotness_status();

void update_hotness_cache();
void hotness_stop();

void echo_queue_commit();

BYTE hmap_get_pixel_float01(int x, int y);

typedef char GLchar;
typedef unsigned int GLsizeiptr;
typedef int GLintptr;

typedef void (APIENTRY* fp_glGenVertexArrays)(GLsizei n, GLuint* arrays);
typedef void (APIENTRY* fp_glBindVertexArray)(GLuint array);
typedef void (APIENTRY* fp_glGenBuffers)(GLsizei n, GLuint* buffers);
typedef void (APIENTRY* fp_glBindBuffer)(GLenum target, GLuint buffer);
typedef void (APIENTRY* fp_glEnableVertexAttribArray)(GLuint index);
typedef void (APIENTRY* fp_glDisableVertexAttribArray)(GLuint index);
typedef void (APIENTRY* fp_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (APIENTRY* fp_glVertexAttribIPointer) (GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer);
typedef void (APIENTRY* fp_glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (APIENTRY* fp_glCompileShader)(GLuint shader);
typedef void (APIENTRY* fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY* fp_glCompileShader)(GLuint shader);
typedef void (APIENTRY* fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY* fp_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY* fp_glAttachShader)(GLuint program, GLuint shader);
typedef void (APIENTRY* fp_glLinkProgram)(GLuint program);
typedef void (APIENTRY* fp_glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY* fp_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY* fp_glDetachShader)(GLuint program, GLuint shader);
typedef void (APIENTRY* fp_glDeleteShader)(GLuint shader);
typedef void (APIENTRY* fp_glUseProgram)(GLuint program);
typedef void (APIENTRY* fp_glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef GLuint(APIENTRY* fp_glCreateProgram)(void);
typedef GLuint(APIENTRY* fp_glCreateShader)(GLenum type);

typedef void (APIENTRY* fp_glUniform1f) (GLint location, GLfloat v0);
typedef void (APIENTRY* fp_glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY* fp_glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY* fp_glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY* fp_glUniform1fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY* fp_glUniform2fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY* fp_glUniform3fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY* fp_glUniform4fv) (GLint location, GLsizei count, const GLfloat* value);
typedef GLint (APIENTRY* fp_glGetUniformLocation) (GLuint program, const GLchar* name);
typedef void (APIENTRY* fp_glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);

typedef void (APIENTRY* fp_glBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const void* data);


extern fp_glGenVertexArrays glGenVertexArrays;
extern fp_glBindVertexArray glBindVertexArray;
extern fp_glGenBuffers glGenBuffers;
extern fp_glBindBuffer glBindBuffer;
extern fp_glEnableVertexAttribArray glEnableVertexAttribArray;
extern fp_glDisableVertexAttribArray glDisableVertexAttribArray;
extern fp_glVertexAttribPointer glVertexAttribPointer;
extern fp_glVertexAttribIPointer glVertexAttribIPointer;
extern fp_glCreateShader glCreateShader;
extern fp_glShaderSource glShaderSource;
extern fp_glGetShaderiv glGetShaderiv;
extern fp_glCompileShader glCompileShader;
extern fp_glGetShaderInfoLog glGetShaderInfoLog;
extern fp_glAttachShader glAttachShader;
extern fp_glLinkProgram glLinkProgram;
extern fp_glGetProgramiv glGetProgramiv;
extern fp_glGetProgramInfoLog glGetProgramInfoLog;
extern fp_glDetachShader glDetachShader;
extern fp_glDeleteShader glDeleteShader;
extern fp_glBufferData glBufferData;
extern fp_glUseProgram glUseProgram;
extern fp_glCreateProgram glCreateProgram;
extern fp_glGetUniformLocation glGetUniformLocation;
extern fp_glGetActiveUniform glGetActiveUniform;

extern fp_glUniform1f glUniform1f;
extern fp_glUniform2f glUniform2f;
extern fp_glUniform3f glUniform3f;
extern fp_glUniform4f glUniform4f;
extern fp_glUniform1fv glUniform1fv;
extern fp_glUniform2fv glUniform2fv;
extern fp_glUniform3fv glUniform3fv;
extern fp_glUniform4fv glUniform4fv;

extern fp_glBufferSubData glBufferSubData;
