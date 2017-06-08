#include <queue>
#include <cassert>
#include <unordered_map>
#include <d3d9.h>
#include "hooks.h"
#include "addrs.h"
#include "defs.h"
#include "opcodes.h"
#include "ctm.h"
#include "creds.h"
#include "timer.h"
#include "lua.h"
#include "linalg.h"
#include "patch.h"

//static HRESULT(*EndScene)(void);
pipe_data PIPEDATA;

static hookable_t *find_hookable(const std::string &funcname);

extern HWND wow_hWnd;

const UINT32 PIPE_PROTOCOL_MAGIC = 0xAB30DD13;

static POINT cursor_pos;
static RECT client_area;

static std::unordered_map<GUID_t, std::string> selected_units;

static DWORD get_wow_d3ddevice() {
#define STATIC_335_DIRECT3DDEVICE 0xC5DF88 
#define STATIC_335_D3DDEVICE_OFFSET 0x397C
	DWORD wow_static_DX9 = DEREF(STATIC_335_DIRECT3DDEVICE);

	if (!wow_static_DX9) return 0;

	DWORD tmp1 = DEREF(wow_static_DX9 + STATIC_335_D3DDEVICE_OFFSET);
	DWORD d3ddevice = tmp1;

	return d3ddevice;
}

static DWORD get_EndScene() {
	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD EndScene = DEREF(DEREF(wowd3d) + 0xA8);

	return EndScene;
}

static DWORD get_Present() {

	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD Present = DEREF(DEREF(wowd3d) + 0x44);
	return Present;
}

static DWORD get_DrawIndexedPrimitive() {
	DWORD wowd3d = get_wow_d3ddevice();
	if (!wowd3d) return 0;

	DWORD DrawIndexedPrimitive = DEREF(DEREF(wowd3d) + 0x148);
	return DrawIndexedPrimitive;
}

template <typename T> trampoline_t &trampoline_t::operator << (const T& arg) {
	int size = sizeof(arg);
	BYTE buf[16];
	memcpy(buf, &arg, size);

	//for (int i = 0; i < 24; ++i) {
	//	PRINT("%02X ", p.bytes[i]);
	//}
	//PRINT("\n(before)\n");

	for (int i = 0; i < size; ++i) {
		bytes[this->length] = buf[i];
		++this->length;
	}

	//for (int i = 0; i < 24; ++i) {
	//	PRINT("%02X ", p.bytes[i]);
	//}
	//PRINT("\n(after)\n\n");

	return (*this);
}


void trampoline_t::append_relative_offset(DWORD offset) {

	DWORD jump = offset - ((DWORD)bytes + length) - 4;
	//PRINT("offset: %X, length: %ld, jump: 0x%X\n", offset, length, jump);
	(*this) << jump;

}

void trampoline_t::append_CALL(DWORD funcaddr) {
	(*this) << (BYTE)0xE8;
	append_relative_offset(funcaddr);
}

void trampoline_t::append_bytes(const BYTE* b, int size) {
	for (int i = 0; i < size; ++i) {
		(*this) << b[i];
	}
}


static std::queue<std::string> esscripts;

void esscript_add(const std::string &script) {
	esscripts.push(script);
}

static void esscript_execute() {
	while (!esscripts.empty()) {
		const std::string &s = esscripts.front();
		DoString(s.c_str());
		esscripts.pop();
	}
}


static void register_luafunc_if_not_registered() {
	if (!lua_registered) {
		register_lop_exec();
	}
}

static void update_hwevent_tick() {
	typedef int tick_count_t(void);
	//int ticks = ((tick_count_t*)GetOSTickCount)();
	DWORD ticks = *(DWORD*)CurrentTicks;

	*(DWORD*)(LastHardwareAction) = ticks;
	// this should make us immune to AFK ^^
}

static RECT window_rect;

static int get_window_width() {
	return window_rect.right - window_rect.left;
}

static int get_window_height() {
	return window_rect.bottom - window_rect.top;
}

