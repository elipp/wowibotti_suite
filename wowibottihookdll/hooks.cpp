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
#include "dipcapture.h"
#include "wc3mode.h"
#include "input.h"

//static HRESULT(*EndScene)(void);
pipe_data PIPEDATA;

const UINT32 PIPE_PROTOCOL_MAGIC = 0xAB30DD13;

#define PUSHAD (BYTE)0x60
#define POPAD (BYTE)0x61
#define RET (BYTE)0xC3

template <typename T> trampoline_t &trampoline_t::operator << (const T& arg) {
	int size = sizeof(arg);
	BYTE buf[16];
	memcpy(buf, &arg, size);

	for (int i = 0; i < size; ++i) {
		bytes[this->length] = buf[i];
		++this->length;
	}

	return (*this);
}

trampoline_t & trampoline_t::append_hexstring(const char *hexstr) {

	// this expects the string to be in the format FFEEDDAA3355 etc.

	int length = strlen(hexstr);

	assert((length & 1) == 0);

	int i = 0;
	while (i < length) {
		BYTE r;
		sscanf(hexstr + i, "%2hhX", &r);
		bytes[this->length] = r;
		++this->length;
		i += 2;
	}

	return (*this);
}



trampoline_t &trampoline_t::append_relative_offset(DWORD offset) {

	DWORD jump = offset - ((DWORD)bytes + length) - 4;
	//PRINT("offset: %X, length: %ld, jump: 0x%X\n", offset, length, jump);
	(*this) << jump;

	return (*this);

}

trampoline_t &trampoline_t::append_CALL(DWORD funcaddr) {
	(*this) << (BYTE)0xE8;
	append_relative_offset(funcaddr);
	return (*this);

}

trampoline_t &trampoline_t::append_bytes(const BYTE* b, int size) {
	for (int i = 0; i < size; ++i) {
		(*this) << b[i];
	}
	return (*this);

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

void hook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
	h->patch.enable();
}

void unhook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
	h->patch.disable();
}


static void __stdcall call_pylpyr() {
	if (wc3mode_enabled())
		wc3_draw_pylpyrs();
}



static void __stdcall Present_hook() {

	init_custom_d3d(); // this doesn't do anything if it's already initialized
	do_wc3mode_stuff();

	if (wc3mode_enabled()) {
		if (mouse_pressed) {
			need_mouseup = 1;
			add_mouseup();
			need_mouseup = 0;
		}
	}

	register_luafunc_if_not_registered();

	static timer_interval_t fifty_ms(50);
	static timer_interval_t half_second(500);

	if (half_second.passed()) {
		update_hwevent_tick();

		if (credentials.valid && !credentials.logged_in) credentials.try_login();

		half_second.reset();
	}

	ctm_handle_delayed_posthook();
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

	if (capture.active) {
		capture.start();
		capture.active = 0;
	}
	else if (capture.need_to_stop) {
		capture.finish();
	}
	

}


static void __stdcall EndScene_hook() {

	draw_custom_d3d();

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
	enable_wc3mode(0);
}

