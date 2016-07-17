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


int handle_pipe_stuff() {
#define PIPE_WRITE_BUF_SIZE 1024
#define PIPE_READ_BUF_SIZE 1024

	HANDLE hPipe;
	DWORD num_bytes;
	DWORD sc;

	DWORD pid = GetCurrentProcessId();

	prepare_patches_and_pipe_data();

	std::string pipe_name = "\\\\.\\pipe\\" + std::to_string(pid);

	char *write_buf = new char[PIPE_WRITE_BUF_SIZE];
	char *read_buf = new char[PIPE_READ_BUF_SIZE];

	hPipe = CreateNamedPipe(pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, PIPE_READ_BUF_SIZE, PIPE_WRITE_BUF_SIZE, 0, NULL);

//	hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);
	
	if (!hPipe) {
		printf("CreateNamedPipe for pipe %s returned INVALID_HANDLE_VALUE; GetLastError() = %d. Retrying.\n", pipe_name.c_str(), GetLastError());
		return 0;
	}

	printf("Successfully created pipe %s!\n", pipe_name.c_str());
	printf("PIPEDATA.data.size() = %d\n", PIPEDATA.data.size());

//	sc = WriteFile(hPipe, &PIPEDATA.data[0], PIPEDATA.data.size(), &num_bytes, NULL);

	while (1) {
		sc = WriteFile(hPipe, &PIPEDATA.data[0], PIPEDATA.data.size(), &num_bytes, NULL);

		if (!sc || num_bytes == 0) {
			if (GetLastError() == ERROR_PIPE_LISTENING) {
				//printf("pipe thread: WARNING: ERROR_PIPE_LISTENING!\n");
			}
			else if (GetLastError() == ERROR_BROKEN_PIPE) {
				printf("pipe thread: ERROR_BROKEN_PIPE\n");
				break;
			}
			else {
				printf("pipe thread: ERROR: 0x%X\n", GetLastError());
				break;
			}
		}
		else {
			break; // successe'd
		}

		//Sleep(200);
	}
	
	sc = ReadFile(hPipe, read_buf, PIPE_READ_BUF_SIZE, &num_bytes, NULL);

	if (!sc || num_bytes == 0) {
		printf("Reading from pipe %s failed, error: 0x%X\n", pipe_name.c_str(), GetLastError());
		return 0;
	}
	else {
		read_buf[num_bytes] = '\0';
		printf("Got response %s from pipe server\n", read_buf);
	}

	std::vector<std::string> tokens;
	tokenize_string(read_buf, ";", tokens);

	if (tokens.size() > 1) {
		printf("DEBUG got more than 1 tokens\n");
		
		for (auto &s : tokens) {
			std::vector<std::string> L2;
			tokenize_string(s, "=", L2);

			if (L2.size() > 1) {
		
				if (L2[0] == "CREDENTIALS") {
					if (L2.size() != 2) {
						printf("parse_credentials: error: malformed credentials (tokenized vector size != 2!)\n");
						break;
					}
					std::vector<std::string> L3;
					tokenize_string(L2[1], ",", L3);

					if (L3.size() != 3) {
						printf("parse_credentials: error: invalid number of members in credential string! Expected 3, got %d\n", L3.size());
						break;
					}

					credentials = cred_t(L3[0], L3[1], L3[2]);
				}
			}
		}
	}
	else {
		if (tokens[0] == "PATCH_FAIL") {
			printf("Got PATCH_FAIL. RIP. No idea what's going to happen next.\n");
		}
		else if (tokens[0] == "PATCH_OK") {
			printf("Got PATCH_OK. No login credentials sent.\n");
		}
	}
	
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);

//	CloseHandle(hPipe);

	delete[] read_buf;
	delete[] write_buf;

	return 1;

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	inj_hModule = hModule;
	glhProcess = GetCurrentProcess();

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
#ifdef DEBUG_CONSOLE
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
#endif

		handle_pipe_stuff();
	
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