#define SMAX 0.7
#define SMIN 0.4

static struct {
	float s;
	float maxdistance;

	glm::vec4 pos;

	glm::vec4 get_cameraoffset() {
		// y = z^2
		return maxdistance*glm::vec4(0, 2*s*s, s, 1.0);
	}
	float get_angle() {
		//derivative dy/dz: y = 2z
		return s*1.57;
	}
	void increment() {
		s += 0.01;
		s = s > SMAX ? SMAX : s;
	}
	void decrement() {
		s -= 0.01;
		s = s < SMIN ? SMIN : s;
	}

} customcamera = { 0.5, 30, glm::vec4(0, 0, 0, 1) };

static void update_camera_rotation(wow_camera_t *camera) {

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), -customcamera.get_angle(), glm::vec3(0, 1.0, 0));
	set_wow_rot(rot);
}

static void move_camera_if_cursor() {

	if (GetActiveWindow() != wow_hWnd) return;

	if (cursor_pos.x < 0 || cursor_pos.x > get_window_width()) return;
	if (cursor_pos.y < 0 || cursor_pos.y > get_window_height()) return;

	//PRINT("camera: 0x%X\n", camera);

	const float dd = 0.1;
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

static void RIP_camera() {
	// [[B7436C] + 7E20]
	// FOV is at camera + 0x40
	// zNear is at 0x38, zFar is 0x3C
	DWORD nop = 0x90909090;

	DWORD *a1 = (DWORD*)0x6075AB;
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

	GetClientRect(wow_hWnd, &window_rect);

	update_wowcamera();
	//move_camera_if_cursor();


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
	
	glm::vec4 newpos = customcamera.pos+customcamera.get_cameraoffset() + glm::vec4(0, 3, -6, 1);

	glm::vec4 nw = glm2wow(newpos);

	camera->x = nw.x;
	camera->y = nw.y;
	camera->z = nw.z;

	//update_camera_rotation(camera);

}


static void mouse_stuff() {
	// found using GetCursorPos breakpoints!!
	// handler for WM_L/RBUTTONDOWN is at 869870
	// WM_LBUTTONUP at 869C00 
	
}

static int rect_active = 0;
static POINT rect_begin;

static void draw_rect(RECT *r, D3DLOCKED_RECT *dr, DWORD XRGB) {

	BYTE *b = (BYTE*)dr->pBits;

	for (int i = r->left; i < r->right; ++i) {
		memcpy(&b[r->top*dr->Pitch + i*4], &XRGB, sizeof(XRGB)); // top horizontal
		memcpy(&b[r->bottom*dr->Pitch + i*4], &XRGB, sizeof(XRGB)); // bottom horizontal
	}

	for (int i = r->top; i < r->bottom; ++i) {
		memcpy(&b[r->left*4 + i*dr->Pitch], &XRGB, sizeof(XRGB)); // left vertical
		memcpy(&b[r->right*4 + i*dr->Pitch], &XRGB, sizeof(XRGB)); // right vertical
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

	
	memset(&b[r.Pitch * y + 4*x], 0xFF, 4);


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

	// NOTE: EVERY OCCURRENCE OF X AND Y COORDINATES ARE INTENTIONALLY SWAPPED (WOW WORKS THIS WAY O_O)
	glm::vec3 cpos = wow2glm(glm::vec3(c->x, c->y, c->z));

	glm::mat4 proj = glm::perspective(c->fov, c->aspect, c->zNear, c->zFar);
	glm::mat4 view = glm::translate(glm::mat4(1.0), -cpos);

	glm::mat4 rot = glm::rotate(glm::mat4(1.0), customcamera.get_angle(), glm::vec3(1.0, 0, 0));
	glm::mat4 nMVP = proj*(rot*view);

	glm::vec4 nclip = nMVP*up;
	nclip /= nclip.w;

	dump_glm_vec4((rot*view)*up);
	dump_glm_vec4(nclip);

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

static void hook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
h->patch.enable();
}

static void unhook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
	h->patch.disable();
}

typedef struct rdmpheader_t {
	DWORD MAGIC;
	DWORD pitch;
	DWORD width;
	DWORD height;
	DWORD stagenum;
	DWORD callstack[8];
} rmpheader_t;


static struct {
	int active;
	int need_to_stop;
	int drawcall_count;
	FILE *capture_outfile;
	std::string name_base;
	int file_index;

	int open_dumpfile(const std::string &name) {

		fopen_s(&capture_outfile, name.c_str(), "wb");
		if (!capture_outfile) {
			PRINT("Couldn't open frame dump file %s!\n", name.c_str());
			return 0;
		}

		PRINT("Opened frame dump file \"%s\"!\n", name.c_str());

		return 1;

	}

	void reset() {
		active = 0;
		need_to_stop = 0;
		drawcall_count = 0;
		if (capture_outfile) {
			fclose(capture_outfile);
			capture_outfile = NULL;
		}
		name_base = "";
		file_index = 1;

	}

	inline std::string get_chunkpostfix(int index) {
		char buf[128];
		sprintf_s(buf, "_chunk%02d", file_index);

		return std::string(buf);
	}

	inline std::string get_basename() {
		SYSTEMTIME t;
		GetLocalTime(&t);

		char buf[128];
		sprintf_s(buf, "D:\\wowframe\\%02d%02d_%02d%02d%02d.rdmp", t.wDay, t.wMonth, t.wHour, t.wMinute, t.wSecond);

		return std::string(buf);
	}

	void start() {

		reset();

		name_base = get_basename();

		if (open_dumpfile(name_base + get_chunkpostfix(file_index))) {
			hook_DrawIndexedPrimitive();
			need_to_stop = 1; // this is to signal Present to unhook shit
			active = 1;
		}
	}

	void finish() {
		PRINT("Frame dump %s complete! (%d stages in %d chunks)\n", name_base.c_str(), drawcall_count, file_index);
		unhook_DrawIndexedPrimitive();
		reset();
	}

	int new_chunk() {
		++file_index;
		fclose(capture_outfile);
		if (!open_dumpfile(name_base + get_chunkpostfix(file_index))) {
			PRINT("new_chunk failed!\n");
			capture_outfile = NULL;
			return 0;
		}
		return 1;
	}

	int append_to_file(const rdmpheader_t *hdr, const LPVOID data, size_t data_size_bytes) {
#define MAX_CHUNKSIZE (1 << 31)
		if (!capture_outfile) return 0;

		if (ftell(capture_outfile) + data_size_bytes > MAX_CHUNKSIZE) {
			if (!new_chunk()) return 0;
		}
		
		fwrite(hdr, sizeof(rdmpheader_t), 1, capture_outfile);
		fwrite(data, 1, data_size_bytes, capture_outfile);

		return 1;
	}

} capture;

void enable_capture_render() {
	capture.active = 1;
}

static void __stdcall call_pylpyr() {
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

	//PRINT("called pylpyr for %llX\n", GUID);

}

static void __stdcall Present_hook() {

	GetCursorPos(&cursor_pos);
	ScreenToClient(wow_hWnd, &cursor_pos);
	GetClientRect(wow_hWnd, &client_area);

	if (rect_active) {
		draw_rect_brute();
	}

	register_luafunc_if_not_registered();

	static timer_interval_t fifty_ms(50);
	static timer_interval_t half_second(500);

	RIP_camera();

	if (half_second.passed()) {
		update_hwevent_tick();

		if (credentials.valid && !credentials.logged_in) credentials.try_login();

		half_second.reset();
	}

	ctm_handle_delayed_posthook();
	ctm_update_prevpos();
	ctm_abort_if_not_moving();

	if (noclip_enabled) {
		if (since_noclip.get_ms() > 250) {
			disable_noclip();
		}
	}


	if (fifty_ms.passed()) {
		ctm_purge_old();

		refollow_if_needed();
		ctm_act();

		fifty_ms.reset();
	}

	//mat4 view, proj;
	//get_wow_view_matrix(&view);
	//get_wow_proj_matrix(&proj);

	if (capture.active) {
		capture.start();
		capture.active = 0;
	}
	else if (capture.need_to_stop) {
		capture.finish();
	}




	//__asm {
	//	int 3;
	//	pushad;
	//	push 0x0C76;
	//	push 0x9F9880;
	//	push 1;
	//	//push 0;
	//	//push 8;
	//	push 0xF1300012;
	//	push 0x3C024E24;
	//	mov edi, 0x4D4DB0;
	//	call edi;
	//	add esp, 0x14;
	//	cmp eax, 0;

	//	jz invalid_guid;
	//	mov edx, dword ptr ds:[eax];
	//	mov ecx, eax;
	//	mov eax, dword ptr ds : [edx];
	//	call eax;

	//invalid_guid:
	//	popad;

	//}

}


static void __stdcall EndScene_hook() {
}

static void __stdcall dump_packet(BYTE *packet) {

	enum {
		CMSG_CAST_SPELL = 0x12E,
		CMSG_SET_SELECTION = 0x13D,
	};

	DWORD length = (DWORD)(packet[0]) << 8;
	length += packet[1] + 2; // +2 for the length bytes themselves (not included apparently)

	DWORD opcode = 0;
	memcpy(&opcode, &packet[2], 2); 
	// ^ interestingly, the client packets have 4 bytes for the opcode while the server packets only have 2.
	// the largest opcode value in mangos/src/game/Opcodes.h is something like 0x500 so no worries

	//PRINT("packet length: total: %d (actual data size without header: %d), opcode: 0x%X\n", length, length - 6, opcode);


	if (opcode == CMSG_CAST_SPELL) {

		BYTE cast_count = packet[6];
		DWORD spellID;
		memcpy(&spellID, &packet[7], 4);
		BYTE flags = packet[11];
		PRINT("got CMSG_SPELL_CAST. packet length: %d, spellID = %d, cast_count = %d, flags = 0x%X\n raw dump:\n", length, spellID, cast_count, flags);
		for (int i = 0; i < length; ++i) {
			PRINT("%02X ", packet[i]);
		}
		PRINT("\n\n");
	}
	else {
		PRINT("opcode: %X\n", opcode);
		for (int i = 6; i < length; ++i) {
			PRINT("%02X ", packet[i]);
		}
		PRINT("\n\n");
	}
}

static void __stdcall ClosePetStables_hook() {
	lua_registered = 0;
}

static void __stdcall broadcast_CTM(float *coords, int action) {

	float x, y, z;

	x = coords[0];
	y = coords[1];
	z = coords[2];

	PRINT("broadcast_CTM: got CTM coords: (%f, %f, %f), action = %d\n", x, y, z, action);

	char sprintf_buf[128];

	sprintf_s(sprintf_buf, "%.1f %.1f %.1f", x, y, z);

	// the CTM mode is determined in the LUA logic, in the subcommand handler.

	DoString("RunMacroText(\"/lole broadcast ctm %s\")", sprintf_buf);
}


static void __stdcall CTM_finished_hookfunc() {

	CTM_t *c = ctm_get_current_action();
	if (!c) { return; }
	
	PRINT("called CTM_finished with ID = %ld\n", c->ID);

	c->handle_posthook();
}

static const trampoline_t *prepare_EndScene_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing EndScene patch...\n");

	DWORD EndScene = get_EndScene();
	PRINT("Found EndScene at 0x%X\n", EndScene);
	p->patch_addr = EndScene;
	
	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)EndScene_hook);
	tr << (BYTE)0x61; // POPAD
		
	memcpy(p->original, (LPVOID)EndScene, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << (BYTE)0xC3; // RET
	
	return &tr;

}




