// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

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

#ifdef _DEBUG
#define DEBUG_CONSOLE
#endif

static HINSTANCE inj_hModule;          // HANDLE for injected module
HANDLE glhProcess;
HWND wow_hWnd;

int afkjump_keyup_queued = 0;
static int enter_world = 0;


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

static int handle_login_creds() {

	printf("@ handle_login_creds\n");
	static const size_t BUF_SIZE = 256;

	std::string cred_fd = "Local\\lole_login_" + std::to_string(GetCurrentProcessId());

	HANDLE login_map = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, cred_fd.c_str());
	DWORD err = GetLastError();
	
	if (err == NO_ERROR) {
		// then the lole launcher has assigned this client some login credentials to use
		char buf[BUF_SIZE];
		LPVOID fd_addr = MapViewOfFile(login_map, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);

		if (fd_addr == NULL) {
			MessageBox(NULL, ("error with MapViewOfFile: " + std::to_string(GetLastError())).c_str(), "mro", MB_OK);
			return 1;
		}

		CopyMemory(buf, fd_addr, BUF_SIZE);

		std::vector<std::string> cred_tokens;
		tokenize_string(buf, ",", cred_tokens);

		if (cred_tokens.size() != 3) {
			MessageBox(NULL, "The login credentials assigned to us were invalid. Expected 3 elements.", "mro", MB_OK);
			return 0;
		}

//		credentials = cred_t(cred_tokens[0], cred_tokens[1], cred_tokens[2]);

	}
	else {
		printf("warning: OpenFileMapping returned error %d for account credential handling! fd name: %s\n", err, cred_fd.c_str());
	}

	CloseHandle(login_map);

	return 1;
}

DWORD WINAPI init_func(LPVOID lpParam) {
	DWORD main_tid = *(DWORD*)lpParam;
	HANDLE main_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, main_tid);
	if (!main_thread) {
		printf("Couldn't open main thread: %d\n", GetLastError());
		return 0;
	}
	SuspendThread(main_thread);

	handle_login_creds();
	// for whatever reason, the endscene hook only works in Debug mode.
	hook_all();
	hook_EndScene();

	ResumeThread(main_thread);

	CloseHandle(main_thread);

	return 1;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	DWORD processID = GetCurrentProcessId();
	//std::thread hook_thread;

	glhProcess = hProcess;

#define PIPE_WRITE_BUF_SIZE 1024
#define PIPE_READ_BUF_SIZE 16

	std::string pipe_name = "\\\\.\\pipe\\" + std::to_string(processID);
	HANDLE hPipe;
	char *write_buf = new char[PIPE_WRITE_BUF_SIZE];
	char *read_buf = new char[PIPE_READ_BUF_SIZE];
	DWORD num_bytes;

	strcpy(write_buf, (std::string("HELLOU MATAFAKAAZ FROM ") + std::to_string(processID)).c_str());

	DWORD sc;

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {

		inj_hModule = hModule;
#ifdef DEBUG_CONSOLE
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
#endif

		hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		while (!hPipe) {
			CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		}

		printf("Got connection to pipe %s!\n", pipe_name.c_str());

		sc = WriteFile(hPipe, write_buf, strlen(write_buf) + 1, &num_bytes, NULL);

		if (!sc || num_bytes == 0) {
			printf("Writing to pipe %s failed, error: 0x%X\n", pipe_name.c_str(), GetLastError());
		}
		else {
			printf("Sent response %s to pipe server!\n", write_buf);
		}

		sc = ReadFile(hPipe, read_buf, PIPE_READ_BUF_SIZE, &num_bytes, NULL);

		if (!sc || num_bytes == 0) {
			printf("Reading from pipe %s failed, error: 0x%X\n", pipe_name.c_str(), GetLastError());
		}
		else {
			read_buf[num_bytes] = '\0';

			printf("Got response %s from pipe server\n", read_buf);
		}

		CloseHandle(hPipe);

//		DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D



		break;
	}
	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		delete[] write_buf;
		break;
	}

	return TRUE;
}

