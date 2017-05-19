#include <queue>
#include "hooks.h"
#include "addrs.h"
#include "defs.h"
#include "opcodes.h"
#include "ctm.h"
#include "creds.h"
#include "timer.h"
#include "lua.h"

static HRESULT(*EndScene)(void);
pipe_data PIPEDATA;

const UINT32 PIPE_PROTOCOL_MAGIC = 0xAB30DD13;

template <typename T> void operator+= (patchbuffer_t &p, const T& arg) {
	int size = sizeof(arg);
	BYTE buf[16];
	memcpy(buf, &arg, size);

	//for (int i = 0; i < 24; ++i) {
	//	PRINT("%02X ", p.bytes[i]);
	//}
	//PRINT("\n(before)\n");

	for (int i = 0; i < size; ++i) {
		p.bytes[p.length] = buf[i];
		++p.length;
	}
	
	//for (int i = 0; i < 24; ++i) {
	//	PRINT("%02X ", p.bytes[i]);
	//}
	//PRINT("\n(after)\n\n");

}

void patchbuffer_t::add_relative_offset(DWORD offset) {
	
	DWORD jump = offset - ((DWORD)bytes + length) - 4;
	PRINT("offset: %X, length: %ld, jump: 0x%X\n", offset, length, jump);
	(*this) += jump;

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

static void update_debug_positions() {

	ObjectManager OM;

	WowObject p;
	if (!OM.get_local_object(&p)) { 
		PRINT("local GUID = %llX, but get_local_object returned an invalid object!\n", OM.get_local_GUID());
		return; 
	}

	vec3 ppos = p.get_pos();
	char buf[128];

	sprintf_s(buf, "(%.1f, %.1f, %.1f)", ppos.x, ppos.y, ppos.z);

	DoString("SetCVar(\"movieSubtitle\", \"%s\", \"player_pos\")", buf);

	if (get_target_GUID() != (GUID_t)0) {
		WowObject t;
		if (!OM.get_object_by_GUID(get_target_GUID(), &t)) return;
		vec3 tpos = t.get_pos();
		sprintf_s(buf, "(%.1f, %.1f, %.1f)", tpos.x, tpos.y, tpos.z);

		DoString("SetCVar(\"movieSubtitle\", \"%s\", \"target_pos\")", buf);
	}
	else {
		DoString("SetCVar(\"movieSubtitle\", \"-\", \"target_pos\")");
	}
}


static void __stdcall EndScene_hook() {
	register_luafunc_if_not_registered();

	static timer_interval_t fifty_ms(50);
	static timer_interval_t half_second(500);
	
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


}

static void __stdcall dump_packet(BYTE *packet) {

	enum {
		CMSG_CAST_SPELL = 0x12E,
		CMSG_SET_SELECTION = 0x13D,
	};

	DWORD length = (DWORD)(packet[0]) << 8;
	length += packet[1] + 2; // +2 for the length bytes themselves (not included apparently)

	DWORD opcode;
	memcpy(&opcode, &packet[2], 4);
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
		//for (int i = 6; i < length; ++i) {
		//	PRINT("%02X ", packet[i]);
		//}
		//PRINT("\n\n");
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

static void __stdcall SpellErrMsg_hook(int msg) {
	typedef int tick_count_t(void);
	//int ticks = ((tick_count_t*)GetOSTickCount)();

	previous_cast_msg.msg = msg;
	//previous_cast_msg.timestamp = ticks;
}

struct hookable {
	std::string funcname;
	LPVOID address;
	const BYTE *original_opcodes;
	BYTE *patch;
	size_t patch_size;
	int(*prepare_patch)(LPVOID, hookable&);
};

static const BYTE LUA_prot_original[] = {
	// we're just making an early exit, so nevermind opcode boundaries
	0x55,
	0x8B, 0xEC,
	0x83, 0x3D, 0x40 // this one is cut in the middle
};

static BYTE LUA_prot_patch[] = {
	0xB8, 0x01, 0x00, 0x00, 0x00, // MOV EAX, 1
	0xC3						  // RET
};


static const BYTE EndScene_original[] = {
	0x6A, 0x20, // push 20
	0xB8, 0xD8, 0xB9, 0x08, 0x6C // MOV EAX, 6C08B9D8 after this should be mu genitals :D
};

static BYTE EndScene_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	0x90, 0x90
};

static const BYTE DelIgnore_original[] = {
	0x6A, 0x01, // push 1
	0x6A, 0x01, // push 1
	0x8D, 0x4D, 0xFC, // LEA ECX, [LOCAL.1]
};

static BYTE DelIgnore_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	0x90, 0x90
};

