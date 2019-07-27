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

#include <process.h>

int afkjump_keyup_queued = 0;
static int enter_world = 0;

PPIPE_t *PIPE;

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

PPIPE_t *create_pipe() {
	
	PPIPE_t *p = new PPIPE_t;

	p->path = "\\\\.\\pipe\\" + std::to_string(GetCurrentProcessId());

	p->hPipe = CreateNamedPipe(p->path.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, PIPE_READ_BUF_SIZE, PIPE_WRITE_BUF_SIZE, 0, NULL);

	if (!p->hPipe) {
		PRINT("CreateNamedPipe for pipe %s returned INVALID_HANDLE_VALUE; GetLastError() = %d. Retrying.\n", p->path.c_str(), GetLastError());
		delete p;
		return NULL;
	}

	p->write_buf = new char[PIPE_WRITE_BUF_SIZE];
	p->read_buf = new char[PIPE_READ_BUF_SIZE];

	return p;
}

void destroy_pipe(PPIPE_t *p) {
	FlushFileBuffers(p->hPipe);
	DisconnectNamedPipe(p->hPipe);

	CloseHandle(p->hPipe);

	delete[] p->read_buf;
	delete[] p->write_buf;

	delete p;
}

int request_patch() {

	int r = 0;

	while ((r = PIPE->write(&PIPEDATA.data[0], PIPEDATA.data.size())) == 0); // 0 means ERROR_PIPE_LISTENING

	// now listen to what wowipotti's answer
	r = PIPE->read();

	std::vector<std::string> tokens;
	tokenize_string(PIPE->read_buf, ";", tokens);

	if (tokens.size() > 1) {
		PRINT("DEBUG got more than 1 tokens\n");

		for (auto &s : tokens) {
			std::vector<std::string> L2;
			tokenize_string(s, "=", L2);

			if (L2.size() > 1) {

				if (L2[0] == "CREDENTIALS") {
					if (L2.size() != 2) {
						PRINT("parse_credentials: error: malformed credentials (tokenized vector size != 2!)\n");
						break;
					}
					std::vector<std::string> L3;
					tokenize_string(L2[1], ",", L3);

					if (L3.size() != 3) {
						PRINT("parse_credentials: error: invalid number of tokens in credential string! Expected 3, got %d\n", L3.size());
						break;
					}

					credentials = cred_t(L3[0], L3[1], L3[2]);
				}
			}
		}
	}
	else {
		if (tokens[0] == "PATCH_FAIL") {
			PRINT("Got PATCH_FAIL. RIP.\n");
			return 0;
		}
		else if (tokens[0] == "PATCH_OK") {
			PRINT("Got PATCH_OK. No login credentials sent.\n");
			return 1;
		}
	}
}

int handle_patching() {

	DWORD num_bytes;
	DWORD sc;

	if (!PIPE) {
		PIPE = create_pipe();
	}

	if (PIPE) {
		PRINT("Successfully created pipe %s!\n", PIPE->path.c_str());
		PRINT("PIPEDATA.data.size() = %d\n", PIPEDATA.data.size());
	}
	else {
		return 0;
	}

	int r = request_patch();
	
	destroy_pipe(PIPE);
	PIPE = NULL;

	return 1;

}

void __cdecl DO_STUFF(void *args) {
	glhProcess = GetCurrentProcess();

#ifdef DEBUG_CONSOLE
	AllocConsole();
	freopen_s(&out, "CONOUT$", "wb", stdout);
#endif

	handle_patching();
	//dscript_read_all();

	EnumWindows(EnumWindowsProc, NULL);

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	inj_hModule = hModule;

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		prepare_pipe_data();
		_beginthread(DO_STUFF, 0, NULL);
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

