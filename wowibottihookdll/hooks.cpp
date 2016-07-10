#include "hooks.h"
#include "addrs.h"
#include "defs.h"
#include "opcodes.h"
#include "ctm.h"
#include "creds.h"

static HRESULT(*EndScene)(void);
pipe_data PIPEDATA;

const UINT32 PIPE_PROTOCOL_MAGIC = 0xAB30DD13;

static void update_hwevent_tick() {
	typedef int tick_count_t(void);
	int ticks = ((tick_count_t*)GetOSTickCount)();

	*(int*)(TicksSinceLastHWEvent) = ticks;
	// this should make us immune to AFK ^^
}

static void attempt_login() {
	//printf("attempting login with account %s\n", credentials.account.c_str());
	DoString(credentials.login_script.c_str());
}

static void update_debug_positions() {
	ObjectManager OM;
	auto p = OM.get_local_object();
	vec3 ppos = p.get_pos();
	char buf[128];

	sprintf(buf, "(%.1f, %.1f, %.1f)", ppos.x, ppos.y, ppos.z);
	DoString("SetCVar(\"movieSubtitle\", \"%s\", \"player_pos\")", buf);

	if (get_target_GUID() != 0) {
		auto t = OM.get_object_by_GUID(get_target_GUID());
		vec3 tpos = t.get_pos();
		sprintf(buf, "(%.1f, %.1f, %.1f)", tpos.x, tpos.y, tpos.z);

		DoString("SetCVar(\"movieSubtitle\", \"%s\", \"target_pos\")", buf);
	}
	else {
		DoString("SetCVar(\"movieSubtitle\", \"-\", \"target_pos\")");
	}
}


static void __stdcall EndScene_hook() {

	static int every_third_frame = 0;
	static int every_thirty_frames = 0;

	if (every_third_frame == 0) {
		refollow_if_needed();
		ctm_act();
	}

	if (every_thirty_frames == 0) {
		update_debug_positions();
		update_hwevent_tick();
		attempt_login();
	}

	every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;
	every_thirty_frames = every_thirty_frames > 29 ? 0 : every_thirty_frames + 1;

}



static void __stdcall broadcast_CTM(float *coords, int action) {

	float x, y, z;

	x = coords[0];
	y = coords[1];
	z = coords[2];

	printf("broadcast_CTM: got CTM coords: (%f, %f %f), action = %d\n", x, y, z, action);

	char sprintf_buf[128];

	sprintf(sprintf_buf, "%.1f,%.1f,%.1f", x, y, z);

	// the CTM mode is determined in the LUA logic, in the subcommand handler.

	DoString("RunMacroText(\"/lole ctm %s\")", sprintf_buf);
}


static void __stdcall DelIgnore_hub(const char* arg_) {
	// the opcodes sent by the addon all begin with the sequence "LOP_"
	static const std::string opcode_prefix("LOP_");

	const std::string arg(arg_);

	if (strncmp(arg.c_str(), opcode_prefix.c_str(), opcode_prefix.length()) != 0) {
		printf("DelIgnore_hub: invalid opcode \"%s\": DelIgnore_hub opcodes must begin with the sequence \"%s\"\n", arg_, opcode_prefix.c_str());
		return;
	}

	if (arg.length() < 6) {
		printf("DelIgnore_hub: invalid opcode \"%s\"\n", arg.c_str());
		return;
	}

	std::string opstr = arg.substr(4, 2);
	char *endptr;
	unsigned long op = strtoul(opstr.c_str(), &endptr, 16);

	if (op & 0x80) {
		int debug_op = op & 0x7F;
		opcode_debug_call(debug_op, arg);
		printf("DelIgnore_hub: got DEBUG opcode %lu -> %s\n", op, opcode_get_funcname(debug_op).c_str());
		return;
	}

	printf("DelIgnore_hub: got opcode %lu -> %s\n", op, opcode_get_funcname(op).c_str());

	int num_args = opcode_get_num_args(op);

	if (num_args > 0) {
		// then we expect to find a ':' and arguments separated by ',':s
		size_t pos;
		if ((pos = arg.find(':', 5)) == std::string::npos) {
			printf("DelIgnore_hub: error: opcode %lu (%s) expects %d arguments, none found! (did you forget ':'?)\n",
				op, opcode_get_funcname(op).c_str(), num_args);
			return;
		}
		std::string args = arg.substr(pos + 1);
		printf("DelIgnore_hub: calling func %s with args \"%s\"\n", opcode_get_funcname(op).c_str(), args.c_str());

		opcode_call(op, args); // call the func with args
	}
	else {
		opcode_call(op, "");
	}

}

