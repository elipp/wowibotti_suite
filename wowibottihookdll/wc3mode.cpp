
#include <Windows.h>
#include <unordered_map>
#include <string>

#include "wc3mode.h"
#include "wowmem.h"
#include "linalg.h"
#include "hooks.h"
#include "input.h"

static POINT cursor_pos;

static RECT window_rect;
static std::unordered_map<GUID_t, std::string> selected_units;

int get_window_width() {
	return window_rect.right - window_rect.left;
}

int get_window_height() {
	return window_rect.bottom - window_rect.top;
}

float get_aspect_ratio() {
	return (float)get_window_width() / (float)get_window_height();
}

static int rect_active = 0;
static int buffers_initialized = 0;
static POINT rect_begin;

static int create_d3d9buffers(IDirect3DDevice9 *d);
static void populate_d3d9buffers();
static void free_d3d9buffers();

#define SMAX 0.7
#define SMIN 0.4

glm::vec4 customcamera_t::get_cameraoffset() {
	// y = z^2
	return maxdistance*glm::vec4(0, 2 * s*s, s, 1.0);
}
float customcamera_t::get_angle() {
	//derivative dy/dz: y = 2z
	return s*1.57;
}
void customcamera_t::increment_s() {
	s += 0.010;
	s = s > SMAX ? SMAX : s;
}
void customcamera_t::decrement_s() {
	s -= 0.010;
	s = s < SMIN ? SMIN : s;
}

float customcamera_t::get_s() { return s;  }

customcamera_t customcamera = { 0.5, 30, glm::vec4(0, 80, 0, 1) };

static void update_camera_rotation(wow_camera_t *camera) {

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), -customcamera.get_angle(), glm::vec3(0, 1.0, 0));
	set_wow_rot(rot);
}

static void move_camera_if_cursor() {

	if (GetActiveWindow() != wow_hWnd) return;

	if (rect_active) return;

	//if (cursor_pos.x < 0 || cursor_pos.x > get_window_width()) return;
	//if (cursor_pos.y < 0 || cursor_pos.y > get_window_height()) return;

	//PRINT("camera: 0x%X\n", camera);

	const float dd = 0.1*((1.0-SMIN) + customcamera.get_s());
	const float margin = 50;

	int ww = get_window_width();
	int wh = get_window_height();

	//PRINT("ww: %d, wh: %d\n", ww, wh);

	// TODO: add camera velocity scaling according to how near the edge of the window the cursor is.


	if (cursor_pos.x < margin) {
		float xdiff = fabs(cursor_pos.x - margin);
		float dr = xdiff / margin;
		customcamera.pos.x -= dd*(1 + dr);
	}
	else if (cursor_pos.x > ww - margin) {
		float xdiff = fabs((ww - cursor_pos.x) - margin);
		float dr = xdiff / margin;
		customcamera.pos.x += dd*(1+dr);
	}

	if (cursor_pos.y < margin) {
		float ydiff = fabs(cursor_pos.y - margin);
		float dr = ydiff / margin;
		customcamera.pos.z -= dd*(1 + dr);
	}
	else if (cursor_pos.y > wh - margin) {
		float ydiff = fabs((wh - cursor_pos.y) - margin);
		float dr = ydiff / margin;
		customcamera.pos.z += dd*(1 + dr);
	}

}

static void update_wowcamera() {

	wow_camera_t *camera = (wow_camera_t*)get_wow_camera();
	if (!camera) return;

	move_camera_if_cursor();

	glm::vec4 newpos = customcamera.pos + customcamera.get_cameraoffset() + glm::vec4(0, 3, -6, 1);

	glm::vec4 nw = glm2wow(newpos);

	camera->x = nw.x;
	camera->y = nw.y;
	camera->z = nw.z;

	update_camera_rotation(camera);

}

#define CAMERAPATCH_BASE 0x6075AB

static const int camera_patchsize = 0x6075EC - CAMERAPATCH_BASE;

static BYTE camera_original[camera_patchsize];
static BYTE camera_patched[camera_patchsize];

static void get_original_camerafunc() {
	memcpy(camera_original, (LPVOID)CAMERAPATCH_BASE, camera_patchsize);
}

