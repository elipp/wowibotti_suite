#include <windows.h>
#include <gl/GL.h>
#include <assert.h>

#include "defs.h"
#include "dllmain.h"
#include "aux_window.h"
#include "shader.h"
#include "wowmem.h"

#pragma comment (lib, "opengl32.lib")

static int created = 0;

static HWND hWnd;
static HDC hDC;
static HGLRC hRC;

#include <fstream>
#include <sstream>
#include <stdexcept>

class ShaderProgram;

static GLuint avoid_VAOid, avoid_VBOid;
static GLuint imp_VAOid, imp_VBOid;
static GLuint imp_VAOid, imp_VBOid;

static ShaderProgram *point_shader;
static ShaderProgram *imp_shader;
static ShaderProgram *rev_shader;

static hconfig_t* current_hconfig;
static hconfig_t Marrowgar;
static int num_avoid_points;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

fp_glGenVertexArrays glGenVertexArrays;
fp_glBindVertexArray glBindVertexArray;
fp_glGenBuffers glGenBuffers;
fp_glBindBuffer glBindBuffer;
fp_glEnableVertexAttribArray glEnableVertexAttribArray;
fp_glDisableVertexAttribArray glDisableVertexAttribArray;
fp_glVertexAttribPointer glVertexAttribPointer;
fp_glCreateShader glCreateShader;
fp_glShaderSource glShaderSource;
fp_glGetShaderiv glGetShaderiv;
fp_glCompileShader glCompileShader;
fp_glGetShaderInfoLog glGetShaderInfoLog;
fp_glAttachShader glAttachShader;
fp_glLinkProgram glLinkProgram;
fp_glGetProgramiv glGetProgramiv;
fp_glGetProgramInfoLog glGetProgramInfoLog;
fp_glDetachShader glDetachShader;
fp_glDeleteShader glDeleteShader;
fp_glBufferData glBufferData;
fp_glUseProgram glUseProgram;
fp_glCreateProgram glCreateProgram;
fp_glBufferSubData glBufferSubData;

fp_glUniform1f glUniform1f;
fp_glUniform2f glUniform2f;
fp_glUniform3f glUniform3f;
fp_glUniform4f glUniform4f;
fp_glUniform1fv glUniform1fv;
fp_glUniform2fv glUniform2fv;
fp_glUniform3fv glUniform3fv;
fp_glUniform4fv glUniform4fv;
fp_glGetUniformLocation glGetUniformLocation;

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

	glGetUniformLocation = (fp_glGetUniformLocation)wglGetProcAddress("glGetUniformLocation"); assert(glGetUniformLocation);
	glBufferSubData = (fp_glBufferSubData)wglGetProcAddress("glBufferSubData"); assert(glBufferSubData);

	return 1;

}


std::vector<avoid_point_t> avoid_npc_t::get_points() const {
	std::vector<avoid_point_t> p;
	ObjectManager OM;
	WowObject iter;
	if (!OM.get_first_object(&iter)) return p;
	
	while (iter.valid()) {

		if (iter.get_type() == OBJECT_TYPE_NPC) {
			if (iter.NPC_get_name() == this->name) {
				vec3 pos = iter.get_pos();
				avoid_point_t point = { pos.x, pos.y, this->radius };
				p.push_back(point);
			}
		}

		iter = iter.next();
	}

	return p;
}

std::vector<avoid_point_t> avoid_spellobject_t::get_points() const {
	std::vector<avoid_point_t> p;

	ObjectManager OM;
	WowObject iter;
	if (!OM.get_first_object(&iter)) return p;

	while (iter.valid()) {

		if (iter.get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
			if (iter.DO_get_spellID() == this->spellID) {
				vec3 pos = iter.DO_get_pos();
				avoid_point_t point = { pos.x, pos.y, this->radius };
				p.push_back(point);
			}
		}

		iter = iter.next();
	}

	return p;
}

std::vector<avoid_point_t> avoid_units_t::get_points() const {
	std::vector<avoid_point_t> p;

	ObjectManager OM;
	WowObject iter;
	if (!OM.get_first_object(&iter)) return p;
	GUID_t player_guid = OM.get_local_GUID();

	while (iter.valid()) {
		if (iter.get_type() == OBJECT_TYPE_UNIT) {
			if (iter.get_GUID() != player_guid) {
				vec3 pos = iter.get_pos();
				avoid_point_t point = { pos.x, pos.y, this->radius };
				p.push_back(point);
			}
		}

		iter = iter.next();
	}

	return p;
}

arena_impassable_t::arena_impassable_t(vec2_t p, vec2_t n) {
	vec2_t un = unit(n);
	vec2_t perp1 = perp(un);
	vec2_t v1 = p + 1000 * perp1;
	vec2_t v2 = p - 1000 * perp1;
	vec2_t v3 = p + 1500 * n;
	PRINT("(%f, %f), (%f, %f), (%f, %f)\n", v1.x, v1.y, v2.x, v2.y, v3.x, v3.y);
	this->tri = { v1, v2, v3 };
}