static void __stdcall broadcast_CTM(float *coords, int action) {

	float x, y, z;

	x = coords[0];
	y = coords[1];
	z = coords[2];

	PRINT("broadcast_CTM: got CTM coords: (%f, %f, %f), action = %d\n", x, y, z, action);

	char sprintf_buf[128];

	sprintf_s(sprintf_buf, "%.1f %.1f %.1f", x, y, z);

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
	
	tr << PUSHAD;

	tr.append_CALL((DWORD)EndScene_hook);
	tr << POPAD;
		
	memcpy(p->original, (LPVOID)EndScene, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; 
	
	return &tr;

}


static const trampoline_t *prepare_ClosePetStables_patch(patch_t *p) {

	static trampoline_t tr;

	tr << PUSHAD; 

	tr.append_CALL((DWORD)ClosePetStables_hook);
	tr << POPAD;

	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; 
	
	return &tr;

}


static const trampoline_t *prepare_CTM_finished_patch(patch_t *p) {
	static trampoline_t tr;

	tr << PUSHAD; // PUSHAD

	tr.append_CALL((DWORD)CTM_finished_hookfunc);
	tr << POPAD; // POPAD

	tr.append_bytes(p->original, p->size);

// NOTE: COULD BE THIS INSTEAD OF (p->patch_addr + p->size): DWORD ret_addr = 0x7273EF;

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET

	return &tr;
}

static const trampoline_t *prepare_sendpacket_patch(patch_t *p) {

	static trampoline_t tr;
	
	PRINT("dump_packet offset: 0x%X, trampoline: 0x%X\n", dump_packet, tr.bytes);

	DWORD retaddr = 0x467773;
	DWORD some_crypt_func = 0x4665B0;

	tr << PUSHAD // PUSHAD
	   << (BYTE)0x52; // push EDX (the packet address)

	tr.append_CALL((DWORD)dump_packet);
	tr << POPAD; // POPAD

	tr.append_CALL(some_crypt_func); // from the original opcodes
	
	tr << (BYTE)0x68 << retaddr; // push RET addr
	
	tr << RET; // RET

	return &tr;

}

static const trampoline_t *prepare_recvpacket_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("dump_packet offset: 0x%X, trampoline: 0x%X\n", dump_packet, tr.bytes);

	DWORD retaddr = 0x467ECD;

	tr << PUSHAD; // PUSHAD

	tr << (BYTE)0x8B << (BYTE)0x46 << (BYTE)0x1C; // mov eax, dword ptr ds:[esi+1c] // decrypted packet address
	tr << (BYTE)0x50; // push eax

	tr.append_CALL((DWORD)dump_packet);
	tr << POPAD; // POPAD

	// original stuff
	tr << (BYTE)0x01 << (BYTE)0x5E << (BYTE)0x20;
	tr << (BYTE)0x8B << (BYTE)0x4D << (BYTE)0xF8;

	tr << (BYTE)0x68 << retaddr; // push RET addr

	tr << RET; // RET

	return &tr;

}



static const trampoline_t *prepare_Present_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing Present patch...\n");

	DWORD Present = get_Present();
	PRINT("Found Present at 0x%X\n", Present);
	p->patch_addr = Present;

	tr << PUSHAD; // PUSHAD

	tr.append_CALL((DWORD)Present_hook);
	tr << POPAD; // POPAD
	
	memcpy(p->original, (LPVOID)Present, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET


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
	tr << PUSHAD; 

	tr.append_CALL((DWORD)DrawIndexedPrimitive_hook);
	tr << POPAD; 

	memcpy(p->original, (LPVOID)DrawIndexedPrimitive, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET

	return &tr;

}

static int __stdcall mbuttondown_hook(DWORD wParam) {
	return 1;
}

// there's a mouse button (r/l) down handler at 8697E0

static const trampoline_t *prepare_mbuttondown_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttondown patch...\n");

	tr << PUSHAD; // PUSHAD
	tr << (BYTE)0x56; // push esi (contains WndProc/wParam)
	tr.append_CALL((DWORD)mbuttondown_hook);
	tr.append_hexstring("83F800"); // cmp eax, 0
	tr.append_hexstring("0F840D000000"); // jz branch 2
	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET; // ret
	
	// branch 2
	tr << POPAD; // POPAD
	tr << (BYTE)0x68 << (DWORD)0x86A80A;
	tr << RET; // ret

	return &tr;

}

static void __stdcall mbuttonup_hook(DWORD wParam) {
	
	
}

static const trampoline_t *prepare_mbuttonup_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttonup patch...\n");

	tr << PUSHAD; // PUSHAD
	tr << (BYTE)0x56; // push esi (contains WndProc/wParam)
	tr.append_CALL((DWORD)mbuttonup_hook);
	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET; 

	return &tr;
}

static const trampoline_t *prepare_pylpyr_patch(patch_t *p) {

 // THE SELECTION RING DRAWING HAPPENS AT 7E41FB!!
 // 4F6F90 could be of interest :>

 // A BREAKPOINT AT 0x4F6FAB is the unique code path for pylpyr (fires ONLY if an unit is selected)
 // 725980 is some member function probably to draw a pylpyr under an unit / npc :D


	static trampoline_t tr;

	PRINT("Preparing pylpyr patch...\n");

	tr << PUSHAD;
	tr.append_CALL((DWORD)call_pylpyr);

	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET;

	return &tr;

}

static void __stdcall mwheel_hook(DWORD wParam) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (zDelta < 0) customcamera.increment_s();
	if (zDelta > 0) customcamera.decrement_s();
}