static void get_patched_camerafunc() {
	memcpy(camera_patched, (LPVOID)CAMERAPATCH_BASE, camera_patchsize);
}

void unpatch_camera() {
	WriteProcessMemory(glhProcess, (LPVOID)CAMERAPATCH_BASE, camera_original, camera_patchsize, NULL);

	static const BYTE orig[2] = { 0xF3, 0xA5 };
	DWORD *rot = (DWORD*)0x4C5810;
	WriteProcessMemory(glhProcess, rot, orig, 2, NULL);
}

static void patch_camera() {
	// [[B7436C] + 7E20]
	// FOV is at camera + 0x40
	// zNear is at 0x38, zFar is 0x3C
	static const DWORD nop = 0x90909090;

	DWORD *a1 = (DWORD*)CAMERAPATCH_BASE;
	WriteProcessMemory(glhProcess, a1, &nop, 2, NULL);

	DWORD *a2 = (DWORD*)0x6075B2;
	WriteProcessMemory(glhProcess, a2, &nop, 3, NULL);

	DWORD *a3 = (DWORD*)0x6075C5;
	WriteProcessMemory(glhProcess, a3, &nop, 3, NULL);

	DWORD *aa1 = (DWORD*)0x6075D2;
	WriteProcessMemory(glhProcess, aa1, &nop, 2, NULL);

	DWORD *aa2 = (DWORD*)0x6075E3;
	WriteProcessMemory(glhProcess, aa2, &nop, 3, NULL);

	DWORD *aa3 = (DWORD*)0x6075E9;
	WriteProcessMemory(glhProcess, aa3, &nop, 3, NULL);

	DWORD *rot = (DWORD*)0x4C5810;
	WriteProcessMemory(glhProcess, rot, &nop, 2, NULL);

}

static int patches_prepared = 0;

static void wc3mode_prepare_camera_patches() {

	// this is kinda stupid but oh well

	get_original_camerafunc();
	patch_camera();
	get_patched_camerafunc();
	unpatch_camera();

	patches_prepared = 1;
}


void reset_camera() {

	wow_camera_t *camera = (wow_camera_t*)get_wow_camera();
	if (!camera) return;

	ObjectManager OM;
	WowObject o;
	if (!OM.get_local_object(&o)) {
		return;
	}

	vec3 pos = o.get_pos();

	customcamera.pos = wow2glm(glm::vec4(pos.x, pos.y, pos.z, 1.0));
	customcamera.pos.z += 8;

	glm::vec4 newpos = customcamera.pos + customcamera.get_cameraoffset();

	glm::vec4 nw = glm2wow(newpos);

	camera->x = nw.x;
	camera->y = nw.y;
	camera->z = nw.z;

}


static void draw_rect(RECT *r, D3DLOCKED_RECT *dr, DWORD XRGB) {

	BYTE *b = (BYTE*)dr->pBits;

	for (int i = r->left; i < r->right; ++i) {
		memcpy(&b[r->top*dr->Pitch + i * 4], &XRGB, sizeof(XRGB)); // top horizontal
		memcpy(&b[r->bottom*dr->Pitch + i * 4], &XRGB, sizeof(XRGB)); // bottom horizontal
	}

	for (int i = r->top; i < r->bottom; ++i) {
		memcpy(&b[r->left * 4 + i*dr->Pitch], &XRGB, sizeof(XRGB)); // left vertical
		memcpy(&b[r->right * 4 + i*dr->Pitch], &XRGB, sizeof(XRGB)); // right vertical
	}

}

template <typename T> T CLAMP(const T& value, const T& low, const T& high) {
	return value < low ? low : (value > high ? high : value);
}

static void fix_mouse_rect(RECT *r) {

	r->left = CLAMP(r->left, (LONG)0, window_rect.right - 1);
	r->right = CLAMP(r->right, (LONG)0, window_rect.right - 1);
	r->top = CLAMP(r->top, (LONG)0, window_rect.bottom - 1);
	r->bottom = CLAMP(r->bottom, (LONG)0, window_rect.bottom - 1);

	if (r->left > r->right) {
		int temp = r->left;
		r->left = r->right;
		r->right = temp;
	}
	if (r->top > r->bottom) {
		int temp = r->top;
		r->top = r->bottom;
		r->bottom = temp;
	}

}