static int initialize_buffers() {

	glGenVertexArrays(1, &avoid_VAOid);
	glBindVertexArray(avoid_VAOid);

	//static const std::vector<GLfloat> vbuf_data = {
	//	-0, 0, 0.5,
	//	0.5, 0, 0.5,
	//	0.25, 0.25, 0.30,
	//};

	static const std::vector<GLfloat> vbuf_data = {
		-390, 2215, 8
	};

	glGenBuffers(1, &avoid_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, avoid_VBOid);
	glBufferData(GL_ARRAY_BUFFER, 512 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vbuf_data.size() * sizeof(float), &vbuf_data[0]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	glBindVertexArray(0);

	glGenVertexArrays(1, &imp_VAOid);
	glBindVertexArray(imp_VAOid);

	glGenBuffers(1, &imp_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, imp_VBOid);
	glBufferData(GL_ARRAY_BUFFER, 64 * sizeof(arena_impassable_t), NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	glBindVertexArray(0);

	return 1;
}

static void update_buffers() {
	ObjectManager OM;
	WowObject p;
	
	std::vector<avoid_point_t> avoid_points;
	avoid_points.reserve(256 * sizeof(avoid_point_t));

	if (!OM.get_local_object(&p)) return;

	current_hconfig = &Marrowgar;
	
	for (const auto& a : current_hconfig->avoid) {
		auto v = a->get_points();
		avoid_points.insert(std::end(avoid_points), std::begin(v), std::end(v));
	}

	num_avoid_points = avoid_points.size();
	if (num_avoid_points == 0) return;

	glBindBuffer(GL_ARRAY_BUFFER, avoid_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, avoid_points.size() * sizeof(avoid_point_t), &avoid_points[0]);

}

void aux_draw() {
	glClearColor(0, 0, 0, 1);

	update_buffers();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (current_hconfig->impassable.size() > 0) {
		glUseProgram(imp_shader->programHandle());
		glUniform2fv(imp_shader->get_uniform_location("arena_middle"), 1, &current_hconfig->arena.middle.x);
		glUniform1f(imp_shader->get_uniform_location("arena_size"), current_hconfig->arena.size);

		glBindVertexArray(imp_VAOid);
		glDrawArrays(GL_TRIANGLES, 0, current_hconfig->impassable.size() * 3);
	}
	
	if (num_avoid_points > 0) {
		glUseProgram(point_shader->programHandle());
		static const GLfloat scr[2] = { HMAP_SIZE, HMAP_SIZE };
		glUniform2fv(point_shader->get_uniform_location("target_size"), 1, scr);
		glUniform2fv(point_shader->get_uniform_location("arena_middle"), 1, &current_hconfig->arena.middle.x);
		glUniform1f(point_shader->get_uniform_location("arena_size"), current_hconfig->arena.size);

		glBindVertexArray(avoid_VAOid);
		glDrawArrays(GL_POINTS, 0, num_avoid_points);
	}

	SwapBuffers(hDC);
}


int create_aux_window(const char* title, int width, int height) {

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
	wc.hIcon = NULL;// LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = NULL; // LoadCursor(NULL, IDC_ARROW);
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

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	try {
		point_shader = new ShaderProgram("shaders\\vs.glsl", "shaders\\fs.glsl", "shaders\\gs.glsl");
		imp_shader = new ShaderProgram("shaders\\imp_vs.glsl", "shaders\\imp_fs.glsl", "");
	}

	catch (const std::exception& ex) {
		PRINT("A shader error occurred: %s\n", ex.what());
		return FALSE;
	}

	point_shader->cache_uniform_location("target_size");
	point_shader->cache_uniform_location("arena_middle");
	point_shader->cache_uniform_location("arena_size");

	imp_shader->cache_uniform_location("arena_middle");
	imp_shader->cache_uniform_location("arena_size");

	// this is ported from the old 

	vec2_t v1B = vec2(-401.8, 2170);
	vec2_t v1P = vec2(-0.762509, -0.646977);
	
	vec2_t v2B = vec2(-422.9, 2200.4);
	vec2_t v2P = vec2(-1.000000, 0.000000);

	vec2_t v3B = vec2(-412.5, 2241.4);
	vec2_t v3P = vec2(-0.834276, 0.551347);

	vec2_t v4B = vec2(-372.9, 2263.8);
	vec2_t v4P = vec2(0.854369, 0.519668);

	vec2_t v5B = vec2(-357.7, 2182.9);
	vec2_t v5P = vec2(0.850798, -0.525493);

	Marrowgar = hconfig_t(true, 
		{ new avoid_npc_t(25, "Lord Marrowgar"), new avoid_npc_t(10, "Coldflame"), new avoid_spellobject_t(10, 69146), new avoid_units_t(8) }, 
		{ 140, {-390, 2215 }, 42 }, 
		{
		arena_impassable_t(v1B, v1P),
		arena_impassable_t(v2B, v2P),
		arena_impassable_t(v3B, v3P),
		arena_impassable_t(v4B, v4P),
		arena_impassable_t(v5B, v5P), 
		});

	glBindBuffer(GL_ARRAY_BUFFER, imp_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, Marrowgar.impassable.size() * sizeof(arena_impassable_t), &Marrowgar.impassable[0]);
	
	//PRINT("imp size: %d\n", Marrowgar.impassable.size() * sizeof(tri_t));
	PRINT("Aux. OGL window successfully created!\n");

	created = 1;

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		break;
	
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;

}