static const trampoline_t *prepare_mwheel_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mwheel patch...\n");

	tr << PUSHAD;
	tr << (BYTE)0x8B << (BYTE)0x45 << (BYTE)0x10; // move wParam to EAX
	tr << (BYTE)0x50; // push eax
	tr.append_CALL((DWORD)mwheel_hook);

	tr << POPAD; // POPAD
	tr << (BYTE)0x68 << (DWORD)0x86A8D1; // skip right to return
	tr << RET;

	return &tr;

}

typedef struct {
	DWORD action;
	GUID_t *GUID;
	float *coords;
	DWORD s2;
} CTM_final_args_t;

void __stdcall CTM_main_hook(CTM_final_args_t *a) {
	PRINT("CTM: action %X, s1: %X, coords: %X, s2: %X\n", a->action, a->GUID, (DWORD)a->coords, a->s2);

	// segfault at 6DD06A with action 0xA

	if (a->action == 0x4) {
		float *c = a->coords;
		DoString("RunMacroText(\"/lole broadcast ctm %.3f %.3f %.3f\")", c[0], c[1], c[2]);
	}
	else if (a->action == 0xA) {
		DoString("RunMacroText(\"/lole broadcast attack 0x%16llX\")", *a->GUID);
	}
}

static const trampoline_t *prepare_CTM_main_patch(patch_t *p) {
	static trampoline_t tr;
	
	tr << PUSHAD;
	tr.append_hexstring("8D54242452"); // lea edx, [esp + 24]; push edx; because LOLZ
	tr.append_CALL((DWORD)CTM_main_hook);
	tr << POPAD;
//	tr.append_bytes(p->original, p->size);
//	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size);
	tr.append_hexstring("C21000"); // retn 10


	return &tr;
}



static const trampoline_t *prepare_AddInputEvent_patch(patch_t *p) {

	static trampoline_t tr;

	tr.append_hexstring("608D5C242453"); // pushad; lea ebx, [esp + 0x24]; push ebx
	tr.append_CALL((DWORD)AddInputEvent_hook);
	tr.append_hexstring("83F800"); // cmp eax, 0
	tr.append_hexstring("0F840F000000"); // jz branch 2

	tr << POPAD;
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size);
	tr << RET;

	// branch 2

	tr << POPAD;
	tr << RET; // straight ret if we want to filter this shit B)

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
	{ "CTM_finished", patch_t(CTM_finished_patchaddr, 10, prepare_CTM_finished_patch) },
	{ "ClosePetStables", patch_t(ClosePetStables_patchaddr, 5, prepare_ClosePetStables_patch) },
	{ "mbuttondown_handler", patch_t(mbuttondown_handler, 6, prepare_mbuttondown_patch) },
	{ "mbuttonup_handler", patch_t(mbuttonup_handler, 7, prepare_mbuttonup_patch) },
	{ "DrawIndexedPrimitive", patch_t(PATCHADDR_LATER, 5, prepare_DrawIndexedPrimitive_patch) },
	{ "pylpyr", patch_t(pylpyr_patchaddr, 9, prepare_pylpyr_patch) },
	{ "mwheel_handler", patch_t(mwheel_hookaddr, 6, prepare_mwheel_patch) },
	{ "CTM_main", patch_t(CTM_main_hookaddr, 6, prepare_CTM_main_patch)},
	{ "AddInputEvent", patch_t(AddInputEvent, 8, prepare_AddInputEvent_patch) },

};

hookable_t *find_hookable(const std::string &funcname) {
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

#define ADD_PATCH_SAFE(patchname) do { int r = get_patch_from_hookable(patchname, &p); assert(r); PIPEDATA.add_patch(p); } while(0)

	ADD_PATCH_SAFE("EndScene");
	ADD_PATCH_SAFE("Present");
	ADD_PATCH_SAFE("CTM_finished");
	ADD_PATCH_SAFE("ClosePetStables");
	ADD_PATCH_SAFE("pylpyr");
	ADD_PATCH_SAFE("CTM_main");
	ADD_PATCH_SAFE("AddInputEvent");

	
	// don't add DrawIndexedPrimitive to this list, it will be patched manually later by a /lole subcommand
	//ADD_PATCH_SAFE("DrawIndexedPrimitive");

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

