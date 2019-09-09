#include <windows.h>
#include <gl/GL.h>
#include <assert.h>

#include "defs.h"
#include "dllmain.h"

#pragma comment (lib, "opengl32.lib")

static int created = 0;

static HWND hWnd;
static HDC hDC;
static HGLRC hRC;

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


#include <fstream>
#include <sstream>


typedef char GLchar;
typedef unsigned int GLsizeiptr;
typedef void (APIENTRY *fp_glGenVertexArrays )(GLsizei n, GLuint* arrays);
typedef void (APIENTRY *fp_glBindVertexArray)(GLuint array);
typedef void (APIENTRY *fp_glGenBuffers)(GLsizei n, GLuint* buffers);
typedef void (APIENTRY *fp_glBindBuffer)(GLenum target, GLuint buffer);
typedef void (APIENTRY *fp_glEnableVertexAttribArray)(GLuint index);
typedef void (APIENTRY *fp_glDisableVertexAttribArray)(GLuint index);
typedef void (APIENTRY *fp_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (APIENTRY *fp_glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (APIENTRY *fp_glCompileShader)(GLuint shader);
typedef void (APIENTRY *fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY *fp_glCompileShader)(GLuint shader);
typedef void (APIENTRY *fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY *fp_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY *fp_glAttachShader)(GLuint program, GLuint shader);
typedef void (APIENTRY *fp_glLinkProgram)(GLuint program);
typedef void (APIENTRY *fp_glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY *fp_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (APIENTRY *fp_glDetachShader)(GLuint program, GLuint shader);
typedef void (APIENTRY *fp_glDeleteShader)(GLuint shader);
typedef void (APIENTRY *fp_glUseProgram)(GLuint program);
typedef void (APIENTRY *fp_glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef GLuint (APIENTRY *fp_glCreateProgram)(void);
typedef GLuint (APIENTRY *fp_glCreateShader)(GLenum type);

typedef void (APIENTRY *fp_glUniform1f) (GLint location, GLfloat v0);
typedef void (APIENTRY *fp_glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *fp_glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *fp_glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *fp_glUniform1fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY *fp_glUniform2fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY *fp_glUniform3fv) (GLint location, GLsizei count, const GLfloat* value);
typedef void (APIENTRY *fp_glUniform4fv) (GLint location, GLsizei count, const GLfloat* value);

static fp_glGenVertexArrays glGenVertexArrays;
static fp_glBindVertexArray glBindVertexArray;
static fp_glGenBuffers glGenBuffers;
static fp_glBindBuffer glBindBuffer;
static fp_glEnableVertexAttribArray glEnableVertexAttribArray;
static fp_glDisableVertexAttribArray glDisableVertexAttribArray;
static fp_glVertexAttribPointer glVertexAttribPointer;
static fp_glCreateShader glCreateShader;
static fp_glShaderSource glShaderSource;
static fp_glGetShaderiv glGetShaderiv;
static fp_glCompileShader glCompileShader;
static fp_glGetShaderInfoLog glGetShaderInfoLog;
static fp_glAttachShader glAttachShader;
static fp_glLinkProgram glLinkProgram;
static fp_glGetProgramiv glGetProgramiv;
static fp_glGetProgramInfoLog glGetProgramInfoLog;
static fp_glDetachShader glDetachShader;
static fp_glDeleteShader glDeleteShader;
static fp_glBufferData glBufferData;
static fp_glUseProgram glUseProgram;
static fp_glCreateProgram glCreateProgram;

static fp_glUniform1f glUniform1f;
static fp_glUniform2f glUniform2f;
static fp_glUniform3f glUniform3f;
static fp_glUniform4f glUniform4f;
static fp_glUniform1fv glUniform1fv;
static fp_glUniform2fv glUniform2fv;
static fp_glUniform3fv glUniform3fv;
static fp_glUniform4fv glUniform4fv;


class ShaderProgram;

static GLuint VAOid, VBOid;
static ShaderProgram *shader;

namespace Shader {

	static GLint checkShaderCompileStatus(GLuint shaderId);
	static const std::string logfilename("shader.log");

};

class ShaderProgram {

	GLuint programHandle_;
	bool error;

public:

	GLint checkLinkStatus();
	ShaderProgram(const std::string& vs_filename, const std::string& fs_filename, const std::string& gs_filename);

	bool valid() const;

	GLuint programHandle() const { return programHandle_; }
};


ShaderProgram::ShaderProgram(const std::string& VS_filename, const std::string& FS_filename, const std::string& GS_filename) {

	std::ofstream logfile(Shader::logfilename, std::ios::ate);
	logfile << "";
	logfile.close();

	GLuint VSid, FSid; //, GSid;

	auto vs_fullname = DLL_base_path + VS_filename;

	std::ifstream input(vs_fullname);
	if (!input.is_open()) {
		PRINT("ShaderProgram: couldn't open file %s!", vs_fullname.c_str());
		error = true;
		return;
	}

	std::stringstream buffer;
	buffer << input.rdbuf();
	std::string VS_contents(buffer.str());
	input.close();
	input.clear();

	buffer.str("");
	buffer.clear();

	auto fs_fullname = DLL_base_path + FS_filename;

	input.open(fs_fullname);
	if (!input.is_open()) {
		PRINT("ShaderProgram: couldn't open file %s!", fs_fullname.c_str());
		error = true;
		return;
	}
	buffer << input.rdbuf();
	std::string FS_contents(buffer.str());

	//PRINT("%s", fs_contents.c_str());
	if (GS_filename != "") {
		PRINT("ShaderProgram: geometry shaders not supported yet.\n");
		// do something. don't know how to use geometry shaders yet, so :P
	}

	VSid = glCreateShader(GL_VERTEX_SHADER);
	FSid = glCreateShader(GL_FRAGMENT_SHADER);

	const char* VS_p = VS_contents.c_str();
	const char* FS_p = FS_contents.c_str();

	const GLint VS_len = VS_contents.length(),
		FS_len = FS_contents.length();


	//PRINT("%d, %d", vs_len, fs_len);

	glShaderSource(VSid, 1, &VS_p, &VS_len);
	glShaderSource(FSid, 1, &FS_p, &FS_len);

	glCompileShader(VSid);
	glCompileShader(FSid);

	if (!Shader::checkShaderCompileStatus(VSid)) {
		PRINT("Vertex shader compilation failed.\n");
		error = true; return;
	}

	if (!Shader::checkShaderCompileStatus(FSid)) {
		PRINT("Fragment shader compilation failed.\n");
		error = true; return;
	}

	programHandle_ = glCreateProgram();


	glAttachShader(programHandle_, VSid);
	glAttachShader(programHandle_, FSid);

	glLinkProgram(programHandle_);

	if (!checkLinkStatus()) {
		error = true; return;
	}

	error = false;
}



GLint ShaderProgram::checkLinkStatus() {

	GLint succeeded;
	glGetProgramiv(programHandle_, GL_LINK_STATUS, &succeeded);

	if (!succeeded) {

		PRINT("[shader status: program %d LINK ERROR!]\n\n", programHandle_);
		error = true;
		return 0;

	}

	else {
		return 1;
	}
}

bool ShaderProgram::valid() const {
	return error ? false : true;
}



GLint Shader::checkShaderCompileStatus(GLuint shaderId)
{

	GLint maxLength = 512;
	GLint succeeded;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &succeeded);

	if (!succeeded)
	{
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);
		char* log = new char[maxLength];
		log[maxLength - 1] = '\0';

		glGetShaderInfoLog(shaderId, maxLength, &maxLength, log);
		PRINT("[shader status: compile error] see shader.log.\n\n");

		std::ofstream logfile(Shader::logfilename, std::ios::out | std::ios::app);
		logfile.write(log, maxLength);
		logfile.close();
		delete[] log;

		return 0;
	}

	else { return 1; }

}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


static int initialize_gl_extensions() {
	glGenVertexArrays = (fp_glGenVertexArrays)wglGetProcAddress("glGenVertexArrays"); assert(glGenVertexArrays);
	glBindVertexArray = (fp_glBindVertexArray)wglGetProcAddress("glBindVertexArray"); assert(glBindVertexArray);
	glGenBuffers = (fp_glGenBuffers)wglGetProcAddress("glGenBuffers"); assert(glGenBuffers);
	glBindBuffer = (fp_glBindBuffer)wglGetProcAddress("glBindBuffer"); assert(glBindBuffer);
	glEnableVertexAttribArray = (fp_glEnableVertexAttribArray)wglGetProcAddress("glEnableVertexAttribArray"); assert(glEnableVertexAttribArray);
	glDisableVertexAttribArray = (fp_glDisableVertexAttribArray)wglGetProcAddress("glDisableVertexAttribArray"); assert(glDisableVertexAttribArray);
	glVertexAttribPointer = (fp_glVertexAttribPointer)wglGetProcAddress("glVertexAttribPointer"); assert(glVertexAttribPointer);
	glCreateShader = (fp_glCreateShader)wglGetProcAddress("glCreateShader"); assert(glCreateShader);
	glShaderSource = (fp_glShaderSource)wglGetProcAddress("glShaderSource"); assert(glShaderSource);
	glCompileShader = (fp_glCompileShader)wglGetProcAddress("glCompileShader"); assert(glCompileShader);
	glGetShaderiv = (fp_glGetShaderiv)wglGetProcAddress("glGetShaderiv"); assert(glGetShaderiv);
	glGetShaderInfoLog = (fp_glGetShaderInfoLog)wglGetProcAddress("glGetShaderInfoLog"); assert(glGetShaderInfoLog);
	glAttachShader = (fp_glAttachShader)wglGetProcAddress("glAttachShader"); assert(glAttachShader);
	glLinkProgram = (fp_glLinkProgram)wglGetProcAddress("glLinkProgram"); assert(glLinkProgram);
	glGetProgramiv = (fp_glGetProgramiv)wglGetProcAddress("glGetProgramiv"); assert(glGetProgramiv);
	glGetProgramInfoLog = (fp_glGetProgramInfoLog)wglGetProcAddress("glGetProgramInfoLog"); assert(glGetProgramInfoLog);
	glDetachShader = (fp_glDetachShader)wglGetProcAddress("glDetachShader"); assert(glDetachShader);
	glDeleteShader = (fp_glDeleteShader)wglGetProcAddress("glDeleteShader"); assert(glDeleteShader);
	glUseProgram = (fp_glUseProgram)wglGetProcAddress("glUseProgram"); assert(glUseProgram);
	glBufferData = (fp_glBufferData)wglGetProcAddress("glBufferData"); assert(glBufferData);
	glCreateProgram = (fp_glCreateProgram)wglGetProcAddress("glCreateProgram"); assert(glCreateProgram);

	glUniform1f = (fp_glUniform1f)wglGetProcAddress("glUniform1f"); assert(glUniform1f);
	glUniform2f = (fp_glUniform2f)wglGetProcAddress("glUniform2f"); assert(glUniform2f);
	glUniform3f = (fp_glUniform3f)wglGetProcAddress("glUniform3f"); assert(glUniform3f);
	glUniform4f = (fp_glUniform4f)wglGetProcAddress("glUniform4f"); assert(glUniform4f);
	glUniform1fv = (fp_glUniform1fv)wglGetProcAddress("glUniform1fv"); assert(glUniform1fv);
	glUniform2fv = (fp_glUniform2fv)wglGetProcAddress("glUniform2fv"); assert(glUniform2fv);
	glUniform3fv = (fp_glUniform3fv)wglGetProcAddress("glUniform3fv"); assert(glUniform3fv);
	glUniform4fv = (fp_glUniform4fv)wglGetProcAddress("glUniform4fv"); assert(glUniform4fv);

	return 1;

}



static int initialize_buffers() {

	glGenVertexArrays(1, &VAOid);
	glBindVertexArray(VAOid);

	static const GLfloat g_vertex_buffer_data[] = {
   -1.0f, -1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   0.0f,  1.0f, 0.0f,
	};

	glGenBuffers(1, &VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBOid);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
	// Draw the triangle !
	//glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
//	glDisableVertexAttribArray(0);

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);

	return 1;
}

void aux_draw() {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader->programHandle());
	glBindVertexArray(VAOid);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	SwapBuffers(hDC);
}