static const BYTE ClosePetStables_original[] = {
	0xE8, 0x3B, 0x1E, 0xF3, 0xFF // CALL 004FAC60. this wouldn't actually even need a trampoline lol
};

static BYTE ClosePetStables_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00
};

static const BYTE CTM_main_original[] = {
	0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18
};

static BYTE CTM_main_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90
};

static const BYTE CTM_aux_original[] = {
	0x55, 0x8B, 0xEC, 0x8B, 0x41, 0x38
};

// this is a patch for func 0x7B8940
static BYTE CTM_aux_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90
};
// NOP for padding is a lot better, since the disassembler gets fucked up with 0x00s

static const BYTE CTM_finished_original[] = {
	0xC7, 0x05, 0xBC, 0x89, 0xD6, 0x00, 0x0D, 0x00, 0x00, 0x00
};

static BYTE CTM_finished_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90
};

static const BYTE spell_errmsg_original[] = {
	0x0F, 0xB6, 0xC0, 0x3D, 0xA8, 0x00, 0x00, 0x00
};

static BYTE spell_errmsg_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90
};

static const BYTE sendpacket_original[] = {
	0xE8, 0x3D, 0xEE, 0xFF, 0xFF
};

static BYTE sendpacket_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00
};

static int prepare_LUA_prot_patch(LPVOID hook_func_addr, hookable &h) {
	return 1;
}