static const trampoline_t *prepare_ClosePetStables_patch(patch_t *p) {

	static trampoline_t tr;

	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)ClosePetStables_hook);
	tr << (BYTE)0x61; // POPAD

	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << (BYTE)0xC3; // RET
	
	return &tr;

}


//static trampoline_t prepare_CTM_main_patch(LPVOID hook_func_addr) {
//	PRINT("Preparing CTM_main patch...\n");
//
//	static BYTE CTM_main_trampoline[] = {
//		// original opcodes from 0x612A90 "CTM_main"
//
//		0x55, // PUSH EBP
//		0x8B, 0xEC, // MOV EBP, ESP
//		0x83, 0xEC, 0x18, // SUB ESP, 18
//
//		0x68, 0x00, 0x00, 0x00, 0x00, // push early return address (performs the 612A90 function normally)
//		0x60,						// pushad
//
//		0x8B, 0x75, 0x8, // MOV ESI, DWORD PTR SS:[ARG.2] CTM_"push" or "action"
//		0x56,			// PUSH ESI
//
//		0xE8, 0x00, 0x00, 0x00, 0x00, // call to get_CTM_retaddr. eax now has 1 or 0, depending on the env
//		0x85, 0xC0,	// test EAX EAX
//		0x74, 0x0F, // jz, hopefully to the second popad part
//		0x8B, 0x75, 0x8, // MOV ESI, DWORD PTR SS:[ARG.2] CTM_"push" or "action"
//		0x56,
//		0x8B, 0x75, 0x10, // MOV ESI, DWORD PTR SS:[ARG.3] (contains the 3 floatz ^^)
//		0x56,			  // PUSH ESI (arg to CTM_broadcast)
//		0xE8, 0x00, 0x00, 0x00, 0x00, // call CTM_broadcast function :D loloz
//		0x61, // popad
//		0xC3, // ret
//		
//		// branch #2
//
//		0x61, // popad
//		0x83, 0xC4, 0x04, // add esp, 4, to "pop" without screwing up reg values
//		0x68, 0x00, 0x00, 0x00, 0x00, // push return late late
//		0xC3 //ret
//	};
//
//	static const uint retaddr_early = CTM_main + 0x6, retaddr_late = CTM_main + 0x12E;
//
//	DWORD tr_offset = ((DWORD)CTM_main_trampoline - (DWORD)CTM_main - 5);
//	memcpy(CTM_main_patch + 1, &tr_offset, sizeof(tr_offset));
//
//	memcpy(CTM_main_trampoline + 7, &retaddr_late, sizeof(retaddr_late));
//
//	DWORD get_CTM_retaddr_offset = (DWORD)get_CTM_retaddr - (DWORD)CTM_main_trampoline - 21;
//	memcpy(CTM_main_trampoline + 17, &get_CTM_retaddr_offset, sizeof(get_CTM_retaddr_offset));
//
//	memcpy(CTM_main_trampoline + 45, &retaddr_early, sizeof(retaddr_early));
//
//	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)CTM_main_trampoline - 38;
//	memcpy(CTM_main_trampoline + 34, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset
//
//	DWORD oldprotect;
//	VirtualProtect((LPVOID)CTM_main_trampoline, sizeof(CTM_main_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);
//
//	PRINT("OK.\nCTM_main trampoline: %p, hook_func_addr: %p\n", &CTM_main_trampoline, hook_func_addr);
//
//	return 1;
//}

