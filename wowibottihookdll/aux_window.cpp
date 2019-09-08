#include <windows.h>
#include <gl/GL.h>
#include <assert.h>

#include "defs.h"

#pragma comment (lib, "opengl32.lib")

static int created = 0;

typedef char GLchar;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

typedef void (*fp_glGenVertexArrays)(GLsizei n, GLuint* arrays);
typedef void (*fp_glBindVertexArray)(GLuint array);
typedef void (*fp_glGenBuffers)(GLsizei n, GLuint* buffers);
typedef void (*fp_glBindBuffer)(GLenum target, GLuint buffer);
typedef void (*fp_glEnableVertexAttribArray)(GLuint index);
typedef void (*fp_glDisableVertexAttribArray)(GLuint index);
typedef void (*fp_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (*fp_glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (*fp_glCompileShader)(GLuint shader);
typedef void (*fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (*fp_glCompileShader)(GLuint shader);
typedef void (*fp_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void (*fp_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (*fp_glAttachShader)(GLuint program, GLuint shader);
typedef void (*fp_glLinkProgram)(GLuint program);
typedef void (*fp_glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
typedef void (*fp_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void (*fp_glDetachShader)(GLuint program, GLuint shader);
typedef void (*fp_glDeleteShader)(GLuint shader);
typedef void (*fp_glUseProgram)(GLuint program);
typedef GLuint (*fp_glCreateProgram)(void);
typedef GLuint (*fp_glCreateShader)(GLenum type);

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
static fp_glUseProgram glUseProgram;


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

	return 1;

}


int create_aux_window() {

	if (created) return 1;

	MSG msg = { 0 };
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = "ogl";
	wc.style = CS_OWNDC;
	if (!RegisterClass(&wc))
		return 0;
	CreateWindow(wc.lpszClassName, "ogl", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 256, 256, 0, 0, NULL, 0);

	created = 1;
	return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
			32,                   // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                   // Number of bits for the depthbuffer
			8,                    // Number of bits for the stencilbuffer
			0,                    // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		HDC ourWindowHandleToDeviceContext = GetDC(hWnd);

		int  letWindowsChooseThisPixelFormat;
		letWindowsChooseThisPixelFormat = ChoosePixelFormat(ourWindowHandleToDeviceContext, &pfd);
		SetPixelFormat(ourWindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

		HGLRC ourOpenGLRenderingContext = wglCreateContext(ourWindowHandleToDeviceContext);
		wglMakeCurrent(ourWindowHandleToDeviceContext, ourOpenGLRenderingContext);

		initialize_gl_extensions();

		MessageBoxA(0, "MOI", "OPENGL VERSION", 0);

	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}