static int prepare_EndScene_patch(LPVOID hook_func_addr, hookable &h) {

	// this actually seems to work :)
#define STATIC_243_DIRECT3DDEVICE 0xD2A15C
#define STATIC_243_D3DDEVICE_OFFSET 0x3864

	#define STATIC_335_DIRECT3DDEVICE 0xC5DF88 
	#define STATIC_335_D3DDEVICE_OFFSET 0x397C

	PRINT("Preparing EndScene patch...\n");

	unsigned char *wow_static_DX9 = *(unsigned char**)STATIC_335_DIRECT3DDEVICE;
	unsigned char *tmp1 = *(unsigned char**)(wow_static_DX9 + STATIC_335_D3DDEVICE_OFFSET);
	unsigned char *tmp2 = *(unsigned char**)(tmp1);

	EndScene = (HRESULT(*)(void))*(unsigned char**)(tmp2 + 0xA8);
	PRINT("Found EndScene at 0x%X\n(Details:\nwow_static_DX9 = 0x%X, tmp1 = 0x%X, tmp2 = 0x%X\n", EndScene, wow_static_DX9, tmp1, tmp2);
	h.address = EndScene;

	static BYTE EndScene_trampoline[] = {
		// original EndScene opcodes follow
		0x6A, 0x20, // push 20
		0xB8, 0x47, 0x7F, 0xD8, 0x6F,
		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (endscene + 7) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call hook_func
		0x61, // popad
		0xC3 //ret
	};

	DWORD tr_offset = ((DWORD)EndScene_trampoline - (DWORD)EndScene - 5);
	memcpy(EndScene_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)EndScene + 7;
	memcpy(EndScene_trampoline + 8, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)EndScene_trampoline - 18;
	memcpy(EndScene_trampoline + 14, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)EndScene_trampoline, sizeof(EndScene_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK. EndScene trampoline: %p, hookfunc addr: %p\n", &EndScene_trampoline, hook_func_addr);

	return 1;

}



static int prepare_DelIgnore_patch(LPVOID hook_func_addr, hookable &h) {

	// DelIgnore == 5BA4B0, 
	// but the string appears in EAX after a call to 72DFF0 (called at 5BA4BC)
	// so hook func at 7BA4C1!

	PRINT("Preparing DelIgnore patch...\n");

	//uint PATCH_ADDR = 0x7BA4C1;

	static BYTE DelIgnore_trampoline[] = {
		// from the original DelIgnore func
		0x6A, 0x01, // push 1
		0x6A, 0x01, // push 1
		0x8D, 0x4D, 0xFC, // LEA ECX, [LOCAL.1]

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (5BA4C1 + 7 = 5BA4C8)
		0x60, // pushad
		0x50, // push EAX, we know that eax contains string address at this point
		0xE8, 0x00, 0x00, 0x00, 0x00, // hookfunc addr
		0x61, // popad
		0xC3 //ret
	};


	DWORD tr_offset = ((DWORD)DelIgnore_trampoline - (DWORD)DelIgnore_hookaddr - 5);
	memcpy(DelIgnore_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)DelIgnore_hookaddr + 0x2B; // jump straight to the end, so we skip the "Player not found."
	memcpy(DelIgnore_trampoline + 8, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)DelIgnore_trampoline - 19;
	memcpy(DelIgnore_trampoline + 15, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)DelIgnore_trampoline, sizeof(DelIgnore_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nDelIgnore trampoline: %p, hookfunc addr: %p\n", &DelIgnore_trampoline, hook_func_addr);


	return 1;
}

static int prepare_ClosePetStables_patch(LPVOID hook_func_addr, hookable &h) {

	PRINT("Preparing ClosePetStables patch...\n");

	// ClosePetStables = 0x4FACA0

	static BYTE ClosePetStables_trampoline[] = {
		// original opcodes from ClosePetStables
		0xE8, 0x00, 0x00, 0x00, 0x00, // CALL 004D3790. need to insert address relative to this trampoline 

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (ClosePetStables + 5) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		0xC3 //ret
	};

	DWORD orig_CALL_target = 0x4D3790;
	DWORD CALL_target_offset = ((DWORD)orig_CALL_target - ((DWORD)ClosePetStables_trampoline + 5));
	memcpy(ClosePetStables_trampoline + 1, &CALL_target_offset, sizeof(CALL_target_offset));

	DWORD tr_offset = ((DWORD)ClosePetStables_trampoline - (DWORD)ClosePetStables - 5);
	memcpy(ClosePetStables_patch + 1, &tr_offset, sizeof(tr_offset));


	DWORD ret_addr = (DWORD)ClosePetStables + 5;
	memcpy(ClosePetStables_trampoline + 6, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)ClosePetStables_trampoline - 16;
	memcpy(ClosePetStables_trampoline + 12, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)ClosePetStables_trampoline, sizeof(ClosePetStables_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nClosePetStables trampoline: %p, hook_func_addr: %p\n", &ClosePetStables_trampoline, hook_func_addr);

	return 1;

}


static int prepare_CTM_aux_patch(LPVOID hook_func_addr, hookable &h) {
	PRINT("Preparing CTM_aux patch...\n");

	static BYTE CTM_aux_trampoline[] = {
		// original opcodes from 0x7B8940 "CTM_aux"
		0x55, // PUSH EBP
		0x8B, 0xEC, // MOV EBP, ESP
		0x8B, 0x41, 0x38, // MOV EAX, DWORD PTR DS:[ECX+38]

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (7B8940 + 6 = 7B8946)
		0x60,					// pushad
		0x8B, 0x4D, 0x0C,		// MOV ECX, DWORD PTR SS:[ARG.2]
		0x51,					// PUSH ECX, this is our argument (pointer to CTM coords on stack =))
		0xE8, 0x00, 0x00, 0x00, 0x00, // call CTM_broadcast function :D loloz
		0x61, // popad
		0xC3 //ret
	};
	
	DWORD tr_offset = ((DWORD)CTM_aux_trampoline - (DWORD)CTM_aux - 5);
	memcpy(CTM_aux_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)CTM_aux + 6;
	memcpy(CTM_aux_trampoline + 7, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)CTM_aux_trampoline - 21; // first byte is at offset 19
	memcpy(CTM_aux_trampoline + 17, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)CTM_aux_trampoline, sizeof(CTM_aux_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nCTM_aux trampoline: %p, hook_func_addr: %p\n", &CTM_aux_trampoline, hook_func_addr);

	return 1;
}

static int __stdcall get_CTM_retaddr(int action) {
	static const uint ret_early = 0, ret_late = 1;
	
	PRINT("get_CTM_retaddr: action = %X\n", action);

	switch (action) {
	case CTM_MOVE: 
		return ret_late;
		break;

	default:
		return ret_early;
		break;

	}

}

static int prepare_CTM_main_patch(LPVOID hook_func_addr, hookable &h) {
	PRINT("Preparing CTM_main patch...\n");

	static BYTE CTM_main_trampoline[] = {
		// original opcodes from 0x612A90 "CTM_main"

		0x55, // PUSH EBP
		0x8B, 0xEC, // MOV EBP, ESP
		0x83, 0xEC, 0x18, // SUB ESP, 18

		0x68, 0x00, 0x00, 0x00, 0x00, // push early return address (performs the 612A90 function normally)
		0x60,						// pushad

		0x8B, 0x75, 0x8, // MOV ESI, DWORD PTR SS:[ARG.2] CTM_"push" or "action"
		0x56,			// PUSH ESI

		0xE8, 0x00, 0x00, 0x00, 0x00, // call to get_CTM_retaddr. eax now has 1 or 0, depending on the env
		0x85, 0xC0,	// test EAX EAX
		0x74, 0x0F, // jz, hopefully to the second popad part
		0x8B, 0x75, 0x8, // MOV ESI, DWORD PTR SS:[ARG.2] CTM_"push" or "action"
		0x56,
		0x8B, 0x75, 0x10, // MOV ESI, DWORD PTR SS:[ARG.3] (contains the 3 floatz ^^)
		0x56,			  // PUSH ESI (arg to CTM_broadcast)
		0xE8, 0x00, 0x00, 0x00, 0x00, // call CTM_broadcast function :D loloz
		0x61, // popad
		0xC3, // ret
		
		// branch #2

		0x61, // popad
		0x83, 0xC4, 0x04, // add esp, 4, to "pop" without screwing up reg values
		0x68, 0x00, 0x00, 0x00, 0x00, // push return late late
		0xC3 //ret
	};

	static const uint retaddr_early = CTM_main + 0x6, retaddr_late = CTM_main + 0x12E;

	DWORD tr_offset = ((DWORD)CTM_main_trampoline - (DWORD)CTM_main - 5);
	memcpy(CTM_main_patch + 1, &tr_offset, sizeof(tr_offset));

	memcpy(CTM_main_trampoline + 7, &retaddr_late, sizeof(retaddr_late));

	DWORD get_CTM_retaddr_offset = (DWORD)get_CTM_retaddr - (DWORD)CTM_main_trampoline - 21;
	memcpy(CTM_main_trampoline + 17, &get_CTM_retaddr_offset, sizeof(get_CTM_retaddr_offset));

	memcpy(CTM_main_trampoline + 45, &retaddr_early, sizeof(retaddr_early));

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)CTM_main_trampoline - 38;
	memcpy(CTM_main_trampoline + 34, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)CTM_main_trampoline, sizeof(CTM_main_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nCTM_main trampoline: %p, hook_func_addr: %p\n", &CTM_main_trampoline, hook_func_addr);

	return 1;
}

static int prepare_CTM_finished_patch(LPVOID hook_func_addr, hookable &h) {

	PRINT("Preparing CTM_finished patch...\n");

	static BYTE CTM_finished_trampoline[] = {
		// original opcodes from ClosePetStables
		0xC7, 0x05, 0xF4, 0x11, 0xCA, 0x00, 0x0D, 0x00, 0x00, 0x00, // MOV [to address] 0xD689BC, 0x0D 

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		0xC3 //ret
	};

	DWORD tr_offset = (DWORD)CTM_finished_trampoline - CTM_update_hookaddr - 5;
	memcpy(CTM_finished_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = 0x7273EF;
	memcpy(CTM_finished_trampoline + 11, &ret_addr, sizeof(ret_addr));

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)CTM_finished_trampoline - 21;
	memcpy(CTM_finished_trampoline + 17, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)CTM_finished_trampoline, sizeof(CTM_finished_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nCTM_finished trampoline: %p, hook_func_addr: %p\n", &CTM_finished_trampoline, hook_func_addr);

	return 1;

}

static int prepare_spell_errmsg_patch(LPVOID hook_func_addr, hookable &h) {
	static BYTE spell_errmsg_trampoline[] = {
		0x0F, 0xB6, 0xC0, // MOVZX, EAX, AL
		0x3D, 0xA8, 0x00, 0x00, 0x00, // CMP EAX, 0A8
		0x68, 0x00, 0x00, 0x00 ,0x00, // push return address
		0x60, // pushad
		0x50, // push eax (contains err msg)
		0xE8, 0x00, 0x00, 0x00, 0x00, // call hookfunc
		0x61, // popad
		0xC3 // ret
	};

	DWORD tr_offset = (DWORD)spell_errmsg_trampoline - SpellErrMsg - 5;
	memcpy(spell_errmsg_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = 0x6F0B78;
	memcpy(spell_errmsg_trampoline + 9, &ret_addr, sizeof(ret_addr));

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)spell_errmsg_trampoline - 20;
	memcpy(spell_errmsg_trampoline + 16, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)spell_errmsg_trampoline, sizeof(spell_errmsg_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	PRINT("OK.\nCTM_finished trampoline: %p, hook_func_addr: %p\n", &spell_errmsg_trampoline, hook_func_addr);

	return 1;
}

static int prepare_sendpacket_patch(LPVOID hook_func_addr, hookable &h) {
	
	static BYTE sendpacket_trampoline[] = {
		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (0x467773) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		
		0xE8, 0x00, 0x00, 0x00, 0x00, // CALL 0x4665B0. need to insert address relative to this trampoline 
		0xC3 //ret
	};

	static patchbuffer_t tr;
	
	PRINT("dump_packet offset: 0x%X, trampoline: 0x%X\n", dump_packet, tr.bytes);

	DWORD retaddr = 0x467773;
	DWORD encrypt = 0x4665B0;

	tr += (BYTE)0x60; // PUSHAD

	tr += (BYTE)0x52; // push EDX (the packet address)

	tr += (BYTE)0xE8; // call dump_packet
	tr.add_relative_offset((DWORD)dump_packet);

	tr += (BYTE)0x61; // POPAD

	tr += (BYTE)0xE8; // CALL encrypt
	tr.add_relative_offset(encrypt);
	
	tr += (BYTE)0x68; // push RET addr
	tr += retaddr;
	tr += (BYTE)0xC3;

	DWORD trampoline_relative = (DWORD)tr.bytes - Packet_hookaddr - 5;
	memcpy(sendpacket_patch+1, &trampoline_relative, 4);

	return 1;

}

static hookable hookable_functions[] = {
	{ "LUA_prot", (LPVOID)LUA_prot, LUA_prot_original, LUA_prot_patch, sizeof(LUA_prot_original), prepare_LUA_prot_patch },
	{ "EndScene", 0x0, EndScene_original, EndScene_patch, sizeof(EndScene_original), prepare_EndScene_patch },
	{ "DelIgnore", (LPVOID)DelIgnore_hookaddr, DelIgnore_original, DelIgnore_patch, sizeof(DelIgnore_original), prepare_DelIgnore_patch },
	{ "ClosePetStables", (LPVOID)ClosePetStables, ClosePetStables_original, ClosePetStables_patch, sizeof(ClosePetStables_original), prepare_ClosePetStables_patch },
	{ "CTM_aux", (LPVOID)CTM_aux, CTM_aux_original, CTM_aux_patch, sizeof(CTM_aux_original), prepare_CTM_aux_patch },
	{ "CTM_main", (LPVOID)CTM_main, CTM_main_original, CTM_main_patch, sizeof(CTM_main_original), prepare_CTM_main_patch },
	{ "CTM_update", (LPVOID)CTM_update_hookaddr, CTM_finished_original, CTM_finished_patch, sizeof(CTM_finished_original), prepare_CTM_finished_patch },
	{ "SpellErrMsg", (LPVOID)SpellErrMsg, spell_errmsg_original, spell_errmsg_patch, sizeof(spell_errmsg_original), prepare_spell_errmsg_patch },
	{ "packet_encrypt", (LPVOID)Packet_hookaddr, sendpacket_original, sendpacket_patch, sizeof(sendpacket_original), prepare_sendpacket_patch },
};

static hookable *find_hookable(const std::string &funcname) {
	for (auto &h : hookable_functions) {
		if (h.funcname == funcname) {
			return &h;
		}
	}
	return NULL;
}

static patch_serialized get_patch_from_hookable(const hookable *hook) {
	return patch_serialized((DWORD)hook->address, hook->patch_size, hook->original_opcodes, hook->patch);
}

static int prepare_patch(const std::string &funcname, LPVOID hook_func_addr) {
	for (auto &h : hookable_functions) {
		if (funcname == h.funcname) {
			h.prepare_patch(hook_func_addr, h);
			PRINT("prepare_patch successful for function %s!\n", funcname.c_str());
			return 1;
		}
	}

	return 0;
}

int prepare_patches_and_pipe_data() {

	//prepare_patch("LUA_prot", 0x0);
//	prepare_patch("CTM_main", broadcast_CTM);
	prepare_patch("CTM_update", CTM_finished_hookfunc);
	prepare_patch("EndScene", EndScene_hook);
//	prepare_patch("SpellErrMsg", SpellErrMsg_hook);
	prepare_patch("ClosePetStables", ClosePetStables_hook);
	prepare_patch("packet_encrypt", dump_packet);

//	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("LUA_prot")));
	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("EndScene")));
//	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("CTM_main")));
	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("CTM_update")));
//	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("SpellErrMsg")));
	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("ClosePetStables")));
	PIPEDATA.add_patch(get_patch_from_hookable(find_hookable("packet_encrypt")));

	return 1;
}

static int install_hook(const std::string &funcname, LPVOID hook_func_addr) {

	for (auto &h : hookable_functions) {
		if (funcname == h.funcname) {
			h.prepare_patch(hook_func_addr, h);
			SIZE_T bytes;
			PRINT("Patching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.patch, h.patch_size, &bytes);
			PRINT("OK!\n");
			return bytes;
		}
	}

	PRINT("install_hook: error: \"%s\": no such function!\n", funcname.c_str());

	return 0;
}

static int uninstall_hook(const std::string &funcname) {
	//for (int i = 0; i < sizeof(hookable_functions) / sizeof(hookable_functions[0]); ++i) {
	for (auto &h : hookable_functions) {
		//	const hookable &h = hookable_functions[i];
		if (funcname == h.funcname) {
			SIZE_T bytes;
			PRINT("Unpatching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.original_opcodes, h.patch_size, &bytes);
			PRINT("OK!\n");
			return bytes;
		}
	}

	PRINT("uninstall_hook: error: \"%s\": no such function!\n", funcname.c_str());

	return 0;
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

