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

	hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	while (!hPipe) {
		CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	}

	printf("Got connection to pipe %s!\n", pipe_name.c_str());
	printf("PIPEDATA.data.size() = %d\n", PIPEDATA.data.size());

	sc = WriteFile(hPipe, &PIPEDATA.data[0], PIPEDATA.data.size(), &num_bytes, NULL);

	if (!sc || num_bytes == 0) {
		printf("Writing to pipe %s failed, error: 0x%X\n", pipe_name.c_str(), GetLastError());
		return 0;
	}
	else {
		printf("Sent our patch data to pipe server!\n");
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

	cred_t creds;

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

					creds = cred_t(L3[0], L3[1], L3[2]);
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

	CloseHandle(hPipe);

	//		DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D

	delete[] read_buf;
	delete[] write_buf;

	return 1;

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	inj_hModule = hModule;
	glhProcess = GetCurrentProcess;

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

