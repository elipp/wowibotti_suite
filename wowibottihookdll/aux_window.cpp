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
#include <unordered_map>

class ShaderProgram;

static GLuint avoid_VAOid, avoid_VBOid;
static GLuint imp_VAOid, imp_VBOid;
static GLuint rev_VAOid, rev_VBOid;

static ShaderProgram *point_shader;
static ShaderProgram *imp_shader;
static ShaderProgram *rev_shader;

static BYTE* pixbuf;

static const hconfig_t* current_hconfig;
static int num_avoid_points;

static int hot_enabled = 0;

static hotness_status_t hstatus;

int hotness_enabled() {
	return hot_enabled;
}

void hotness_toggle() {
	if (!current_hconfig) {
		echo_wow("hotness_toggle called but no config set! use hconfig_set <confname> first");
		hot_enabled = 0;
	}
	else {
		hot_enabled = !hot_enabled;
		echo_wow("hotness set to %d", hot_enabled);
	}
}

static const std::unordered_map<std::string, hconfig_t> hconfigs = {
	// this will leak memory when ejected :D NVM
	{"Marrowgar",
	hconfig_t("Lord Marrowgar",
		{ new avoid_npc_t(25, "Lord Marrowgar"), new avoid_npc_t(15, "Coldflame"), new avoid_spellobject_t(10, 69146), new avoid_units_t(8) },
		{ 140, {-390, 2215 }, 42 },
		{ arena_impassable_t(vec2(-401.8, 2170), vec2(-0.762509, -0.646977)),
		arena_impassable_t(vec2(-422.9, 2200.4), vec2(-1.000000, 0.000000)),
		arena_impassable_t(vec2(-412.5, 2241.4), vec2(-0.834276, 0.551347)),
		arena_impassable_t(vec2(-372.9, 2263.8), vec2(0.854369, 0.519668)),
		arena_impassable_t(vec2(-357.7, 2182.9), vec2(0.850798, -0.525493)),
		})
	},
};

int hconfig_set(const std::string& confname) {

	try {
		const auto& c = hconfigs.at(confname);
		current_hconfig = &c;
		dual_echo("hconfig_set: config set to %s", confname.c_str());
		if (current_hconfig->impassable.size() > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, imp_VBOid);
			glBufferSubData(GL_ARRAY_BUFFER, 0, current_hconfig->impassable.size() * sizeof(arena_impassable_t), &current_hconfig->impassable[0]);
		}
	
		return 1;
	}
	catch (const std::exception &e) {
		dual_echo("hconfig_set: error: %s", e.what());
		current_hconfig = NULL;
		return 0;
	}

}

hotness_status_t hotness_status() {
	return hstatus;
}

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
	this->tri = { v1, v2, v3 };
}