static void draw_pixel(int x, int y) {

	IDirect3DDevice9 *d3dd = (IDirect3DDevice9*)get_wow_d3ddevice();

	IDirect3DSwapChain9 *sc;
	if (FAILED(d3dd->GetSwapChain(0, &sc))) {
		PRINT("GetSwapChain failed\n");
		return;
	}
	IDirect3DSurface9 *s;

	if (FAILED(sc->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &s))) {
		PRINT("GetBackBuffer failed\n");
		return;
	}

	D3DLOCKED_RECT r;

	if (FAILED(s->LockRect(&r, NULL, D3DLOCK_DONOTWAIT))) {
		PRINT("%d LockRect failed\n", GetTickCount());
		return;
	}

	BYTE *b = (BYTE*)r.pBits;

	memset(&b[r.Pitch * y + 4 * x], 0xFF, 4);

	s->UnlockRect();

	s->Release();
	sc->Release();
}

static RECT get_selection_rect() {
	RECT mr;

	mr.bottom = rect_begin.y;
	mr.right = rect_begin.x;

	mr.top = cursor_pos.y;
	mr.left = cursor_pos.x;

	fix_mouse_rect(&mr);

	return mr;
}

static void draw_rect_brute() {

	IDirect3DDevice9 *d3dd = (IDirect3DDevice9*)get_wow_d3ddevice();
	if (!d3dd) return;

	IDirect3DSwapChain9 *sc;
	if (FAILED(d3dd->GetSwapChain(0, &sc))) {
		PRINT("GetSwapChain failed\n");
		return;
	}
	IDirect3DSurface9 *s;

	if (FAILED(sc->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &s))) {
		PRINT("GetBackBuffer failed\n");
		return;
	}

	D3DLOCKED_RECT r;

	RECT mr = get_selection_rect();

	//PRINT("b: %d, t: %d, l: %d, r: %d\n", mr.bottom, mr.top, mr.left, mr.right);
	HRESULT hr = s->LockRect(&r, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(hr)) {
		PRINT("%d LockRect failed, %X\n", GetTickCount(), hr);
		return;
	}


	//	MAKE_D3DHRESULT(s)
	draw_rect(&mr, &r, D3DCOLOR_XRGB(0, 255, 0));

	D3DSURFACE_DESC d;
	s->GetDesc(&d);

	PRINT("D3DSURFACE_DESC: FORMAT: %X\n", d.Format);

	s->UnlockRect();

	s->Release();
	sc->Release();

}

static POINT map_clip_to_screen(const glm::vec4& c) {
	float cx = c.x + 0.5;
	float cy = (-c.y) + 0.5;

	int px = cx * get_window_width();
	int py = cy * get_window_height();

	return POINT{ px, py };
}

static int get_screen_coords(GUID_t GUID, POINT *coords) {
	wow_camera_t *c = (wow_camera_t*)get_wow_camera();
	if (!c) return 0;

	ObjectManager OM;
	WowObject o;
	if (!OM.get_object_by_GUID(GUID, &o)) return 0;
	//if (!OM.get_local_object(&o)) return 0;

	vec3 unitpos = o.get_pos();
	//glm::vec4 up(-unitpos.y, unitpos.x, unitpos.z, 1);
	glm::vec4 up = wow2glm(glm::vec4(unitpos.x, unitpos.y, unitpos.z, 1.0));

	// increasing up.y just slightly will make the selection more intuitive
	up.y += 0.4;

	glm::vec3 cpos = wow2glm(glm::vec3(c->x, c->y, c->z));

	glm::mat4 proj = glm::perspective(c->fov, c->aspect, c->zNear, c->zFar);
	glm::mat4 view = glm::translate(glm::mat4(1.0), -cpos);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), customcamera.get_angle(), glm::vec3(1.0, 0, 0));
	glm::mat4 nMVP = proj*(rot*view);

	glm::vec4 nclip = nMVP*up;
	nclip /= nclip.w;

	
	*coords = map_clip_to_screen(nclip);
	return 1;
}

static inline float get_rect_area(const RECT &r) {
	return abs((r.bottom - r.top) * (r.left - r.right));
}

