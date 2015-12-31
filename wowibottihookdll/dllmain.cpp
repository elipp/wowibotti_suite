// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <D3D9.h>
#include <string>
#include <cstdio>

#include "addrs.h"
#include "defs.h"
#include "ObjectManager.h"
#include "hooks.h"

#include "window.h"

static int const (*LUA_DoString)(const char*, const char*, const char*) = (int const(*)(const char*, const char*, const char*)) LUA_DoString_addr;
static int const (*SelectUnit)(GUID_t) = (int const(*)(GUID_t)) SelectUnit_addr;

HINSTANCE  inj_hModule;          //Injected Modules Handle

HANDLE glhProcess;

static point_timestamp pt;

static int face_queued = 0;
static int broadcast_blast = 0;

static BYTE original_opcodes[8];

static int BLAST_ENABLED = 0;
static int CONSTANT_CTM_TARGET = 0;

static void __stdcall walk_to_target();

enum {
	CTM_MOVE = 0x4,
	CTM_TALK_NPC = 0x5,
	CTM_LOOT = 0x6,
	CTM_MOVE_AND_ATTACK = 0xA
};

void __stdcall print_errcode(int code) {
	printf("Spell cast failed, errcode %d\n", code);
}

static int patch_LUA_prot(HANDLE hProcess) {

	static LPVOID lua_prot_addr = (LPVOID)0x49DBA0;
	static BYTE patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
	WriteProcessMemory(hProcess, lua_prot_addr, patch, sizeof(patch), NULL);
	return 1;
}

// this __declspec(noinline) thing has got to do with the msvc optimizer.
// seems like the inline assembly is discarded when this func is inlined, in which case were fucked
__declspec(noinline) static void set_facing(float x) {
	void const (*SetFacing)(float) = (void const (*)(float))0x007B9DE0;

	uint this_ecx = DEREF(0xE29D28) + 0xBE0;

	printf("set_facing: this_ecx: %X\n", this_ecx);
	// this is due to the fact that SetFacing is uses a __thiscall calling convention, so the base addr needs to be passed in ECX. no idea which base address this is though :D

	__asm push ecx
	__asm mov ecx, this_ecx
	SetFacing(x);
	__asm pop ecx;

}

static void click_to_move(vec3 point, uint action, GUID_t interact_GUID) {
	static const uint
		CTM_X = 0xD68A18,
		CTM_Y = 0xD68A1C,
		CTM_Z = 0xD68A20,
		CTM_push = 0xD689BC,
		CTM_mystery = 0xD689AE,
		CTM_GUID = 0xD689C0, // this is for interaction
		CTM_MOVE_ATTACK_ZERO = 0xD689CC; // this must be 0 for at least CTM_MOVE_AND_ATTACK, otherwise segfault
	// explanation: the function 7BCB00 segfaults at 7BCB14, if it's not
	// can't remember which function calls 7BCB00, but the branch
	// isn't taken when there's a 0 at D689CC. :D


	// seems like addresses D689A0 D689A4 D689A8 are floats, maybe some angles?
	// A0 = the angle where the character should be walking?
	// D689A8 and D689AC are weird..
	// for walking, "mystery" is a constant float 0.5

	writeAddr(CTM_X, &point.x, sizeof(point.x));
	writeAddr(CTM_Y, &point.y, sizeof(point.y));
	writeAddr(CTM_Z, &point.z, sizeof(point.z));
	
	uint zero = 0;

	if (interact_GUID != 0) {
		writeAddr(CTM_GUID, &interact_GUID, sizeof(GUID));
		if (action == CTM_MOVE_AND_ATTACK) {
			writeAddr(CTM_MOVE_ATTACK_ZERO, &zero, sizeof(zero));
		}
	}


	//	writeAddr(CTM_mystery, &mystery, 2);
	writeAddr(CTM_push, &action, sizeof(action));

}


int dump_wowobjects_to_log() {

	ObjectManager OM;

	if (!OM.valid()) {
		fprintf(stderr, "WowObject dumping failed: ObjectManager base addr seems to be NULL.\n(Is your character in the world?)\n");
		return 0;
	}

	FILE *fp = fopen("C:\\Users\\elias\\Desktop\\out.log", "a");

	if (fp) {
		WowObject next = OM.getFirstObject();
		GUID_t target_GUID;
		readAddr(PLAYER_TARGET_ADDR, &target_GUID, sizeof(target_GUID));
		fprintf(fp, "local GUID = 0x%016llX, player target: %016llX\n", OM.get_localGUID(), target_GUID);

		while (next.valid()) {

			if (next.get_type() == 3 || next.get_type() == 4) { // 3 = NPC, 4 = Unit
				vec3 pos = next.get_pos();
				fprintf(fp, "object GUID: 0x%016llX, base addr = %X, type: %s, coords = (%f, %f, %f), rot: %f\n",
					next.get_GUID(), next.get_base(), next.get_type_name().c_str(), pos.x, pos.y, pos.z, next.get_rot());

				// print bytes
				if (next.get_type() == 3) {
					fprintf(fp, "name: %s, health: %d/%d\n\n", next.NPC_get_name().c_str(), next.NPC_getCurHealth(), next.NPC_getMaxHealth());
				}
				else if (next.get_type() == 4) {
					fprintf(fp, "name: %s, target guid: %llX\n\n", next.unit_get_name().c_str(), next.unit_get_target_GUID());
				}
			}

			next = next.getNextObject();
		}
		fclose(fp);

	}
	return 1;
}

