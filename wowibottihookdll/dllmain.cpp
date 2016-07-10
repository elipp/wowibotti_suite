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


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	DWORD pid = GetCurrentProcessId();
	inj_hModule = hModule;
	glhProcess = hProcess;

#define PIPE_WRITE_BUF_SIZE 1024
#define PIPE_READ_BUF_SIZE 16

	std::string pipe_name;
	HANDLE hPipe;
	char *write_buf, *read_buf;
	DWORD num_bytes;
	DWORD sc;


	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
#ifdef DEBUG_CONSOLE
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
#endif

		prepare_patches_and_pipe_data(); 

		pipe_name = "\\\\.\\pipe\\" + std::to_string(pid);

		write_buf = new char[PIPE_WRITE_BUF_SIZE];
		read_buf = new char[PIPE_READ_BUF_SIZE];

		hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		while (!hPipe) {
			CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		}

		printf("Got connection to pipe %s!\n", pipe_name.c_str());
		printf("PIPEDATA.data.size() = %d\n", PIPEDATA.data.size());

		sc = WriteFile(hPipe, &PIPEDATA.data[0], PIPEDATA.data.size(), &num_bytes, NULL);

		if (!sc || num_bytes == 0) {
			printf("Writing to pipe %s failed, error: 0x%X\n", pipe_name.c_str(), GetLastError());
		}
		else {
			printf("Sent our patch data to pipe server!\n", write_buf);
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

		delete[] read_buf;
		delete[] write_buf;

		break;
	}
	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