static inline void broaden_rect(RECT *r) {
	r->bottom += 20;
	r->left -= 20;
	r->right += 20;
	r->top -= 20;
}

static std::unordered_map<GUID_t, std::string> get_units_in_selection_rect(RECT sel) {

	std::unordered_map<GUID_t, std::string> units;

	ObjectManager OM;
	WowObject iter;

	if (!OM.get_first_object(&iter)) return units;

	if (get_rect_area(sel) < 150) {
		PRINT("rect_area < 150! broadening\n");
		broaden_rect(&sel);
	}

	while (iter.valid()) {
		if (iter.get_type() == OBJECT_TYPE_UNIT) {
			POINT coords;
			memset(&coords, 0, sizeof(coords));
			GUID_t unitguid = iter.get_GUID();
			get_screen_coords(unitguid, &coords);

			PRINT("checking unit 0x%016llX (screen coords: %d, %d), selection rect data: (t: %ld, b: %ld, l: %ld, r: %ld)\n", 
				unitguid, coords.x, coords.y, sel.top, sel.bottom, sel.left, sel.right);

			if (coords.x > sel.left && coords.x < sel.right
				&& coords.y < sel.bottom && coords.y > sel.top) {
				units[unitguid] = iter.unit_get_name();
				//PRINT("^UNIT IN RECT!\n");
			}

		}
		iter = iter.next();

	}

	return units;

}

void do_wc3mode_stuff() {

	if (!patches_prepared) wc3mode_prepare_camera_patches();

	if (wc3mode_enabled()) { // having the check here actually fixes a lot of problems.. :D
		update_wowcamera();
	}

	GetClientRect(wow_hWnd, &window_rect);
	get_cursor_pos(&cursor_pos);

}

static void add_units_to_selection(const std::unordered_map<GUID_t, std::string> &units) {

	for (auto &u : units) {
		if (selected_units.find(u.first) == selected_units.end()) {
			selected_units[u.first] = u.second;
		}
	}
}

static void replace_selection(const std::unordered_map<GUID_t, std::string> &units) {
	selected_units = units;
}

static std::string get_selected_units_commasep() {

	if (selected_units.size() < 1) return "";

	std::string units_concatd;

	for (auto &u : selected_units) {
		units_concatd.append(u.second);
		units_concatd.push_back(',');
	}

	units_concatd.pop_back(); // get rid of the last ','

	return units_concatd;
}

void wc3mode_mouseup_hook() {

	rect_active = 0;
	free_d3d9buffers();

	auto rectunits = get_units_in_selection_rect(get_selection_rect());

	if (rectunits.size() < 1) {
	//	DoString("RunMacroText(\"/lole clearselection\")"); // in actual Warcraft III, the selection doesn't get cleared when you click on emptiness
		return;
	}

	if (controlkeyisdown()) { // add the
		add_units_to_selection(rectunits);
	}
	else {
		replace_selection(rectunits);
	}

	std::string units_commasep = get_selected_units_commasep();

	DoString("RunMacroText(\"/lole setselection %s\")", units_commasep.c_str());
	
}

static void set_internal_selection(const std::string &names_commaseparated) {

	if (names_commaseparated == "") {
		selected_units = std::unordered_map<GUID_t, std::string>();
		return;
	}

	std::vector<std::string> names;
	tokenize_string(names_commaseparated, ",", names);
	selected_units.clear();

	ObjectManager OM;

	for (auto &n : names) {
		WowObject o;
		if (OM.get_unit_by_name(n, &o)) {
			selected_units[o.get_GUID()] = n;
		}
	}
}

void wc3mode_setselection(const std::string &names_commaseparated) {
	//PRINT("%s\n", names_commaseparated.c_str());
	set_internal_selection(names_commaseparated);

	if (names_commaseparated == "") {
		DoString("RunMacroText(\"/lole clearselection\")", names_commaseparated.c_str());
	}
	else {
		DoString("RunMacroText(\"/lole setselection %s\")", names_commaseparated.c_str());
	}
}


static struct {
	std::unordered_map<GUID_t, std::string> cgroups[10];
	void assign(const std::unordered_map<GUID_t, std::string> &units, int index) {
		if (index < 0 || index > 9) return;
		cgroups[index] = units;
	}