std::vector<rev_target_t> hconfig_t::get_rev_targets() const {
	ObjectManager OM;
	WowObject p;
	OM.get_local_object(&p);

	std::vector<rev_target_t> r;

	vec3 ppos = p.get_pos();
	r.push_back({ ppos.x, ppos.y, 10 });
	
	auto b = OM.get_NPCs_by_name(bossname);

	if (b.size() == 0) {
		return r;
	}
	if (b.size() > 1) {
		PRINT("warning: more than one targets exist with the bossname \"%s\"\n", bossname.c_str());
	}

	vec3 bpos = b[0].get_pos();

	r.push_back({ bpos.x, bpos.y, 55 });

	return r;

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

	glGenVertexArrays(1, &rev_VAOid);
	glBindVertexArray(rev_VAOid);
	glGenBuffers(1, &rev_VBOid);
	glBindBuffer(GL_ARRAY_BUFFER, rev_VBOid);
	static const tri_t fullscreen_tri = { vec2(-1, -1), vec2(3, -1), vec2(-1, 3) }; // might want to add a margin
	glBufferData(GL_ARRAY_BUFFER, 1 * sizeof(tri_t), &fullscreen_tri, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
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

	current_hconfig = &hconfigs.at("Marrowgar");
	
	for (const auto& a : current_hconfig->avoid) {
		auto v = a->get_points();
		avoid_points.insert(std::end(avoid_points), std::begin(v), std::end(v));
	}

	num_avoid_points = avoid_points.size();
	if (num_avoid_points == 0) return;

	glBindBuffer(GL_ARRAY_BUFFER, avoid_VBOid);
	glBufferSubData(GL_ARRAY_BUFFER, 0, avoid_points.size() * sizeof(avoid_point_t), &avoid_points[0]);

}

static inline BYTE get_pixel(int x, int y) {
	return pixbuf[y * HMAP_SIZE + x];
}

static vec2_t world2screen(float x, float y, const arena_t& arena) {

	const float A = (arena.middle.y + arena.size / 2.0);
	const float B = (arena.middle.x - arena.size / 2.0);

	float nx = (-y + A) / arena.size;
	float ny = (x - B) / arena.size;
	return vec2(nx * 2 - 1, ny * 2 - 1);

}

static vec2_t screen2world(float x, float y, const arena_t& arena) {

	const float A = (arena.middle.y + arena.size / 2.0);
	const float B = (arena.middle.x - arena.size / 2.0);

	float nx = B + arena.size * (y + 1) / 2.0f;
	float ny = A - arena.size * (x + 1) / 2.0f;

	return vec2(nx, ny);
}

static vec2_t tex2screen(int x, int y) {
	// assume square texture
	float fx = (float(x)) / (float)HMAP_SIZE;
	float fy = (float(y)) / (float)HMAP_SIZE;

	return vec2(2 * fx - 1, -2 * fy + 1);
}

static vec2_t tex2world(int x, int y, const arena_t& arena) {
	vec2_t t = tex2screen(x, y);
	return screen2world(t.x, t.y, arena);
}

static inline int clampi(int i, int min, int max) {
	if (i < min) return min;
	if (i > max) return max;
	return i;
}

static vec2i_t screen2tex(float x, float y) {
	int xt = clampi((x + 1) * HMAP_SIZE / 2.0f, 0, HMAP_SIZE - 1);
	int yt = clampi((y + 1) * HMAP_SIZE / 2.0f, 0, HMAP_SIZE - 1);
	return { xt, yt };
}

static vec2i_t world2tex(float x, float y) {
	vec2_t scr = world2screen(x, y, current_hconfig->arena);
	return screen2tex(scr.x, scr.y);
}


static pixinfo_t get_lowest_pixel() {
	vec2i_t lowest = { 0,0 };
	BYTE lowest_val = 255;
	for (int y = 0; y < HMAP_SIZE; ++y) {
		for (int x = 0; x < HMAP_SIZE; ++x) {
			BYTE p = get_pixel(x, y);
			if (p < lowest_val) {
				lowest_val = p;
				lowest = { x, y };
			}
		}
	}
	
	return { lowest, lowest_val };
}

static void update_hstatus() {

	if (!current_hconfig) return;

	pixinfo_t lowest = get_lowest_pixel();
	hstatus.best_pixel = lowest.pos;
	hstatus.best_hotness = lowest.value;
	vec2_t w = tex2world(lowest.pos.x, lowest.pos.y, current_hconfig->arena);
	hstatus.best_world_pos = vec3(w.x, w.y, current_hconfig->arena.z);
	ObjectManager OM;
	WowObject p;
	OM.get_local_object(&p);
	vec3 ppos = p.get_pos();
	vec2i_t ppos_tex = world2tex(ppos.x, ppos.y);
	hstatus.current_hotness = get_pixel(ppos_tex.x, ppos_tex.y);
	PRINT("lowest: (%d, %d), value %u (world: %f, %f), (player pixel: %u, %u), current_hotness = %u\n", lowest.pos.x, lowest.pos.y, lowest.value, w.x, w.y, ppos_tex.x, ppos_tex.y, hstatus.current_hotness);
}

void aux_draw() {

	static const GLfloat scr[2] = { HMAP_SIZE, HMAP_SIZE };

	glClearColor(0, 0, 0, 1);

	update_buffers();

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(rev_shader->programHandle());
	glBindVertexArray(rev_VAOid);
	glUniform2fv(rev_shader->get_uniform_location("render_target_size"), 1, scr);
	glUniform2fv(rev_shader->get_uniform_location("arena_middle"), 1, &current_hconfig->arena.middle.x);
	glUniform1f(rev_shader->get_uniform_location("arena_size"), current_hconfig->arena.size);

	auto rev = current_hconfig->get_rev_targets();
	for (const auto& r : rev) {
		glUniform2fv(rev_shader->get_uniform_location("world_pos"), 1, &r.pos.x);
		glUniform1f(rev_shader->get_uniform_location("radius"), r.radius);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	if (current_hconfig->impassable.size() > 0) {
		glUseProgram(imp_shader->programHandle());
		glUniform2fv(imp_shader->get_uniform_location("arena_middle"), 1, &current_hconfig->arena.middle.x);
		glUniform1f(imp_shader->get_uniform_location("arena_size"), current_hconfig->arena.size);

		glBindVertexArray(imp_VAOid);
		glDrawArrays(GL_TRIANGLES, 0, current_hconfig->impassable.size() * 3);
	}
	
	if (num_avoid_points > 0) {
		glUseProgram(point_shader->programHandle());
		glUniform2fv(point_shader->get_uniform_location("render_target_size"), 1, scr);
		glUniform2fv(point_shader->get_uniform_location("arena_middle"), 1, &current_hconfig->arena.middle.x);
		glUniform1f(point_shader->get_uniform_location("arena_size"), current_hconfig->arena.size);

		glBindVertexArray(avoid_VAOid);
		glDrawArrays(GL_POINTS, 0, num_avoid_points);
	}

	glReadPixels(0, 0, HMAP_SIZE, HMAP_SIZE, GL_RED, GL_UNSIGNED_BYTE, pixbuf);
	update_hstatus();

	SwapBuffers(hDC);

}


void aux_hide() {
	ShowWindow(hWnd, SW_HIDE);
}
void aux_show() {
	ShowWindow(hWnd, SW_SHOW);
}

#define CLASSNAME "ogl hotness"

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
	wc.lpszClassName = CLASSNAME;

	if (!RegisterClass(&wc))
	{
		PRINT("registerclass failed: %d\n", GetLastError());
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

	if (!(hWnd = CreateWindowEx(dwExStyle, CLASSNAME, title,
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

	//ShowWindow(hWnd, SW_SHOW);
	//SetForegroundWindow(hWnd);
	//SetFocus(hWnd);

	initialize_gl_extensions();
	initialize_buffers();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	try {
		point_shader = new ShaderProgram("shaders\\vs.glsl", "shaders\\fs.glsl", "shaders\\gs.glsl");
		imp_shader = new ShaderProgram("shaders\\imp_vs.glsl", "shaders\\imp_fs.glsl", "");
		rev_shader = new ShaderProgram("shaders\\rev_vs.glsl", "shaders\\rev_fs.glsl", "");
	}

	catch (const std::exception& ex) {
		PRINT("A shader error occurred: %s\n", ex.what());
		MessageBox(NULL, "Shader error", "error", 0);
		return FALSE;
	}

	point_shader->cache_uniform_location("render_target_size");
	point_shader->cache_uniform_location("arena_middle");
	point_shader->cache_uniform_location("arena_size");

	imp_shader->cache_uniform_location("arena_middle");
	imp_shader->cache_uniform_location("arena_size");

	rev_shader->cache_uniform_location("world_pos");
	rev_shader->cache_uniform_location("radius");
	rev_shader->cache_uniform_location("render_target_size");
	rev_shader->cache_uniform_location("arena_middle");
	rev_shader->cache_uniform_location("arena_size");

	pixbuf = new BYTE[HMAP_SIZE * HMAP_SIZE]; // apparently we can read only the red channel with glReadPixels so skip the * 4 (= 4 bytes of RGBA)
	memset(pixbuf, 0, HMAP_SIZE* HMAP_SIZE);
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

void opengl_cleanup() {
	wglDeleteContext(hRC);
	delete[] pixbuf;
	DestroyWindow(hWnd);
	UnregisterClass(CLASSNAME, GetModuleHandle(NULL));
}