static const trampoline_t *prepare_CTM_finished_patch(patch_t *p) {
	static trampoline_t tr;

	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)CTM_finished_hookfunc);
	tr << (BYTE)0x61; // POPAD

	tr.append_bytes(p->original, p->size);

// NOTE: COULD BE THIS INSTEAD OF (p->patch_addr + p->size): DWORD ret_addr = 0x7273EF;

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << (BYTE)0xC3; // RET

	return &tr;
}

static const trampoline_t *prepare_sendpacket_patch(patch_t *p) {

	static trampoline_t tr;
	
	PRINT("dump_packet offset: 0x%X, trampoline: 0x%X\n", dump_packet, tr.bytes);

	DWORD retaddr = 0x467773;
	DWORD some_crypt_func = 0x4665B0;

	tr << (BYTE)0x60 // PUSHAD
	   << (BYTE)0x52; // push EDX (the packet address)

	tr.append_CALL((DWORD)dump_packet);
	tr << (BYTE)0x61; // POPAD

	tr.append_CALL(some_crypt_func); // from the original opcodes
	
	tr << (BYTE)0x68 << retaddr; // push RET addr
	
	tr << (BYTE)0xC3; // RET

	return &tr;

}

static const trampoline_t *prepare_recvpacket_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("dump_packet offset: 0x%X, trampoline: 0x%X\n", dump_packet, tr.bytes);

	DWORD retaddr = 0x467ECD;

	tr << (BYTE)0x60; // PUSHAD

	tr << (BYTE)0x8B << (BYTE)0x46 << (BYTE)0x1C; // mov eax, dword ptr ds:[esi+1c] // decrypted packet address
	tr << (BYTE)0x50; // push eax

	tr.append_CALL((DWORD)dump_packet);
	tr << (BYTE)0x61; // POPAD

	// original stuff
	tr << (BYTE)0x01 << (BYTE)0x5E << (BYTE)0x20;
	tr << (BYTE)0x8B << (BYTE)0x4D << (BYTE)0xF8;

	tr << (BYTE)0x68 << retaddr; // push RET addr

	tr << (BYTE)0xC3; // RET

	return &tr;

}