	void clear() { for (int i = 0; i < 9; ++i) cgroups[i] = std::unordered_map<GUID_t, std::string>(); }
} cgroup_pool;


void wc3mode_assign_cgroup(int index) {
	if (index > 9) return;
	cgroup_pool.assign(selected_units, index);
}

void wc3mode_restore_cgroup(int index) {
	if (index > 9) return;

	selected_units = cgroup_pool.cgroups[index];

	wc3mode_setselection(get_selected_units_commasep());
}


static int wc3_enabled = 0;

int wc3mode_enabled() {
	return wc3_enabled;
}

void enable_wc3mode(int b) {
	if (b == 2) { b = !wc3_enabled; }

	if (b) {
		wc3_enabled = 1;
		patch_camera();
		DoString("RunMacroText(\"/lole sfshow\")");
	}

	else {
		wc3_enabled = 0;
		unpatch_camera();
		DoString("RunMacroText(\"/lole sfhide\")");
		rect_active = 0;
	}
}

void wc3_start_rect() {
	get_cursor_pos(&rect_begin);
	rect_active = 1;
}

bool wc3_rect_active() {
	return rect_active == 1;
}

#pragma optimize( "", off )

void wc3_draw_pylpyrs() {
	ObjectManager OM;
	WowObject o;

	for (auto &u : selected_units) {
		if (!OM.get_object_by_GUID(u.first, &o)) continue;
		DWORD objectbase = o.get_base();
		DWORD vt = DEREF(objectbase);
		DWORD funcaddr = DEREF(vt + 0x6C);

		__asm {
			mov ecx, objectbase;
			call funcaddr;
		}

	}
}

#pragma optimize ("", on)

// D3D9 stuff
// these were generated by FXC.exe.
// for the vertex shader: FXC /Fh /T vs_3_0 vs.hlsl
// pixel shader: FXC /Fh /T ps_3_0 vs.hlsl

#include "vs.h"
#include "ps.h"

#include "qvs.h"
#include "qps.h"

static IDirect3DVertexBuffer9 *vbuffer;
static IDirect3DIndexBuffer9 *ibuffer;
static IDirect3DVertexShader9 *vs;
static IDirect3DPixelShader9 *ps;
static IDirect3DVertexShader9 *quad_vs;
static IDirect3DPixelShader9 *quad_ps;
static IDirect3DVertexDeclaration9 *vd;
static IDirect3DVertexDeclaration9 *quad_vd;


static LPDIRECT3DTEXTURE9 rendertex;
static LPDIRECT3DSURFACE9 rendertex_surf, back_buffer, depthstencil;
static D3DVIEWPORT9 viewport_original;


static IDirect3DVertexBuffer9 *lol_vbuffer, *lol_quadbuffer;
static IDirect3DIndexBuffer9 *lol_ibuffer;