int create_aux_window(const char* title, int width, int height)
{
	if (created) return 1;

	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;

	RECT WindowRect;
	WindowRect.left = (long)0;
	WindowRect.right = (long)width;
	WindowRect.top = (long)0;
	WindowRect.bottom = (long)height;


	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = NULL;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OpenGL";

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "FAILED TO REGISTER THE WINDOW CLASS.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = width;
	dmScreenSettings.dmPelsHeight = height;
	dmScreenSettings.dmBitsPerPel = 32;
	dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;


	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
	

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if (!(hWnd = CreateWindowEx(dwExStyle, "OpenGL", title,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
		0, 0,
		WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top,
		NULL,
		NULL,
		NULL,
		NULL)))
	{
		MessageBox(NULL, "window creation error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}


	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(hDC = GetDC(hWnd)))
	{
		MessageBox(NULL, "CANT CREATE A GL DEVICE CONTEXT.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		MessageBox(NULL, "cant find a suitable pixelformat.", "ERROUE", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}


	if (!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		MessageBox(NULL, "Can't SET ZE PIXEL FORMAT.", "ERROU", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(hRC = wglCreateContext(hDC)))
	{
		MessageBox(NULL, "WGLCREATECONTEXT FAILED.", "ERREUHX", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!wglMakeCurrent(hDC, hRC))
	{
		MessageBox(NULL, "Can't activate the gl rendering context.", "ERAIX", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);

	initialize_gl_extensions();
	initialize_buffers();

	shader = new ShaderProgram("shaders\\vs.glsl", "shaders\\fs.glsl", "");

	created = 1;

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{


	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}