static const trampoline_t *prepare_Present_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing Present patch...\n");

	DWORD Present = get_Present();
	PRINT("Found Present at 0x%X\n", Present);
	p->patch_addr = Present;

	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)Present_hook);
	tr << (BYTE)0x61; // POPAD
	
	memcpy(p->original, (LPVOID)Present, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << (BYTE)0xC3; // RET


	return &tr;
		
}

static void __stdcall DrawIndexedPrimitive_hook() {

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

	D3DSURFACE_DESC d;
	s->GetDesc(&d);

	rdmpheader_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.MAGIC = 0x700FF000;
	hdr.pitch = r.Pitch;
	hdr.width = d.Width;
	hdr.height = d.Height;
	hdr.stagenum = capture.drawcall_count;

	PRINT("stagenum: %d, pitch: %d\n", hdr.stagenum, hdr.pitch);

	CaptureStackBackTrace(0, 8, (PVOID*)&hdr.callstack[0], NULL); 

	capture.append_to_file(&hdr, r.pBits, r.Pitch * d.Height);

	s->UnlockRect();

	s->Release();
	sc->Release();

	++capture.drawcall_count;

}

static const trampoline_t *prepare_DrawIndexedPrimitive_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing DrawIndexedPrimitive patch...\n");

	DWORD DrawIndexedPrimitive = get_DrawIndexedPrimitive();
	PRINT("Found DrawIndexedPrimitive at 0x%X\n", DrawIndexedPrimitive);

	p->patch_addr = DrawIndexedPrimitive;
	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)DrawIndexedPrimitive_hook);
	tr << (BYTE)0x61; // POPAD

	memcpy(p->original, (LPVOID)DrawIndexedPrimitive, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << (BYTE)0xC3; // RET

	return &tr;

}