static D3DVERTEXELEMENT9 vdecl[] = {
	{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	D3DDECL_END()
};

static D3DVERTEXELEMENT9 quad_vdecl[] = {
	{ 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

static int customd3d_initialized = 0;

void rect_to_vertices(const RECT &r, float *in) {

	float w = get_window_width();
	float h = get_window_height();

	float bottom = -(2*r.bottom/h-1);
	float left = 2*r.left/w-1;
	float right = 2*r.right/w-1;
	float top = -(2*r.top/h-1);

	float vertices[] = {
		left, bottom,
		right, bottom,
		right, top,
		left, top,
	};

	memcpy(in, vertices, sizeof(vertices));
}

typedef struct vec2_t {
	float x; float y;
} vec2_t;

vec2_t vec2(float x, float y) {
	vec2_t v;
	v.x = x;
	v.y = y;
	return v;
}

static const float ARENA_SIZE = 104;

static const vec2_t MIDDLE { -390, 2210 };

vec2_t wow2normal(float x, float y) {
	float nx = 1.0 / (ARENA_SIZE) * (-y + (MIDDLE.y + ARENA_SIZE / 2.0));
	float ny = 1.0 / (ARENA_SIZE) * (x - (MIDDLE.x - ARENA_SIZE / 2.0));
	return vec2(nx * 2 - 1, ny * 2 - 1);
}

vec2_t wow2screen(float x, float y) {

	vec2_t n = wow2normal(x, y);

	float as = get_aspect_ratio();

	////vec2_t R = vec2((n.x * 2 - 1)/as, (n.y * 2 - 1));
	//vec2_t R = vec2(n.x * 2 - 1, n.y * 2 - 1);
	//vec2_t r = vec2(R.x / 5.0 - 0.8, R.y / 5.0);
	return n;
}

#define MAX_NUM_QUADS 64
#define SIZEOF_LOLVBUFFER (MAX_NUM_QUADS * 4 * 2) // 4 vertices per quad, 2 components per vertex
#define SIZEOF_LOLIBUFFER (MAX_NUM_QUADS * 3 * 2) // 3 indices per triangle, 2 triangles per quad

typedef struct tuple_t {
	vec2_t pos;
	float rot;
} tuple_t;

typedef struct quad_t {
	vec2_t v[4];
} quad_t;

quad_t pointtoquad(const tuple_t &t) {
	// ignore rot for now :D
	quad_t q;
	float as = get_aspect_ratio();
	static const float rectsize = 0.06;
	static const float hsx = rectsize;
	//static const float hsy = rectsize*as; // use this if rendering on the 16:9 back buffer
	static const float hsy = hsx;

	q.v[0] = vec2(t.pos.x - hsx, t.pos.y - hsy);
	q.v[1] = vec2(t.pos.x + hsx, t.pos.y - hsy);
	q.v[2] = vec2(t.pos.x + hsx, t.pos.y + hsy);
	q.v[3] = vec2(t.pos.x - hsx, t.pos.y + hsy);

	return q;

}

static int num_flames_to_render = 0;

static int update_lolbuffers();

static void render_flames(IDirect3DDevice9* d) {
	
	d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
	d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);

	d->SetRenderState(D3DRS_LIGHTING, FALSE);
	d->SetRenderState(D3DRS_ZENABLE, FALSE);

	d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	d->SetVertexDeclaration(vd);
	d->SetVertexShader(vs);
	d->SetPixelShader(ps);

	if (!update_lolbuffers()) return;
	static const float playercolor[] = { 0, 1, 0, 1.0 };
	static const float flamecolor[] = { 1, 0, 0, 1.0 };

	d->SetStreamSource(0, lol_vbuffer, 0, 2 * sizeof(float));
	d->SetIndices(lol_ibuffer);

	d->SetPixelShaderConstantF(0, playercolor, 1);
	d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);

	d->SetPixelShaderConstantF(0, flamecolor, 1);
	d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, num_flames_to_render * 4, 6, 2 * num_flames_to_render);

}

static int update_lolbuffers() {
	ObjectManager OM;
	if (!OM.valid()) return 0;
	
	WowObject i;
	if (!OM.get_first_object(&i)) return 0;

	std::vector<tuple_t> flames;

	while (i.valid()) {
		if (i.get_type() == OBJECT_TYPE_NPC) {
			if (i.NPC_get_name() == "Coldflame") {
				vec3 ipos = i.get_pos();
				vec2_t pos = wow2screen(ipos.x, ipos.y);
				flames.push_back({ pos, i.get_rot() });
			}
		}
		else if (i.get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
			if (i.DO_get_spellID() == 69146) {
				vec3 ipos = i.DO_get_pos();
				vec2_t pos = wow2screen(ipos.x, ipos.y);
				flames.push_back({ pos, i.get_rot() });
			}
		}
		i = i.next();
	}

	if (flames.size() < 1) { 
		num_flames_to_render = 0;
		return 1; 
	}

	quad_t *mem;
	lol_vbuffer->Lock(0, 0, (void**)&mem, 0);

	WowObject p;
	OM.get_local_object(&p);
	vec3 ppos = p.get_pos();
	
	vec2_t Ppos = wow2screen(ppos.x, ppos.y);
	quad_t pq = pointtoquad({ Ppos, 0 });
	mem[0] = pq;

	int n = 1;
	for (auto &f : flames) {
		quad_t q = pointtoquad(f);
		mem[n] = q;
		++n;
	}
	lol_vbuffer->Unlock();
	num_flames_to_render = n;

	return 1;

}