static void __stdcall CTM_finished_hookfunc() {
	ctm_commit();
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
	0xE8, 0xBB, 0xFF, 0xFF, 0xFF // CALL 004FAC60. this wouldn't actually even need a trampoline lol
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

static int prepare_LUA_prot_patch(LPVOID hook_func_addr, hookable &h) {
	return 1;
}

static int prepare_EndScene_patch(LPVOID hook_func_addr, hookable &h) {

	// this actually seems to work :)

	printf("Preparing EndScene patch...\n");

	unsigned char *wow_static_DX9 = *(unsigned char**)0xD2A15C;
	unsigned char *tmp1 = *(unsigned char**)(wow_static_DX9 + 0x3864);
	unsigned char *tmp2 = *(unsigned char**)(tmp1);

	EndScene = (HRESULT(*)(void))*(unsigned char**)(tmp2 + 0xA8);
	printf("Found EndScene at %p\n(Details:\n[0xD2A15C] = %p\n[[0xD2A15C] + 0x3864] = %p\n[[[0xD2A15C] + 0x3864]] = %p)\n\n", EndScene, wow_static_DX9, tmp1, tmp2);
	h.address = EndScene;

	static BYTE EndScene_trampoline[] = {
		// original EndScene opcodes follow
		0x6A, 0x20, // push 20
		0xB8, 0xD8, 0xB9, 0x08, 0x6C, // MOV EAX, 6C08B9D8 after this should be mu genitals :D
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

	printf("OK. EndScene trampoline: %p, hookfunc addr: %p\n", &EndScene_trampoline, hook_func_addr);

	return 1;

}



static int prepare_DelIgnore_patch(LPVOID hook_func_addr, hookable &h) {

	// DelIgnore == 5BA4B0, 
	// but the string appears in EAX after a call to 72DFF0 (called at 5BA4BC)
	// so hook func at 7BA4C1!

	printf("Preparing DelIgnore patch...\n");

	uint PATCH_ADDR = 0x7BA4C1;

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

	printf("OK.\nDelIgnore trampoline: %p, hookfunc addr: %p\n", &DelIgnore_trampoline, hook_func_addr);


	return 1;
}

static int prepare_ClosePetStables_patch(LPVOID hook_func_addr, hookable &h) {

	printf("Preparing ClosePetStables patch...\n");

	// ClosePetStables = 0x4FACA0

	static BYTE ClosePetStables_trampoline[] = {
		// original opcodes from ClosePetStables
		0xE8, 0x00, 0x00, 0x00, 0x00, // CALL 004FAC60. need to insert address relative to this trampoline 

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (ClosePetStables + 5) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		0xC3 //ret
	};

	DWORD orig_CALL_target = 0x4FAC60;
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

	printf("OK.\nClosePetStables trampoline: %p, hook_func_addr: %p\n", &ClosePetStables_trampoline, hook_func_addr);

	return 1;

}


static int prepare_CTM_aux_patch(LPVOID hook_func_addr, hookable &h) {
	printf("Preparing CTM_aux patch...\n");

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

	printf("OK.\nCTM_aux trampoline: %p, hook_func_addr: %p\n", &CTM_aux_trampoline, hook_func_addr);

	return 1;
}

static int __stdcall get_CTM_retaddr(int action) {
	static const uint ret_early = 0, ret_late = 1;
	
	printf("get_CTM_retaddr: action = %X\n", action);

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
	printf("Preparing CTM_main patch...\n");

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

	printf("OK.\nCTM_main trampoline: %p, hook_func_addr: %p\n", &CTM_main_trampoline, hook_func_addr);

	return 1;
}

static int prepare_CTM_finished_patch(LPVOID hook_func_addr, hookable &h) {

	printf("Preparing CTM_finished patch...\n");

	static BYTE CTM_finished_trampoline[] = {
		// original opcodes from ClosePetStables
		0xC7, 0x05, 0xBC, 0x89, 0xD6, 0x00, 0x0D, 0x00, 0x00, 0x00, // MOV [to address] 0xD689BC, 0x0D 

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		0xC3 //ret
	};

	DWORD tr_offset = (DWORD)CTM_finished_trampoline - CTM_update_hookaddr - 5;
	memcpy(CTM_finished_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = 0x612A7B;
	memcpy(CTM_finished_trampoline + 11, &ret_addr, sizeof(ret_addr));


	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)CTM_finished_trampoline - 21;
	memcpy(CTM_finished_trampoline + 17, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)CTM_finished_trampoline, sizeof(CTM_finished_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("OK.\nCTM_finished trampoline: %p, hook_func_addr: %p\n", &CTM_finished_trampoline, hook_func_addr);

	return 1;

}

static hookable hookable_functions[] = {
	{ "LUA_prot", (LPVOID)LUA_prot, LUA_prot_original, LUA_prot_patch, sizeof(LUA_prot_original), prepare_LUA_prot_patch},
	{ "EndScene", 0x0, EndScene_original, EndScene_patch, sizeof(EndScene_original), prepare_EndScene_patch},
	{ "DelIgnore", (LPVOID)DelIgnore_hookaddr, DelIgnore_original, DelIgnore_patch, sizeof(DelIgnore_original), prepare_DelIgnore_patch },
	{ "ClosePetStables", (LPVOID)ClosePetStables, ClosePetStables_original, ClosePetStables_patch, sizeof(ClosePetStables_original), prepare_ClosePetStables_patch },
	{ "CTM_aux", (LPVOID)CTM_aux, CTM_aux_original, CTM_aux_patch, sizeof(CTM_aux_original), prepare_CTM_aux_patch},
	{ "CTM_main", (LPVOID)CTM_main, CTM_main_original, CTM_main_patch, sizeof(CTM_main_original), prepare_CTM_main_patch},
	{ "CTM_update", (LPVOID)CTM_update_hookaddr, CTM_finished_original, CTM_finished_patch, sizeof(CTM_finished_original), prepare_CTM_finished_patch}
};

static int prepare_patch(const std::string &funcname, LPVOID hook_func_addr) {
	for (auto &h : hookable_functions) {
		if (funcname == h.funcname) {
			h.prepare_patch(hook_func_addr, h);
			printf("prepare_patch successful for function %s!\n", funcname.c_str());
			return 1;
		}
	}

	return 0;
}

int prepare_patches_and_pipe_data() {

	prepare_patch("LUA_prot", 0x0);
	prepare_patch("DelIgnore", DelIgnore_hub);
	prepare_patch("CTM_main", broadcast_CTM);
	prepare_patch("CTM_update", CTM_finished_hookfunc);
	prepare_patch("EndScene", EndScene_hook);

	PIPEDATA.add_patch(patch_serialized(LUA_prot, sizeof(LUA_prot_patch), LUA_prot_original, LUA_prot_patch));
	PIPEDATA.add_patch(patch_serialized(DelIgnore_hookaddr, sizeof(DelIgnore_patch), DelIgnore_original, DelIgnore_patch));
	PIPEDATA.add_patch(patch_serialized(CTM_main, sizeof(CTM_main_patch), CTM_main_original, CTM_main_patch));
	PIPEDATA.add_patch(patch_serialized(CTM_update, sizeof(CTM_aux_patch), CTM_aux_original, CTM_aux_patch));
	PIPEDATA.add_patch(patch_serialized((UINT32)EndScene, sizeof(EndScene_patch), EndScene_original, EndScene_patch));

	return 1;
}

static int install_hook(const std::string &funcname, LPVOID hook_func_addr) {

	for (auto &h : hookable_functions) {
		if (funcname == h.funcname) {
			h.prepare_patch(hook_func_addr, h);
			SIZE_T bytes;
			printf("Patching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.patch, h.patch_size, &bytes);
			printf("OK!\n");
			return bytes;
		}
	}

	printf("install_hook: error: \"%s\": no such function!\n", funcname.c_str());

	return 0;
}

static int uninstall_hook(const std::string &funcname) {
	//for (int i = 0; i < sizeof(hookable_functions) / sizeof(hookable_functions[0]); ++i) {
	for (auto &h : hookable_functions) {
		//	const hookable &h = hookable_functions[i];
		if (funcname == h.funcname) {
			SIZE_T bytes;
			printf("Unpatching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.original_opcodes, h.patch_size, &bytes);
			printf("OK!\n");
			return bytes;
		}
	}

	printf("uninstall_hook: error: \"%s\": no such function!\n", funcname.c_str());

	return 0;
}

int hook_all() {

	install_hook("DelIgnore", DelIgnore_hub);
	install_hook("CTM_main", broadcast_CTM);
	install_hook("CTM_update", CTM_finished_hookfunc);
	install_hook("EndScene", EndScene_hook);

	return 1;
}

int unhook_all() {

	uninstall_hook("EndScene");
	uninstall_hook("DelIgnore");
	uninstall_hook("ClosePetStables");
	uninstall_hook("CTM_main");
	uninstall_hook("CTM_update");

	return 1;
}

int hook_EndScene() {
	install_hook("EndScene", EndScene_hook);

	return 1;
}

int patch_DelIgnore() {
	install_hook("DelIgnore", DelIgnore_hub);

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

