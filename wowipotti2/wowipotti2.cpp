// wowipotti2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <vector>
#include <cstdio>

#define MAX_LOADSTRING 100

struct wowcl {
	HWND window_handle;
	std::string window_title;
	int valid;
	wowcl() {};
	wowcl(HWND hWnd, std::string &title)
		: window_handle(hWnd), window_title(title), valid(1) {}
};

std::vector<wowcl> wow_handles;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {

	char title[MAX_LOADSTRING];
	char class_name[MAX_LOADSTRING];
	
	GetClassName(hWnd, class_name, sizeof(class_name));
	GetWindowText(hWnd, title, sizeof(title));

	if (strcmp(title, "World of Warcraft") == 0) {
		wow_handles.push_back(wowcl(hWnd, std::string(title)));
	}

	return TRUE;
}

static BOOL SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege) {

	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES tpPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

	if (!LookupPrivilegeValue(NULL, Privilege, &luid)) return FALSE;

	// 
	// first pass.  get current privilege setting
	// 
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		&tpPrevious,
		&cbPrevious
		);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	// 
	// second pass.  set privilege based on previous setting
	// 
	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnablePrivilege) {
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	}
	else {
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
			tpPrevious.Privileges[0].Attributes);
	}

	AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tpPrevious,
		cbPrevious,
		NULL,
		NULL
		);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	return TRUE;
}



static int obtain_debug_privileges() {
	HANDLE hToken;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			if (!ImpersonateSelf(SecurityImpersonation))
				return 0;

			if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
				printf("OpenThreadToken pheyled:(\n");
				return 0;
			}
		}
		else
			return 0;
	}

	// enable SeDebugPrivilege
	if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
	{
		fprintf(stderr, "SetPrivilege pheyled.\n");
		fprintf(stderr, "ERROR: WINAPI SetPrivilege() failed! DLL injection will fail.\n");

		// close token handle
		CloseHandle(hToken);

		// indicate failure
		return 0;
	}
	return 1;
}



static int inject_dll(HWND window_handle) {

	char* DirPath = new char[MAX_PATH];
	char* FullPath = new char[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, DirPath);

	sprintf_s(FullPath, MAX_PATH, "%s\\wowibottihookdll.dll", DirPath);
	//sprintf_s(FullPath, MAX_PATH, "C:\\Users\\Elias\\Documents\\Visual Studio 2015\\Projects\\wowibottihookdll\\Release\\wowibottihookdll.dll");
	printf("%s\n", FullPath);

	DWORD pid;
	DWORD tid = GetWindowThreadProcessId(window_handle, &pid);

	printf("pid = %d, tid = %d\n", pid, tid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL) { fprintf(stderr, "OpenProcess failed: %d\n", GetLastError()); return 0; }

	LPVOID LoadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	if (LoadLibraryAddr == NULL) { fprintf(stderr, "LoadLibraryA failed: %d\n", GetLastError()); return 0; }

	LPVOID LLParam = (LPVOID)VirtualAllocEx(hProcess, NULL, strlen(FullPath),
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (LLParam == NULL) { fprintf(stderr, "VirtualAllocEx failed: %d\n", GetLastError()); return 0; }

	DWORD ret = WriteProcessMemory(hProcess, LLParam, FullPath, strlen(FullPath), NULL);
	if (ret == 0) { fprintf(stderr, "WriteProcessMemory failed: %d\n", GetLastError()); return 0; }

	HANDLE rt = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryAddr,
		LLParam, NULL, NULL);
	if (rt == NULL) { fprintf(stderr, "CreateRemoteThread failed: %d\n", GetLastError()); return 0; }

	CloseHandle(hProcess);

	printf("injection successful! lole away!!\n");
	delete[] DirPath;
	delete[] FullPath;

	return 1;

}


int main()
{
	if (!obtain_debug_privileges()) {
		printf("\nwowipotti2: ERROR: couldn't obtain debug privileges.\n(Are you running as administrator?)\n\nPress enter to exit.\n");
		getchar();
		return 0;
	}
	
	EnumWindows(EnumWindowsProc, NULL);
	
	for (auto c : wow_handles) {
		inject_dll(c.window_handle);
		printf("wowipotti2: injected wowibottihookdll to window %X\n", c.window_handle);
	}

	return 0;
}