typedef struct texquad_vertex {
	float pos[2];
	float uv[2];
} texquad_vertex;

static int create_lolbuffers(IDirect3DDevice9 *d) {
	
	HRESULT hr;

	hr = d->CreateVertexBuffer(SIZEOF_LOLVBUFFER * sizeof(float), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &lol_vbuffer, NULL);
	hr = d->CreateIndexBuffer(SIZEOF_LOLIBUFFER * sizeof(UINT16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &lol_ibuffer, NULL);

	void *mem;

	lol_vbuffer->Lock(0, 0, &mem, 0);
	memset(mem, 0, SIZEOF_LOLVBUFFER * sizeof(float));
	lol_vbuffer->Unlock();

	static const int num_lolindices = SIZEOF_LOLIBUFFER;
	static UINT16 lol_indices[num_lolindices];
	int N = 0;
	for (int i = 0; i < num_lolindices; i += 6) {
		lol_indices[i] = N;
		lol_indices[i + 1] = N + 1;
		lol_indices[i + 2] = N + 2;

		lol_indices[i + 3] = N + 2;
		lol_indices[i + 4] = N;
		lol_indices[i + 5] = N + 3;
		N += 4;
	}

	lol_ibuffer->Lock(0, 0, &mem, 0);
	memcpy(mem, lol_indices, sizeof(lol_indices));
	lol_ibuffer->Unlock();


	hr = d->CreateVertexBuffer(4 * sizeof(texquad_vertex), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &lol_quadbuffer, NULL);
	
	const float as = 1440.0 / 900.0;
	const float size = 0.5;
	vec2_t mp_llc = vec2(-0.92, -0.4);

	texquad_vertex tq[4] = {
		{mp_llc.x, mp_llc.y, 0, 1 },
		{mp_llc.x + size, mp_llc.y, 1, 1},
		{mp_llc.x + size, mp_llc.y + size*as, 1, 0},
		{mp_llc.x, mp_llc.y + size*as, 0, 0}
	};
	lol_quadbuffer->Lock(0, 0, &mem, 0);
	memcpy(mem, &tq, sizeof(tq));
	lol_quadbuffer->Unlock();

	return 1;

}

static int create_d3d9buffers(IDirect3DDevice9 *d) {
	HRESULT hr;

	hr = d->CreateVertexBuffer(4 * 2 * sizeof(float), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vbuffer, NULL);
	if (FAILED(hr)) {
		PRINT("CreateVertexBuffer failed: %X\n", hr);
		return 0;
	}

	hr = d->CreateIndexBuffer(5 * sizeof(UINT16), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibuffer, NULL);
	if (FAILED(hr)) {
		PRINT("CreateIndexBuffer failed: %X\n", hr);
		return 0;
	}

	create_lolbuffers(d);

	buffers_initialized = 1;
	return 1;
}

static void populate_d3d9buffers() {

	if (!buffers_initialized) return;

	float vertices[2 * 4];
	rect_to_vertices(get_selection_rect(), vertices);

	float *V = &vertices[0];

	void *mem;
	vbuffer->Lock(0, 0, &mem, 0);
	memcpy(mem, vertices, 4 * 2 * sizeof(float));
	vbuffer->Unlock();

	static const UINT16 indices[] = {
		0, 1, 2, 3, 0
	};

	ibuffer->Lock(0, 0, &mem, 0);
	memcpy(mem, indices, sizeof(indices));
	ibuffer->Unlock();

}

static void free_d3d9buffers() {

	if (!buffers_initialized) return;

	vbuffer->Release();
	vbuffer = NULL;
	ibuffer->Release();
	ibuffer = NULL;

	lol_vbuffer->Release();
	lol_vbuffer = NULL;
	lol_ibuffer->Release();
	lol_ibuffer = NULL;

	lol_quadbuffer->Release();
	lol_quadbuffer = NULL;

	rendertex_surf->Release();
	rendertex->Release();

	vs->Release();
	ps->Release();

	quad_vs->Release();
	quad_ps->Release();

	vd->Release();
	quad_vd->Release();

	buffers_initialized = 0;
}

void cleanup_custom_d3d() {
	free_d3d9buffers();
}

void reset_renderstate() {
	IDirect3DDevice9 *d = (IDirect3DDevice9*)get_wow_d3ddevice();
	if (!d) return;

	d->SetRenderState(D3DRS_ZENABLE, TRUE);
	d->SetRenderTarget(0, back_buffer);
	d->SetDepthStencilSurface(depthstencil); // for  some reason, this is required
}

void draw_lolstuffXD() {
	
	if (!customd3d_initialized) return;

	IDirect3DDevice9 *d = (IDirect3DDevice9*)get_wow_d3ddevice();
	if (!d) return;

	//d->SetRenderTarget(0, rendertex_surf);
	//d->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(1, 1, 1), 1.0f, 0);
	//d->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	//d->SetRenderTarget(0, back_buffer);
	//d->SetDepthStencilSurface(depthstencil);
	
	d->SetRenderTarget(0, rendertex_surf);
	d->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(200, 0, 0, 0), 1.0f, 0);
	render_flames(d);

	//d->SetRenderTarget(0, back_buffer);
	//render_flames(d);

	d->SetRenderTarget(0, back_buffer);
	d->SetDepthStencilSurface(depthstencil);
	
	d->SetVertexDeclaration(quad_vd);
	d->SetVertexShader(quad_vs);
	d->SetPixelShader(quad_ps);

	d->SetStreamSource(0, lol_quadbuffer, 0, 4 * sizeof(float));
	
	d->SetTexture(0, rendertex);
	
	d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2); 

	reset_renderstate();

}


