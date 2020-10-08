#pragma once

#pragma comment (lib, "opengl32.lib")

#include <Windows.h>
#include <gl/gl.h>

#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include "aux_window.h"

namespace Shader {

	static GLint checkShaderCompileStatus(GLuint shaderId);
	static const std::string logfilename("shader.log");

};

class ShaderProgram {

	GLuint programHandle_;
	bool error;

	std::unordered_map<std::string, GLint> uniform_locations;

public:

	GLint checkLinkStatus();
	ShaderProgram(const std::string &shader_folder);

	bool valid() const;

	void cache_uniform_locations(const std::vector<std::string>& uniform_name);
	GLint get_uniform_location(const std::string& name) const;
	GLuint programHandle() const { return programHandle_; }
};

class Render;
typedef void (*render_initfunc)(Render*);
typedef void (*render_drawfunc)(Render*);

class Render {
public:
	ShaderProgram* shader;
	GLuint VAOid, VBOid;
	render_initfunc _init;
	render_drawfunc _draw;
	Render(ShaderProgram *p, render_initfunc initf, render_drawfunc drawf) 
		: VAOid(0), VBOid(0), shader(p), _init(initf), _draw(drawf) {

		_init(this);
	}
	
	void update_buffer(const void* data, size_t s) {
		glBindBuffer(GL_ARRAY_BUFFER, VBOid);
		glBufferSubData(GL_ARRAY_BUFFER, 0, s, data);
	}
	template <typename T>
	void update_buffer(const std::vector<T> &data) {
		glBindBuffer(GL_ARRAY_BUFFER, VBOid);
		std::size_t s = data.size() * sizeof(T);
		glBufferSubData(GL_ARRAY_BUFFER, 0, s, &data[0]);
	}


	void draw() {
		_draw(this);
	}

};
