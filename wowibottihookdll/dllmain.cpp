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
#include "window.h"

HINSTANCE  inj_hModule;          // HANDLE for injected module

HANDLE glhProcess;

static int face_queued = 0;

static BYTE original_opcodes[8];

static void __stdcall walk_to_target();

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


DWORD WINAPI ThreadProc(LPVOID lpParam) {
	MSG messages;
	char *pString = reinterpret_cast<char *> (lpParam);
	HMENU hMenu = CreateDLLWindowMenu();
	RegisterDLLWindowClass("DLLWindowClass", inj_hModule);

	//HWND hwnd = CreateWindowEx(0, "DLLWindowClass", pString, WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, hMenu, inj_hModule, NULL);
	//ShowWindow(hwnd, SW_SHOWNORMAL);

	hook_all();

	//if (RegisterHotKey(hwnd, 100, MOD_ALT, 'G')) {
		//printf("Registered window %X as blast client!\n", (DWORD)hwnd);
	//}

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
			unhook_all();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
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

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		
		patch_LUA_prot(hProcess);
		//hook_all();
		windowThread = CreateThread(0, NULL, ThreadProc, (LPVOID)"Dump", NULL, NULL);
		inj_hModule = hModule;

		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");

		PostThreadMessage(GetThreadId(windowThread), WM_DESTROY, 0, 0);
		//WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

