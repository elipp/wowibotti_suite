// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <D3D9.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <thread>

#include "addrs.h"
#include "ctm.h"
#include "defs.h"
#include "dungeon_script.h"
#include "wowmem.h"
#include "hooks.h"
#include "opcodes.h"
#include "timer.h"
#include "creds.h"
#include "packet.h"
#include "dllmain.h"

HINSTANCE inj_hModule;          // HANDLE for injected module
HANDLE glhProcess;
HWND wow_hWnd;

std::string DLL_path;
std::string DLL_base_path;

#include <process.h>

int afkjump_keyup_queued = 0;
static int enter_world = 0;

static FILE *out;

void close_console() {
#ifdef DEBUG_CONSOLE
	fclose(out);
	FreeConsole();
#endif
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
			//PRINT("got window HWND! (pid = %d)\n", pid);
		}
	}

	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		inj_hModule = hModule;
		glhProcess = GetCurrentProcess();
		auto endscene_patch = patch_t(PATCHADDR_LATER, 7, prepare_EndScene_patch);
		endscene_patch.enable(glhProcess);
		break;
	}
	case DLL_THREAD_ATTACH:
	//	printf("ATTACH\n");
		break;

	case DLL_THREAD_DETACH:
	//	printf("THREAD_DETAHCH\n");
		break;

	case DLL_PROCESS_DETACH:
	//	printf("PROCESS_DETACH\n");

		break;
	}

	return TRUE;
}