static void __stdcall mbuttondown_hook() {
	GetCursorPos(&rect_begin);
	ScreenToClient(wow_hWnd, &rect_begin);
	//PRINT("mbd_hook: %d, %d", rect_begin.x, rect_begin.y);
	rect_active = 1;
}

static const trampoline_t *prepare_mbuttondown_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttondown patch...\n");

	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)mbuttondown_hook);
	tr << (BYTE)0x61; // POPAD

	tr << (BYTE)0xC3; // just ret ^^

	return &tr;

}

static void __stdcall mbuttonup_hook() {
	rect_active = 0;

	get_units_in_selection_rect(get_selection_rect());

	if (selected_units.size() < 1) {
		DoString("RunMacroText(\"/lole clearselection\")");
		return;
	}

	PRINT("Units within rectselect bounds:\n");
	std::string units_concatd;

	for (auto &u : selected_units) {
		units_concatd = units_concatd + u.second + ",";
	}
	units_concatd.pop_back();

	DoString("RunMacroText(\"/lole setselection %s\")", units_concatd.c_str());
}

static const trampoline_t *prepare_mbuttonup_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttonup patch...\n");

	tr << (BYTE)0x60; // PUSHAD

	tr.append_CALL((DWORD)mbuttonup_hook);
	tr << (BYTE)0x61; // POPAD

	tr << (BYTE)0xC3; // just ret ^^

	return &tr;
}

