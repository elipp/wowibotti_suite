// wowipotti2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include <cstdio>
#include <tchar.h>
#include <string>
#include <cassert>

#include "wowipotti2.h"

static HINSTANCE hInst;	
static HWND main_window_hWnd;
static WNDPROC editbox_original_wndproc;

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

	printf("DLL injection to pid %d successful!\n", pid);
	delete[] DirPath;
	delete[] FullPath;

	return 1;

}

static int inject_to_all() {

	wow_handles = std::vector<wowcl>();

	EnumWindows(EnumWindowsProc, NULL);

	for (auto c : wow_handles) {
		if (!inject_dll(c.window_handle)) return 0;
	}

	return 1;
}

static int validate_editbox_value_apply(HWND hWnd) {

	char textbuf[64];
	long value;
	char *endptr;

	GetWindowText(hWnd, textbuf, 64);

	value = strtol(textbuf, &endptr, 10);	// the editbox has ES_NUMBER, so it's quite hard to come up with an invalid value :P

	if (value >= 0 && value <= 25) {
		// considered valid
		//t.t_ms = value;
		//t.str = std::to_string(value);

	}
	else {
		SetWindowText(hWnd, "1");
		SendMessage(hWnd, EM_SETSEL, 1, 1);
	}

	return 1;
}

LRESULT CALLBACK numeric_editbox_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	switch (msg) {

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			validate_editbox_value_apply(hWnd);
			break;
		default:
			break;
		}

		break;

	case WM_KILLFOCUS:
		validate_editbox_value_apply(hWnd);
		CallWindowProc(editbox_original_wndproc, hWnd, msg, wParam, lParam);
		break;

	default:
		return CallWindowProc(editbox_original_wndproc, hWnd, msg, wParam, lParam);
	}

	return 0;
}

static int launch_clients(HWND dlg) {
	HWND edit = GetDlgItem(dlg, ID_EDIT_NUMCLIENTS);

	char textbuf[64];
	long value;
	char *endptr;

	GetWindowText(edit, textbuf, 64);

	value = strtol(textbuf, &endptr, 10);	// the editbox has ES_NUMBER, so it's quite hard to come up with an invalid value :P

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;


	for (int i = 0; i < value; ++i) {
		CreateProcess("C:\\Users\\Elias\\Downloads\\Feenix 2.4.3 client\\Wow.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
	}

	return 1;

}


static int set_affinities() {
	
	wow_handles = std::vector<wowcl>();

	EnumWindows(EnumWindowsProc, NULL);

	DWORD pid;
	int n = 0;

	for (auto c : wow_handles) {
		GetWindowThreadProcessId(c.window_handle, &pid);
		HANDLE proc_handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

		DWORD aff_mask = (0x3 << (n * 2));

		SetProcessAffinityMask(proc_handle, aff_mask);
		SetPriorityClass(proc_handle, HIGH_PRIORITY_CLASS);
		n = (n < 3) ? (n + 1) : 0;

		printf("proc_handle = %X, PID = %d, aff_mask = %X\n", proc_handle, pid, aff_mask);

		CloseHandle(proc_handle);

	}

	return 1;
}



INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int lo, hi;
	switch (message) {

	case WM_COMMAND:
		lo = LOWORD(wParam);
		hi = HIWORD(wParam);

		switch (lo) {
		//case ID_CHECKBOXBLAST:
		//	toggle_blast();
		//	return TRUE;
		//	break;
		case ID_UPDOWN_NUMCLIENTS:
			return TRUE;
			break;
		case ID_BUTTON_LAUNCH:
			launch_clients(hWnd);
			return TRUE;
			break;
	
		case ID_BUTTON_AFFINITY:
			set_affinities();
			return TRUE;
			break;

		case ID_BUTTON_INJECT:
			inject_to_all();
			return TRUE;
			break;

		default:
			break;
		}

	case WM_CREATE:
		break;

	case WM_HOTKEY:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	}

	return FALSE;
}

static void errcode_messagebox() {
	auto e = GetLastError();
	std::string errstr = std::to_string(e);

	MessageBox(NULL, errstr.c_str(), "erzar", MB_OK);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, DlgProc, 0);

	if (!hWnd)
		return FALSE;
	

	HWND num_clients_editbox = GetDlgItem(hWnd, ID_EDIT_NUMCLIENTS);
	editbox_original_wndproc = (WNDPROC)SetWindowLongPtr(num_clients_editbox, GWLP_WNDPROC, (LONG_PTR)numeric_editbox_wndproc);
	SetWindowText(num_clients_editbox, "0");
	SendMessage(num_clients_editbox, EM_SETSEL, 1, 1);

	HWND numclients_spin = GetDlgItem(hWnd, ID_UPDOWN_NUMCLIENTS); assert(numclients_spin != INVALID_HANDLE_VALUE);
	SendMessage(numclients_spin, UDM_SETRANGE32, 0, 25);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	main_window_hWnd = hWnd;

	return TRUE;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	INITCOMMONCONTROLSEX ic;

	ic.dwSize = sizeof(ic);
	ic.dwICC = ICC_STANDARD_CLASSES;

	InitCommonControlsEx(&ic);

	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}


	if (!obtain_debug_privileges()) {
		printf("\nwowipotti2: ERROR: couldn't obtain debug privileges.\n(Are you running as administrator?)\n\nPress enter to exit.\n");
		getchar();
		return 0;
	}
	

	
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);


	MSG msg;

	int ret;
	while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (ret == -1)
			return -1;
		if (!IsDialogMessage(main_window_hWnd, &msg) || msg.wParam == VK_RETURN) {	// so that tab works on the dialog controls
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;

	return 0;
}

