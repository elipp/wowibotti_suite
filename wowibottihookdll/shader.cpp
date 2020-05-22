#include <Windows.h>
#include <gl/GL.h>
#include <cstdio>

#include "shader.h"

#include "defs.h"
#include "aux_window.h"
#include "dllmain.h"

static bool file_exists(const std::string& filename) {
	if (FILE * file = fopen(filename.c_str(), "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}


static std::string read_shader(const std::string& fname) {
	std::ifstream input(fname);
	if (!input.is_open()) {
		PRINT("ShaderProgram: couldn't open file %s!\n", fname.c_str());
		return "ERROR";
	}

	std::stringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();

}

static int load_shader(GLenum stype, const std::string& filename, GLuint *sid) {

	const std::string source = read_shader(filename);

	if (source == "ERROR") {
		throw std::exception(("unable to open " + filename).c_str());
		return 0;
	}

	GLuint s = glCreateShader(stype);
	GLint sslen = source.length();
	const GLchar* src = source.c_str();
	glShaderSource(s, 1, &src, &sslen);
	glCompileShader(s);

	if (!Shader::checkShaderCompileStatus(s)) {
		throw std::exception(("shader compilation failed! filename: " + filename).c_str());
		return 0;
	}

	*sid = s;



	return 1;

}

ShaderProgram::ShaderProgram(const std::string& shader_folder) {

	GLuint VSid, FSid, GSid;

	auto vs_fullname = DLL_base_path + "\\\\shaders\\\\" + shader_folder + "\\\\vs.glsl";
	auto gs_fullname = DLL_base_path + "\\\\shaders\\\\" + shader_folder + "\\\\gs.glsl";
	auto fs_fullname = DLL_base_path + "\\\\shaders\\\\" + shader_folder + "\\\\fs.glsl";

	bool has_gs = file_exists(gs_fullname);

	try {
		load_shader(GL_VERTEX_SHADER, vs_fullname, &VSid);
		if (has_gs) load_shader(GL_GEOMETRY_SHADER, gs_fullname, &GSid);
		load_shader(GL_FRAGMENT_SHADER, fs_fullname, &FSid);
	}

	catch (...) {
		throw;
		return;
	}

	programHandle_ = glCreateProgram();

	glAttachShader(programHandle_, VSid);
	if (has_gs) glAttachShader(programHandle_, GSid);
	glAttachShader(programHandle_, FSid);

	glLinkProgram(programHandle_);

	if (!checkLinkStatus()) {
		throw std::exception("program link error!");
		return;
	}

}

static void print_uniform_data(GLuint program) {

	GLint count = 0;

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
	printf("shaderprog %u: Active uniforms: %d\n", program, count);

	char namebuf[256];

	for (int i = 0; i < count; i++) {
		GLsizei length = 0;
		GLint size = 0;
		GLenum type = 0;
		glGetActiveUniform(program, (GLuint)i, 256, &length, &size, &type, namebuf);
		printf("- uniform #%d type: %u name: %s\n", i, type, namebuf);
	}
}

void ShaderProgram::cache_uniform_locations(const std::vector<std::string>& uniforms) {
	for (const auto& u : uniforms) {
		GLint loc = glGetUniformLocation(this->programHandle_, u.c_str());
		if (loc == -1) { PRINT("warning: shaderprog %u: glGetUniformLocation returned -1 for uniform %s\n", this->programHandle(), u.c_str()); }
		else { 
			PRINT("shaderprog %u: inserting %s to location %u in the uniform cache\n", this->programHandle(), u.c_str(), loc);
			this->uniform_locations.insert({ u, loc }); 
		}
	}

	print_uniform_data(programHandle_);

}

GLint ShaderProgram::get_uniform_location(const std::string &name) const {
	GLint r = -1;
	try {
		r = this->uniform_locations.at(name);
	}
	catch (const std::exception &e) {
		PRINT("get_uniform_location (cached): error: %s (key: %s)\n", e.what(), name.c_str());
	}

	return r;
}

GLint ShaderProgram::checkLinkStatus() {

	GLint succeeded;
	glGetProgramiv(programHandle_, GL_LINK_STATUS, &succeeded);

	if (!succeeded) {
		char logbuf[1024];
		int len = 1024;
		glGetProgramInfoLog(programHandle_, sizeof(logbuf), &len, logbuf);
		PRINT("[shader status: program %d LINK ERROR!]\n%s\n", programHandle_, logbuf);
		throw std::exception("shader link error");
		return 0;
	}

	else {
		return 1;
	}
}

bool ShaderProgram::valid() const {
	return error ? false : true;
}



GLint Shader::checkShaderCompileStatus(GLuint shaderId) {

	GLint succeeded;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &succeeded);

	if (!succeeded)
	{
		char log[1024];
		int len = 1024;
		glGetShaderInfoLog(shaderId, len, &len, log);
		PRINT("[shader status: compile error]\n%s\n", log);

		return 0;
	}

	else { return 1; }

}