static const trampoline_t *prepare_pylpyr_patch(patch_t *p) {

 // THE SELECTION RING DRAWING HAPPENS AT 7E41FB!!
 // 4F6F90 could be of interest :>

 // A BREAKPOINT AT 0x4F6FAB is the unique code path for pylpyr (fires ONLY if an unit is selected)
 // 725980 is some member function probably to draw a pylpyr under an unit / npc :D


	static trampoline_t tr;

	PRINT("Preparing pylpyr patch...\n");

	tr << (BYTE)0x60;
	tr.append_CALL((DWORD)call_pylpyr);

	tr << (BYTE)0x61; // POPAD
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << (BYTE)0xC3;

	return &tr;

}

static void __stdcall mwheel_hook(DWORD wParam) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (zDelta < 0) customcamera.increment();
	if (zDelta > 0) customcamera.decrement();
}

static const trampoline_t *prepare_mwheel_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mwheel patch...\n");

	tr << (BYTE)0x60;
	tr << (BYTE)0x8B << (BYTE)0x45 << (BYTE)0x10; // move wParam to EAX
	tr << (BYTE)0x50; // push eax
	tr.append_CALL((DWORD)mwheel_hook);

	tr << (BYTE)0x61; // POPAD
	tr << (BYTE)0x68 << (DWORD)0x86A8D1; // skip right to return
	tr << (BYTE)0xC3;

	return &tr;

}

static hookable_t hookable_functions[] = {
	//{ "EndScene", 0x0, EndScene_original, EndScene_patch, EndScene_original, prepare_EndScene_patch },
	//{ "ClosePetStables", (LPVOID)ClosePetStables, ClosePetStables_original, ClosePetStables_patch, ClosePetStables_original, prepare_ClosePetStables_patch },
	//{ "CTM_update", (LPVOID)CTM_update_hookaddr, CTM_finished_original, CTM_finished_patch, CTM_finished_original, prepare_CTM_finished_patch },
	//{ "SpellErrMsg", (LPVOID)SpellErrMsg, spell_errmsg_original, spell_errmsg_patch, spell_errmsg_original, prepare_spell_errmsg_patch },
	//{ "SendPacket", (LPVOID)SendPacket_hookaddr, sendpacket_original, sendpacket_patch, sendpacket_original, prepare_sendpacket_patch },
	//{ "RecvPacket", (LPVOID)RecvPacket_hookaddr, recvpacket_original, recvpacket_patch, recvpacket_original, prepare_recvpacket_patch },
	//{ "Present", (LPVOID)0x0, present_original, present_patch, present_original, prepare_present_patch },
	//{ "mbuttondown_handler", (LPVOID)mbuttondown_handler, mbuttondown_original, mbuttondown_patch, mbuttondown_patch, prepare_mbuttondown_patch },
	//{ "mbuttonup_handler", (LPVOID)mbuttonup_handler, mbuttonup_original, mbuttonup_patch, mbuttonup_patch, prepare_mbuttonup_patch },
	//{ "DrawIndexedPrimitive", 0x0, drawindexedprimitive_original, drawindexedprimitive_patch, drawindexedprimitive_patch, prepare_drawindexedprimitive_patch },

	{ "EndScene", patch_t(PATCHADDR_LATER, 7, prepare_EndScene_patch) },
	{ "Present", patch_t(PATCHADDR_LATER, 5, prepare_Present_patch) },
	{ "CTM_finished", patch_t(CTM_finished_patchaddr, 5, prepare_CTM_finished_patch) },
	{ "ClosePetStables", patch_t(ClosePetStables_patchaddr, 5, prepare_ClosePetStables_patch) },
	{ "mbuttondown_handler", patch_t(mbuttondown_handler, 5, prepare_mbuttondown_patch) },
	{ "mbuttonup_handler", patch_t(mbuttonup_handler, 5, prepare_mbuttonup_patch) },
	{ "DrawIndexedPrimitive", patch_t(PATCHADDR_LATER, 5, prepare_DrawIndexedPrimitive_patch) },
	{ "pylpyr", patch_t(pylpyr_patchaddr, 9, prepare_pylpyr_patch) },
	{ "mwheel_handler", patch_t(mwheel_hookaddr, 6, prepare_mwheel_patch) },

};