void draw_custom_d3d() {
	if (!customd3d_initialized) return;
	if (!rect_active) return;

	IDirect3DDevice9 *d = (IDirect3DDevice9*)get_wow_d3ddevice();
	if (!d) return;

	populate_d3d9buffers();

	d->SetStreamSource(0, vbuffer, 0, 2 * sizeof(float));
	d->SetIndices(ibuffer);

	// we're drawing the selection rectangle onto the back buffer
	d->SetRenderTarget(0, back_buffer);
	d->SetDepthStencilSurface(depthstencil);

	static const float rcolor[] = { 0, 1, 0, 1 };
	d->SetPixelShaderConstantF(0, rcolor, 1);

	d->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, 4, 0, 4);

	//d->SetViewport(&viewport_original);
}

int init_custom_d3d() {
 
	if (customd3d_initialized) return 1;

	IDirect3DDevice9 *d = (IDirect3DDevice9*)get_wow_d3ddevice();

	if (!d) return 0;

	HRESULT hr;
	hr = d->CreateVertexShader((DWORD*)VSbuf, &vs);
	if (FAILED(hr)) {
		PRINT("CreateVertexShader failed: %X\n", hr);
		return 0;
	}
	

	hr = d->CreatePixelShader((DWORD*)PSbuf, &ps);
	if (FAILED(hr)) {
		PRINT("CreatePixelShader failed: %X\n", hr);
		return 0;
	}


	hr = d->CreateVertexDeclaration(vdecl, &vd);
	if (FAILED(hr)) {
		PRINT("CreateVertexDeclaration failed: %X\n", hr);
		return 0;
	}

	hr = d->CreateVertexDeclaration(quad_vdecl, &quad_vd);
	hr = d->CreateVertexShader((DWORD*)g_vs30_main, &quad_vs);
	hr = d->CreatePixelShader((DWORD*)g_ps30_main, &quad_ps);

	// having these two (Create[Vertex|Index]Buffer) calls here in the init function cause a hang when resizing the window

	if (!buffers_initialized) {
		create_d3d9buffers(d);
	}

	hr = d->GetRenderTarget(0, &back_buffer);
	assert(SUCCEEDED(hr));

	hr = d->GetDepthStencilSurface(&depthstencil);
	assert(SUCCEEDED(hr));

	hr = d->CreateTexture(128, 128, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &rendertex, NULL);
	assert(SUCCEEDED(hr));

	hr = rendertex->GetSurfaceLevel(0, &rendertex_surf);
	assert(SUCCEEDED(hr));


	customd3d_initialized = 1;

	return 1;
}

