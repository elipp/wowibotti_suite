#include <Windows.h>
#include <unordered_map>
#include <string>

#include "wc3mode.h"
#include "wowmem.h"
#include "linalg.h"
#include "hooks.h"

static POINT cursor_pos;
static RECT client_area;

static RECT window_rect;
static std::unordered_map<GUID_t, std::string> selected_units;

static int get_window_width() {
	return window_rect.right - window_rect.left;
}

static int get_window_height() {
	return window_rect.bottom - window_rect.top;
}

static int rect_active = 0;
static POINT rect_begin;

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

customcamera_t customcamera = { 0.5, 30, glm::vec4(0, 0, 0, 1) };

static void update_camera_rotation(wow_camera_t *camera) {

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), -customcamera.get_angle(), glm::vec3(0, 1.0, 0));
	set_wow_rot(rot);
}

static void move_camera_if_cursor() {

	if (GetActiveWindow() != wow_hWnd) return;

	if (rect_active) return;

	if (cursor_pos.x < 0 || cursor_pos.x > get_window_width()) return;
	if (cursor_pos.y < 0 || cursor_pos.y > get_window_height()) return;

	//PRINT("camera: 0x%X\n", camera);

	const float dd = 0.1*((1.0-SMIN) + customcamera.get_s());
	const int margin = 150;

	int ww = get_window_width();
	int wh = get_window_height();

	//PRINT("ww: %d, wh: %d\n", ww, wh);

	if (cursor_pos.x < margin) {
		customcamera.pos.x -= dd;
	}
	else if (cursor_pos.x > ww - margin) {
		customcamera.pos.x += dd;
	}

	if (cursor_pos.y < margin) {
		customcamera.pos.z -= dd;
	}
	else if (cursor_pos.y > wh - margin) {
		customcamera.pos.z += dd;
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

static const int camera_patchsize = 0x6075EB - CAMERAPATCH_BASE;

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

	ObjectManager OM;
	WowObject o;
	if (!OM.get_local_object(&o)) {
		return;
	}

	vec3 pos = o.get_pos();

	wow_camera_t *camera = (wow_camera_t*)get_wow_camera();
	if (!camera) return;

	customcamera.pos = wow2glm(glm::vec4(pos.x, pos.y, pos.z, 1.0));

	glm::vec4 newpos = customcamera.pos + customcamera.get_cameraoffset() + glm::vec4(0, 3, -6, 1);

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

	r->left = CLAMP(r->left, (LONG)0, client_area.right - 1);
	r->right = CLAMP(r->right, (LONG)0, client_area.right - 1);
	r->top = CLAMP(r->top, (LONG)0, client_area.bottom - 1);
	r->bottom = CLAMP(r->bottom, (LONG)0, client_area.bottom - 1);

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

	if (FAILED(s->LockRect(&r, NULL, D3DLOCK_DONOTWAIT))) {
		PRINT("%d LockRect failed\n", GetTickCount());
		return;
	}

	draw_rect(&mr, &r, D3DCOLOR_XRGB(0, 255, 0));

	//D3DSURFACE_DESC d;
	//s->GetDesc(&d);

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
	up.y += 0.8;

	glm::vec3 cpos = wow2glm(glm::vec3(c->x, c->y, c->z));

	glm::mat4 proj = glm::perspective(c->fov, c->aspect, c->zNear, c->zFar);
	glm::mat4 view = glm::translate(glm::mat4(1.0), -cpos);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), customcamera.get_angle(), glm::vec3(1.0, 0, 0));
	glm::mat4 nMVP = proj*(rot*view);

	glm::vec4 nclip = nMVP*up;
	nclip /= nclip.w;

	//dump_glm_vec4((rot*view)*up);
	//dump_glm_vec4(nclip);

	PRINT("\n------------------\n");

	*coords = map_clip_to_screen(nclip);

	return 1;
}

static int get_units_in_selection_rect(RECT sel) {

	selected_units.clear();

	ObjectManager OM;
	WowObject iter;

	if (!OM.get_first_object(&iter)) return 0;

	while (iter.valid()) {
		if (iter.get_type() == OBJECT_TYPE_UNIT) {
			POINT coords;
			memset(&coords, 0, sizeof(coords));
			GUID_t unitguid = iter.get_GUID();
			get_screen_coords(unitguid, &coords);

			//PRINT("checking unit 0x%016llX (screen coords: %d, %d), selection rect data: (t: %ld, b: %ld, l: %ld, r: %ld)\n", 
			//	unitguid, coords.x, coords.y, sel.top, sel.bottom, sel.left, sel.right);

			if (coords.x > sel.left && coords.x < sel.right
				&& coords.y < sel.bottom && coords.y > sel.top) {
				selected_units[unitguid] = iter.unit_get_name();
				//PRINT("^UNIT IN RECT!\n");
			}

		}
		iter = iter.next();

	}

	return selected_units.size();

}

void do_wc3mode_stuff() {

	if (!patches_prepared) wc3mode_prepare_camera_patches();

	GetCursorPos(&cursor_pos);
	ScreenToClient(wow_hWnd, &cursor_pos);
	GetClientRect(wow_hWnd, &client_area);

	if (rect_active) {
		draw_rect_brute();
	}

	GetClientRect(wow_hWnd, &window_rect);
	update_wowcamera();

}

void wc3mode_mouseup_hook() {
	rect_active = 0;
	get_units_in_selection_rect(get_selection_rect());

	if (selected_units.size() < 1) {
		DoString("RunMacroText(\"/lole clearselection\")");
		return;
	}

	std::string units_concatd;

	for (auto &u : selected_units) {
		units_concatd = units_concatd + u.second + ",";
	}
	units_concatd.pop_back();

	DoString("RunMacroText(\"/lole setselection %s\")", units_concatd.c_str());
}

static int wc3_enabled = 0;

int wc3mode_enabled() {
	return wc3_enabled;
}

void enable_wc3mode(int b) {
	PRINT("enable_wc3mode: b = %d\n", b);
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
	GetCursorPos(&rect_begin);
	ScreenToClient(wow_hWnd, &rect_begin);
	rect_active = 1;
}

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