static void __stdcall walk_to_target() {

	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	click_to_move(t.get_pos(), CTM_MOVE, 0);

}

static void __stdcall face_target() {
	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return;

	vec3 diff = p.get_pos() - t.get_pos();

	// this formula is for a directed angle (clockwise angle). atan2(det, dot). 
	// now we're taking the angle with respect to the x axis (north in wow), so the computation becomes simply:
	float directed_angle = atan2(diff.y, diff.x);

	//printf("player coords: (%f, %f, %f), target coords: (%f, %f, %f), diff = (%f, %f, %f)\nangle = %f, player rot = %f\n", 
		//pc.x, pc.y, pc.z, tc.x, tc.y, tc.z, diff.x, diff.y, diff.z, directed_angle, p.get_rot());

	// the wow angle system seems to be counter-clockwise

	set_facing(directed_angle + M_PI);

}

static void __stdcall melee_behind_target() {
	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return;

	float target_rot = t.get_rot();

	// basically rotating the vector (1, 0, 0) target_rot radians anti-clockwise
	//point rot_unit(1.0 * std::cos(target_rot) - 0.0 * std::sin(target_rot), 1.0 * std::sin(target_rot) + 0.0 * std::cos(target_rot), 0.0);
	// point rot_unit(std::cos(target_rot), std::sin(target_rot), 0.0);
	vec3 pc = p.get_pos();
	vec3 tc = t.get_pos();

	vec3 point_behind(tc.x - std::cos(target_rot), tc.y - std::sin(target_rot), tc.z);

	vec3 diff = pc - point_behind;

	//printf("melee_behind.. diff.length: %f\n", diff.length());

	if (diff.length() < 1.0) {
		face_queued = 1;
		return;
	}
	else {
		click_to_move(point_behind, CTM_MOVE, 0);
	}

}


static int player_is_moving() {


	ObjectManager OM;
	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return -1;

	static const float normal_speed_per_tick = 2.18e-6;

	point_timestamp new_pt = p.get_pos();

	LONGLONG tick_diff = new_pt.ticks.QuadPart - pt.ticks.QuadPart;

	//if (tick_diff > 500000) { // on my machine, 200000 ticks is (roughly) every third frame, so about 46ms. 
		// so basically, if the player hasn't moved enough in about 100ms, it can be considered stationary
		// 100% run speed is about 2.18e-6 units ("yards") per tick

	vec3 diff = new_pt.p - pt.p;

	// 0.47 / 216000 is roughly walking speed

	if (diff.length() > 0.15) { return 1; }

	pt = new_pt;

	return 1;
}


static void __stdcall every_frame_hook_func() {

	static int every_third_frame = 0;
	static const char* script = "RunMacro(\"lole\")";
	static const char* n = "";

	if (broadcast_blast) {
		if (BLAST_ENABLED) {
			printf("broadcasting BLAST_ON to party members.\n");
			static const char* blast_on = "SendAddonMessage(\"lole_blast\", \"BLAST_ON\", \"PARTY\")";
			LUA_DoString(blast_on, blast_on, "");
		}
		else {
			printf("broadcasting BLAST_OFF to party members.\n");
			static const char* blast_off = "SendAddonMessage(\"lole_blast\", \"BLAST_OFF\", \"PARTY\")";
			LUA_DoString(blast_off, blast_off, "");
		}
		broadcast_blast = 0;
	}


	if (every_third_frame == 0) {
		if (BLAST_ENABLED) {
			LUA_DoString(script, script, n);
		}
		if (CONSTANT_CTM_TARGET) {
			walk_to_target();
		}

		if (face_queued) { // this is a trick to not mess up the main thread
			face_target();
			face_queued = 0;
		}
	}

	every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;


}


static void change_target(const std::string &arg) {

	static const std::string prefix = "LOLE_TARGET_GUID:";

	std::string GUID_numstr(arg.c_str() + 2); // better make a copy of it. the GUID_str still has the "0x" prefix in it 

	char *end;
	GUID_t GUID = strtoull(GUID_numstr.c_str(), &end, 16);

	printf("got LOLE_TARGET_GUID: GUID = %llX\nGUID_str + prefix.length() + 2 = \"%s\"\n", GUID, GUID_numstr.c_str());

	if (end != GUID_numstr.c_str() + GUID_numstr.length()) {
		printf("[WARNING]: change_target: couldn't wholly convert GUID string argument (strtoull(\"%s\", &end, 16) failed, bailing out\n", GUID_numstr.c_str());
		return;
	}

	SelectUnit(GUID);
}

