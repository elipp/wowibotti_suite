// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <D3D9.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>
#include <algorithm>

#include "addrs.h"
#include "ctm.h"
#include "defs.h"
#include "wowmem.h"
#include "hooks.h"
#include "opcodes.h"
#include "timer.h"

//#define ENABLE_DEBUG_CONSOLE

HINSTANCE  inj_hModule;          // HANDLE for injected module
HANDLE glhProcess;
HWND wow_hWnd;

int afkjump_keyup_queued = 0;

static BYTE original_opcodes[8];


void __stdcall print_errcode(int code) {
	printf("Spell cast failed, errcode %d\n", code);
}

static int patch_LUA_prot(HANDLE hProcess) {
	printf("Patching LUA protection function...");
	static LPVOID lua_prot_addr = (LPVOID)0x49DBA0;
	static BYTE patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
	WriteProcessMemory(hProcess, lua_prot_addr, patch, sizeof(patch), NULL);
	printf("OK!\n");
	return 1;
}



static void __stdcall EndScene_hook() {

	static int every_third_frame = 0;

	if (every_third_frame == 0) {
		refollow_if_needed();
		
		if (afkjump_keyup_queued > 1) --afkjump_keyup_queued;

		if (afkjump_keyup_queued == 1) {
			PostMessage(wow_hWnd, WM_KEYUP, VK_LEFT, get_KEYUP_LPARAM(VK_LEFT));
			afkjump_keyup_queued = 0;
		}
		loot_next(); 
		//ctm_lock_until_done();


	}

	every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;
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

	int num_args = opcode_get_num_args(op);

	printf("DelIgnore_hub: got opcode %lX -> %s\n", op, opcode_get_funcname(op).c_str());

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


static int hook_all() {
	
	install_hook("EndScene", EndScene_hook);
	install_hook("DelIgnore", DelIgnore_hub);
	//install_hook("ClosePetStables", melee_behind_target);
	//install_hook("CTM_aux", broadcast_CTM);
	install_hook("CTM_main", broadcast_CTM);
	
	return 1;
}

static int unhook_all() {
	
	uninstall_hook("EndScene");
	uninstall_hook("DelIgnore");
	uninstall_hook("ClosePetStables");
	
	return 1;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {

	char title[100];
	char class_name[100];

	GetClassName(hWnd, class_name, sizeof(class_name));
	GetWindowText(hWnd, title, sizeof(title));

	DWORD pid = GetCurrentProcessId();
	DWORD this_pid;

	if (strcmp(title, "World of Warcraft") == 0 && strcmp(class_name, "GxWindowClassD3d") == 0) {
		GetWindowThreadProcessId(hWnd, &this_pid);
		if (pid == this_pid) {
			wow_hWnd = hWnd;
			printf("got window HWND! (pid = %d)\n", pid);
		}
	}

	return TRUE;
}


DWORD WINAPI ThreadProc(LPVOID lpParam) {

	hook_all();

	DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D

	MSG messages;

	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 1;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hook_thread = INVALID_HANDLE_VALUE;

	glhProcess = hProcess;

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		
		patch_LUA_prot(hProcess);
		//hook_all();
		hook_thread = CreateThread(0, NULL, ThreadProc, (LPVOID)"Dump", NULL, NULL);
		inj_hModule = hModule;

#ifdef ENABLE_DEBUG_CONSOLE
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
#endif
		EnumWindows(EnumWindowsProc, NULL);

		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");

		PostThreadMessage(GetThreadId(hook_thread), WM_DESTROY, 0, 0);
		//WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