static hookable_t *find_hookable(const std::string &funcname) {
	for (auto &h : hookable_functions) {
		if (h.funcname == funcname) {
			return &h;
		}
	}
	return NULL;
}

static int get_patch_from_hookable(const std::string &funcname, patch_serialized *p) {
	hookable_t *hook = find_hookable(funcname);
	if (!hook) {
		PRINT("get_patch_from_hookable: find_hookable for %s failed, expecting a disaster.\n", funcname.c_str());
		return 0;
	}

	*p = patch_serialized(hook->patch.patch_addr, hook->patch.size, hook->patch.original, hook->patch.patch);
	return 1;
}

int prepare_pipe_data() {
	
	patch_serialized p;

#define ADD_PATCH_SAFE(patchname) do { assert(get_patch_from_hookable(patchname, &p)); PIPEDATA.add_patch(p); } while(0)

	ADD_PATCH_SAFE("EndScene");
	ADD_PATCH_SAFE("Present");
	ADD_PATCH_SAFE("CTM_finished");
	ADD_PATCH_SAFE("ClosePetStables");
	ADD_PATCH_SAFE("mbuttondown_handler");
	ADD_PATCH_SAFE("mbuttonup_handler");
	ADD_PATCH_SAFE("pylpyr");
	ADD_PATCH_SAFE("mwheel_handler");
	
	// don't add DrawIndexedPrimitive to this list, it will be patched manually later by a /lole subcommand
	//ADD_PATCH_SAFE("DrawIndexedPrimitive");


	//PIPEDATA.add_patch(get_patch_from_hookable("mbuttondown_handler"));
	//PIPEDATA.add_patch(get_patch_from_hookable("mbuttonup_handler"));
	//PIPEDATA.add_patch(get_patch_from_hookable("DrawIndexedPrimitive"));
	//	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("CTM_main"));

	//	PIPEDATA.add_patch(get_patch_from_hookable("SendPacket"));
	//	PIPEDATA.add_patch(get_patch_from_hookable("RecvPacket"));
	return 1;
}


patch_serialized::patch_serialized(UINT32 patch_addr, UINT32 patch_size, const BYTE *original_opcodes, const BYTE *patch_opcodes) {
	buffer_size = 2 * sizeof(UINT32) + 2 * patch_size;
	
	buffer = new BYTE[buffer_size];

	memcpy(buffer + 0 * sizeof(UINT32), &patch_addr, sizeof(UINT32));
	memcpy(buffer + 1 * sizeof(UINT32), &patch_size, sizeof(UINT32));
	memcpy(buffer + 2 * sizeof(UINT32), original_opcodes, patch_size);
	memcpy(buffer + 2 * sizeof(UINT32) + patch_size, patch_opcodes, patch_size);

}

int pipe_data::add_patch(const patch_serialized &p) {
	data.insert(data.end(), &p.buffer[0], &p.buffer[p.buffer_size]);
	++num_patches;
	memcpy(&data[4], &num_patches, sizeof(UINT32));

	return num_patches;
}