static void blast(const std::string &arg) {
	if (arg == "BLAST_ON") {
		printf("got BLAST_ON AddonMessage, enabling blast.\n");
		BLAST_ENABLED = 1;
	}
	else if (arg == "BLAST_OFF") {
		printf("got BLAST_OFF AddonMessage, disabling blast.\n");
		BLAST_ENABLED = 0;
	}
	else {
		printf("blast (from DelIgnore_hub): warning: unknown argument \"%s\"\n", arg.c_str());
		BLAST_ENABLED = 0;
	}
}

typedef void(*hubfunc_t)(const std::string &);

static const struct {
	std::string prefix;
	hubfunc_t func;
} hubfuncs[] = {
	{"LOLE_TARGET_GUID", change_target},
	{"LOLE_BLAST", blast}
};


static void __stdcall DelIgnore_hub(const char* arg_) {
	std::string arg(arg_);
	auto p = arg.find(':');

	if (p == std::string::npos) {
		printf("Error: DelIgnore_hub: couldn't extract prefix from string \"%s\"; expected to find \':\'!\n", arg_);
		return;
	}

	std::string prefix = arg.substr(0, p);

	for (auto &f : hubfuncs) {
		if (f.prefix == prefix) {
			f.func(arg.c_str() + f.prefix.length() + 1); // just pass the arg without the prefix and the ':'
		}
	}
}


static int hook_all() {
	
	install_hook("EndScene", every_frame_hook_func);
	install_hook("DelIgnore", DelIgnore_hub);
	install_hook("ClosePetStables", melee_behind_target);
	
	return 1;
}

static int unhook_all() {
	
	uninstall_hook("EndScene");
	uninstall_hook("DelIgnore");
	uninstall_hook("ClosePetStables");
	
	return 1;
}


DWORD WINAPI ThreadProc(LPVOID lpParam) {
	MSG messages;
	char *pString = reinterpret_cast<char *> (lpParam);
	HMENU hMenu = CreateDLLWindowMenu();
	RegisterDLLWindowClass("DLLWindowClass", inj_hModule);

	HWND hwnd = CreateWindowEx(0, "DLLWindowClass", pString, WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, hMenu, inj_hModule, NULL);
	ShowWindow(hwnd, SW_SHOWNORMAL);

	if (RegisterHotKey(hwnd, 100, MOD_ALT, 'G')) {
		printf("Registered window %X as blast client!\n", (DWORD)hwnd);
	}

	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 1;
}


LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	int lo, hi;
	switch (message) {

	case WM_COMMAND:
		switch (wParam)
		{
		case MYMENU_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case MYMENU_DUMP:
			dump_wowobjects_to_log();
			break;
		case MYMENU_HOOK:
			hook_all();
			break;

		case MYMENU_UNHOOK:
			unhook_all();
			break;
		case MYMENU_CTM: {

			ObjectManager OM;
			WowObject o = OM.get_object_by_GUID(OM.get_localGUID());
			if (!o.valid()) {
				break;
			}

			WowObject t = OM.get_object_by_GUID(o.unit_get_target_GUID());
			if (!t.valid()) {
				break;
			}
			click_to_move(t.get_pos(), CTM_MOVE_AND_ATTACK, t.get_GUID());
			break;
		}
		case MYMENU_CTM_CONSTANT: {
			CONSTANT_CTM_TARGET = !CONSTANT_CTM_TARGET;
			printf("CONSTANT_CTM: %d\n", CONSTANT_CTM_TARGET);
			break;
		}
		}
		break;
	case WM_HOTKEY:
		lo = LOWORD(lParam);
		hi = HIWORD(lParam);

		if (lo == MOD_ALT) {
			if (hi == MKEY_G) {
				BLAST_ENABLED = !BLAST_ENABLED;
				broadcast_blast = 1;
				printf("G pressed, BLAST = %d\n", BLAST_ENABLED);
				return TRUE;
			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	HANDLE windowThread = INVALID_HANDLE_VALUE;

	glhProcess = hProcess;

	AllocConsole();
	freopen("CONOUT$", "wb", stdout);

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		patch_LUA_prot(hProcess);
		windowThread = CreateThread(0, NULL, ThreadProc, (LPVOID)"Dump", NULL, NULL);
		inj_hModule = hModule;
		
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");
		// might want to suspend the thread :DD
		PostThreadMessage(GetThreadId(windowThread), WM_DESTROY, 0, 0);
		//WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

