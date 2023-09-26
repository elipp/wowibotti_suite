// wowipotti2.cpp : Defines the entry point for the console application.
//

#include <Windows.h>
#include <CommCtrl.h>
#include <AclAPI.h>

#include <vector>
#include <cstdio>
#include <tchar.h>
#include <string>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <thread>

#include <algorithm>
#include <cctype>
#include <string>

static std::string make_lower(const std::string &s) {
	std::string lower = s;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });

	return lower;
}


#include "UI.h"

static HINSTANCE hInst;
static HWND main_window_hWnd;

static HWND edit_num_clients_hWnd;
static HWND button_launch_hWnd;
static HWND button_assign_hWnd;
static HWND button_affinity_hWnd;
static HWND button_inject_hWnd;
static HWND button_refresh_hWnd;
static HWND updown_hWnd;
static HWND pathedit_hWnd;
static HWND numclients_static_hWnd;

static std::vector<HANDLE> thread_handles;

static int num_characters_selected = 0;

static std::unordered_map <std::string, HWND> char_select_checkboxes;

static std::string DLL_path;

#define MAX_LOADSTRING 100

#define CHAR_POS_DX 78

#ifdef _DEBUG
#define DEBUG_CONSOLE
#endif

struct wowcl_t {
	HWND window_handle;
	std::string window_title;
	int valid;
	DWORD pid;
	DWORD tid; // tid of window thread
	int index;
	HANDLE library_handle;
	wowcl_t() {};
	wowcl_t(HWND hWnd, std::string title, int client_index)
		: window_handle(hWnd), window_title(title), valid(1), index(client_index) {
		tid = GetWindowThreadProcessId(hWnd, &pid);
		char *endptr;
		//ShowWindow(main_window_hWnd, SW_HIDE);

		if (index + 1 < 10) {
			char c = std::to_string(index + 1)[0]; // this is pretty risque... :D
			RegisterHotKey(main_window_hWnd, index, MOD_ALT, c);
		}
		else {
			RegisterHotKey(main_window_hWnd, index, MOD_ALT, '0');
		}
	}

	~wowcl_t() {
		UnregisterHotKey(main_window_hWnd, index);
	}

};

static std::vector<wowcl_t*> wow_handles;
static std::vector<HANDLE> cred_file_map_handles;

static void clear_wow_handles() {
	for (auto &c : wow_handles) {
		delete c;
	}
	wow_handles = std::vector<wowcl_t*>();

}

struct wowaccount_t {
	std::string login_name, password, char_name, class_name;
	int valid;
	wowaccount_t(std::string ln, std::string pw, std::string chn, std::string cln)
		: login_name(ln), password(pw), char_name(chn), class_name(cln), valid(1) {}

	wowaccount_t() : valid(0) {}

	std::string get_pipe_formatted_cred_string() const {
		return login_name + "," + password + "," + char_name;
	}
};

static std::unordered_map <DWORD, wowaccount_t> creds;


static void error_box(const std::string &msg) {
	MessageBox(NULL, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
}

inline void tokenize_string(const std::string& str, const std::string& delim, std::vector<std::string>& tokens) {
	size_t start, end = 0;
	while (end < str.size()) {
		start = end;
		while (start < str.size() && (delim.find(str[start]) != std::string::npos)) {
			start++;  // skip initial whitespace
		}
		end = start;
		while (end < str.size() && (delim.find(str[end]) == std::string::npos)) {
			end++; // skip to end of word
		}
		if (end - start != 0) {  // just ignore zero-length strings.
			tokens.push_back(std::string(str, start, end - start));
		}
	}
}



static int find_stuff_between(const std::string &in_str, char c, std::string &out_str) {
	unsigned d_begin = in_str.find(c);
	unsigned d_end = in_str.find_last_of(c);

	if (d_begin == std::string::npos || d_end == std::string::npos || d_begin == d_end) {
		error_box("Invalid config variable (values needs to be enclosed in quotes. E.g. WOWPATH=\"<your_path>\"). Exiting.");
		return 0;
	}

	out_str = in_str.substr(d_begin + 1, d_end - d_begin - 1);

	return 1;

}



struct potti_config {
	std::string client_exe_dir;
	std::vector<wowaccount_t> accounts;

	int read_from_file(const char* filename) {
		CHAR buffer[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, buffer, MAX_PATH);
		printf("%s\n", buffer);
		std::ifstream conf_file(filename);
		if (!conf_file.is_open()) {
			error_box("Couldn't find config file (potti.conf) from the working directory.");
			return FALSE;
		}

		std::vector <std::string> lines;

		std::string line;
		while (std::getline(conf_file, line)) {
			lines.push_back(line);
			//	printf("got line \"%s\"", line.c_str());
		}

		for (auto &l : lines) {
			std::vector<std::string> tokens;
			tokenize_string(l, "=", tokens);

			if (tokens.size() != 2) {
				error_box("Failure reading config file. Exiting!");
				return FALSE;
			}

			if (tokens[0] == "WOWDIR") {
				if (!find_stuff_between(tokens[1], '"', this->client_exe_dir)) {
					return 0;
				}

				printf("Client path: %s\n", this->client_exe_dir.c_str());
			}
			else if (tokens[0] == "ACCOUNTS") {
				std::string acc_str;
				if (!find_stuff_between(tokens[1], '"', acc_str)) {
					return 0;
				}

				std::vector <std::string> L1_tokens;

				tokenize_string(acc_str, ";", L1_tokens);

				for (auto &L1 : L1_tokens) {

					std::vector<std::string> L2_tokens;
					tokenize_string(L1, ",", L2_tokens);

					if (L2_tokens.size() != 4) {
						error_box("Error parsing ACCOUNTS conf var (expected exactly 4 members). Exiting.");
						return 0;
					}

					this->accounts.push_back(wowaccount_t(L2_tokens[0], L2_tokens[1], L2_tokens[2], make_lower(L2_tokens[3])));
					//printf("Added account %s\n", L2_tokens[0].c_str());
				}

			}
			else {
				error_box("Unknown config variable \"" + tokens[0] + "\". Exiting!");
				return 0;
			}

		}

		conf_file.close();
		return 1;

	}

};

static potti_config config_state;

static int clindex = 0;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {

	char title[MAX_LOADSTRING];
	char class_name[MAX_LOADSTRING];

	GetClassName(hWnd, class_name, sizeof(class_name));
	GetWindowText(hWnd, title, sizeof(title));


	if (strcmp(title, "World of Warcraft") == 0) {

		wow_handles.push_back(new wowcl_t(hWnd, title, clindex));
		++clindex;
	}

	return TRUE;
}


static void enum_windows() {
	clindex = 0;
	clear_wow_handles();
	EnumWindows(EnumWindowsProc, NULL);
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

static int create_account_assignments2() {
	static const size_t BUF_MAX = 256;

	enum_windows();

	if (wow_handles.size() < 1) { return 1; }

	size_t num_to_process = num_characters_selected;
	if (wow_handles.size() < num_to_process) {
		num_to_process = wow_handles.size();
	}

	// gather all checked clients to a vector
	std::vector <std::pair<wowaccount_t, wowcl_t*>> assigned_accs;
	int n = 0;
	for (const auto &account : config_state.accounts) {
		HWND hWnd = char_select_checkboxes[account.char_name];
		LRESULT state = SendMessage(hWnd, BM_GETCHECK, 0, 0);
		if (state == BST_CHECKED) {
			assigned_accs.push_back(std::make_pair(account, wow_handles[n]));
			++n;
		}
	}

	if (num_characters_selected != assigned_accs.size()) {
		printf("WARNING: create_account_assignments: num_characters_selected (%d) != assigned_accs.size() (%d)!!\n", num_characters_selected, assigned_accs.size());
	}

	for (unsigned int i = 0; i < num_to_process; ++i) {

		const wowcl_t *c = assigned_accs[i].second;
		const wowaccount_t &acc = assigned_accs[i].first;

		//std::string content = acc.login_name + "," + acc.password + "," + acc.char_name;

		creds[c->pid] = acc;

		printf("Assigned account %s to pid %d\n", acc.login_name.c_str(), c->pid);
	}

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
	if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
		printf("SetPrivilege for SE_DEBUG_NAME pheyled.\n");
		printf("ERROR: WINAPI SetPrivilege() failed! DLL injection will fail.\n");

		// close token handle
		CloseHandle(hToken);

		// indicate failure
		return 0;
	}

	return 1;
}

static int suspend_process_for(DWORD pid, DWORD ms) {
	DebugActiveProcess(pid);
	Sleep(ms);
	DebugActiveProcessStop(pid);

	return 1;
}

static int remote_thread_dll(wowcl_t *cl) {

	if (!obtain_debug_privileges()) {
		printf("Couldn't obtain debug privileges. Please re-run this program with administrator privileges.");
		return 0;
	}

	int res = 0;

	printf("Attempting to inject DLL %s to process %d...\n", DLL_path.c_str(), cl->pid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cl->pid);
	if (hProcess == NULL) { printf("OpenProcess failed: %d\n", GetLastError()); return 0; }

	LPVOID LoadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	if (LoadLibraryAddr == NULL) { printf("LoadLibraryA failed: %d\n", GetLastError()); return 0; }

	LPVOID LLParam = (LPVOID)VirtualAllocEx(hProcess, NULL, DLL_path.length(),
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (LLParam == NULL) { printf("VirtualAllocEx failed: %d\n", GetLastError()); return 0; }

	DWORD ret = WriteProcessMemory(hProcess, LLParam, DLL_path.c_str(), DLL_path.length(), NULL);
	if (ret == 0) { printf("WriteProcessMemory failed: %d\n", GetLastError()); return 0; }

	HANDLE rt = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryAddr,
		LLParam, NULL, NULL);

	if (rt == NULL) { printf("CreateRemoteThread failed: %d\n", GetLastError()); return 0; }

	DWORD library_handle = 0;

	WaitForSingleObject(rt, INFINITE);
	GetExitCodeThread(rt, &library_handle);
	if (library_handle == 0) {
		printf("Error: LoadLibraryAddr returned 0: %d\n", GetLastError());
		return 0;
	}

	cl->library_handle = (HANDLE)library_handle;

	CloseHandle(hProcess);

	printf("DLL injection to pid %d successful!\n", cl->pid);

	return 1;
}

struct patch_t {
	UINT32 patch_addr, patch_size;
	BYTE *original_opcodes, *patch_opcodes;
};

static int parse_pipe_response(const BYTE *resp, size_t resp_length, std::vector<patch_t> &patches) {

#define PIPE_PROTOCOL_MAGIC 0xAB30DD13

	UINT32 magic, num_patches, meta_size;
	memcpy(&magic, &resp[0], sizeof(UINT32));
	memcpy(&num_patches, &resp[1 * sizeof(UINT32)], sizeof(UINT32));
	memcpy(&meta_size, &resp[2 * sizeof(UINT32)], sizeof(UINT32));

	if (magic == PIPE_PROTOCOL_MAGIC) {
		printf("PIPE_PROTOCOL_MAGIC MATCHES! Client sent %d patches in a total of %d bytes. (meta_size = %d)\n", num_patches, resp_length, meta_size);
	}
	else {
		printf("ERROR: PIPE_PROTOCOL_MAGIC mismatch! (What!?)\n");
		return 0;
	}


	const BYTE *iter = &resp[3 * sizeof(UINT32)];

	for (UINT32 i = 0; i < num_patches; ++i) {
		patch_t p;

		memcpy(&p.patch_addr, &iter[0 * sizeof(UINT32)], sizeof(UINT32));
		memcpy(&p.patch_size, &iter[1 * sizeof(UINT32)], sizeof(UINT32));
		printf("patch #%d: iter = %d, patch addr: 0x%08X, size = %u\n", i + 1, (int)(iter - resp), p.patch_addr, p.patch_size);

		p.original_opcodes = new BYTE[p.patch_size];
		memcpy(p.original_opcodes, &iter[2 * sizeof(UINT32)], p.patch_size);

		p.patch_opcodes = new BYTE[p.patch_size];
		memcpy(p.patch_opcodes, &iter[2 * sizeof(UINT32) + p.patch_size], p.patch_size);

		iter += 2 * sizeof(UINT32) + 2 * p.patch_size;

		patches.push_back(p);
	}


	return 1;
}

static void dump_patch(const patch_t &p) {
	printf("patch_addr: 0x%X, p.patch_size = %d\n", p.patch_addr, p.patch_size);
}

static int suspend_and_apply_patches(DWORD pid, const std::vector<patch_t> patches) {
	printf("Suspending Wow.exe with PID %d for patching...\n", pid);
	DebugActiveProcess(pid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (!hProcess) {
		printf("OpenProcess(PROCESS_ALL_ACCESS, FALSE, %d) failed. GetLastError() = %d\n", pid, GetLastError());
		return 0;
	}

	for (const auto &p : patches) {

		dump_patch(p);

		SIZE_T num_bytes;
		BOOL r = WriteProcessMemory(hProcess, (LPVOID)p.patch_addr, p.patch_opcodes, p.patch_size, &num_bytes);

		if (r == 0) {
			printf("WriteProcessMemory failed: %d\n", GetLastError());
			return 0;
		}
	}

	CloseHandle(hProcess);

	DebugActiveProcessStop(pid);

	printf("Patching done. Resumed Wow.exe with PID %d!\n", pid);

	return 1;
}

static int get_credentials(DWORD pid, std::string *cred_str) {

	if (creds.size() < 1) {
		printf("(send_credentials: no credentials appear to have been assigned. Skipping!)\n");
		return 0;
	}

	if (creds.find(pid) == creds.end()) {
		printf("send_credentials: No credentials were assigned to pid %d. Exiting.\n", pid);
		return 0;
	}

	const wowaccount_t &account = creds[pid];

	*cred_str = account.get_pipe_formatted_cred_string();

	return 1;

}


static int do_pipe_operations(DWORD pid) {

#define PIPE_READ_BUF_SIZE 1024
#define PIPE_WRITE_BUF_SIZE 1024

	HANDLE hPipe;
	DWORD last_err;

	std::string pipe_name = "\\\\.\\pipe\\" + std::to_string(pid);

	printf("Trying to connect to pipe %s...\n", pipe_name.c_str());

	hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);

	const int num_retries = 4;
	int rnum = 1;
	while (hPipe == INVALID_HANDLE_VALUE) {

		if (rnum > num_retries) {
			printf("do_pipe_operations: maximum number of retries reached; client could have already been injected to (or crashed).\n");
			return 0;
		}

		last_err = GetLastError();
		if (last_err == ERROR_FILE_NOT_FOUND) {
			printf("(warning: pipe descriptor %s not (yet?) available (ERROR_FILE_NOT_FOUND), retrying after 500 ms (%d/%d)...)\n", pipe_name.c_str(), rnum, num_retries);
			Sleep(500);
			hPipe = CreateFile(pipe_name.c_str(), GENERIC_READ | FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);
		}
		else {
			printf("CreateFile failed with error %d, aborting!\n", GetLastError());
			return 0;
		}

		++rnum;

	}

	BYTE read_buf[PIPE_READ_BUF_SIZE];

	static const std::string PATCH_OK = "PATCH_OK";
	static const std::string PATCH_FAIL = "PATCH_FAIL";

	DWORD sc = 0;
	DWORD num_bytes = 0;

	printf("pipe connection established; waiting for client to send data...\n");

	sc = ReadFile(hPipe, read_buf, PIPE_READ_BUF_SIZE, &num_bytes, NULL); // wait for client to propagate patch addresses

	if (!sc || num_bytes == 0) {
		printf("pipe %s: ReadFile returned %d; last error: %d\n", pipe_name.c_str(), sc, GetLastError());
	}

	std::vector<patch_t> patches;

	std::string response_str;

	if (parse_pipe_response(read_buf, num_bytes, patches)) {
		response_str = PATCH_OK;
		suspend_and_apply_patches(pid, patches);

		for (auto &p : patches) {
			delete[] p.original_opcodes;
			delete[] p.patch_opcodes;
		}

		std::string cred_str;
		if (get_credentials(pid, &cred_str)) {
			response_str.append(";CREDENTIALS=" + cred_str);
		}

	}
	else {
		response_str = PATCH_FAIL;
		printf("parse_pipe_response() failed :(\n");
	}

	sc = WriteFile(hPipe, response_str.c_str(), response_str.length() + 1, &num_bytes, NULL);
	printf("Sent response %s to client %d. Closing pipe.\n", response_str.c_str(), pid);

	CloseHandle(hPipe);

	return 1;
}

static DWORD WINAPI inject_threadfunc(LPVOID param) {

	// lol. each thread needs to individually obtain debug privileges..
	wowcl_t *cl = (wowcl_t*)param;

	//DWORD tid = GetWindowThreadProcessId(cl->window_handle, &cl->tpid);

	if (cl->tid == 0 || cl->pid == 0) {
		printf("error: GetWindowThreadProcessId() returned 0. GetLastError() = %d. Exiting.\n", GetLastError());
		return 0;
	}

	if (!remote_thread_dll(cl)) {
		printf("remote_thread_dll() failed. Exiting.\n");
		return 0;
	}

	//if (!do_pipe_operations(cl->pid)) {
	//	printf("do_pipe_operations() failed. Exiting.\n");
	//	return 0;
	//}

	return 1;
}


static HANDLE inject_dll(wowcl_t *cl) {
	HANDLE hThread = CreateThread(NULL, 0, inject_threadfunc, (LPVOID)cl, 0, NULL);
	return hThread;
}

static int inject_to_all() {

	EnableWindow(button_inject_hWnd, FALSE);
	thread_handles = std::vector<HANDLE>();

	enum_windows();

	for (auto c : wow_handles) {
		thread_handles.push_back(inject_dll(c));
	}

	if (thread_handles.size() > 0) {
		if (WaitForMultipleObjects(thread_handles.size(), &thread_handles[0], TRUE, INFINITE) == WAIT_FAILED) {
			printf("WaitForMultipleObjects failed: error %d\n", GetLastError());
		}
	}

	EnableWindow(button_inject_hWnd, TRUE);

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


HWND create_button(const std::string &text, int pos_x, int pos_y, int width, int height, HWND parent_hWnd) {

	HWND btn_hWnd = CreateWindow(
		"BUTTON",  // Predefined class; Unicode assumed 
		text.c_str(),      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		pos_x,         // x position 
		pos_y,         // y position 
		width,        // Button width
		height,        // Button height
		parent_hWnd,     // Parent window
		NULL,       // No menu.
		(HINSTANCE)GetWindowLongPtr(main_window_hWnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	if (!btn_hWnd) {
		error_box("Rekt. create_button() whaled: " + std::to_string(GetLastError()));
		return NULL;
	}

	SendMessage(btn_hWnd,
		WM_SETFONT,
		(WPARAM)GetStockObject(DEFAULT_GUI_FONT),
		MAKELPARAM(FALSE, 0));

	return btn_hWnd;
}

static HWND create_checkbox_tooltip(HWND hwndParent, const std::string &text) {
	// Create a tooltip.
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwndParent, NULL, hInst, NULL);

	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	// Set up "tool" information. In this case, the "tool" is the entire parent window.

	TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = hwndParent;
	ti.hinst = hInst;

	char *textcpy = strdup(text.c_str());
	ti.lpszText = textcpy;

	GetClientRect(hwndParent, &ti.rect);

	// Associate the tooltip with the "tool" window.
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);

	return hwndTT;
}


static HWND create_checkbox(const std::string &char_name, const std::string &acc_name, int pos_x, int pos_y, HWND parent_hWnd) {
	HWND hWnd = CreateWindow("BUTTON", char_name.c_str(), WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
		pos_x, pos_y, CHAR_POS_DX - 8, 20, parent_hWnd, NULL, (HINSTANCE)GetWindowLongPtr(main_window_hWnd, GWLP_HINSTANCE), NULL);
	if (!hWnd) {
		error_box("Rekt. create_checkbox() whaled: " + std::to_string(GetLastError()));
		return NULL;
	}

	SendMessage(hWnd,
		WM_SETFONT,
		(WPARAM)GetStockObject(DEFAULT_GUI_FONT),
		MAKELPARAM(FALSE, 0));

	HWND tt = create_checkbox_tooltip(hWnd, acc_name);

	return hWnd;
}

static int launch_clients() {

	char textbuf[512];
	LRESULT result = SendMessage(pathedit_hWnd, WM_GETTEXT, sizeof(textbuf), LPARAM(textbuf));
	config_state.client_exe_dir = std::string(textbuf);

	long value;
	char *endptr;

	GetWindowText(edit_num_clients_hWnd, textbuf, 512);

	value = strtol(textbuf, &endptr, 10);	// the editbox has ES_NUMBER, so it's quite hard to come up with an invalid value :P

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	std::string cpath = config_state.client_exe_dir + "\\Wow.exe";

	for (int i = 0; i < value; ++i) {
		if (CreateProcess(cpath.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo) == 0) {
			printf("CreateProcess for Wow.exe (path: %s) failed: %d\n", cpath.c_str(), GetLastError());
		}
		Sleep(100); // without this launching multiple clients at once sometimes causes a hangup in the 3D acceleration stack startup
	}

	enum_windows();

	return 1;

}

static int set_affinities() {

	enum_windows();

	unsigned int n = 0;

	const unsigned int ncores = std::thread::hardware_concurrency();

	printf("set_affinities: detected %u CPU cores.\n", ncores);

	for (auto c : wow_handles) {

		HANDLE proc_handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, c->pid);

		DWORD aff_mask = (0x3 << (n * 2));

		SetProcessAffinityMask(proc_handle, aff_mask);
		SetPriorityClass(proc_handle, ABOVE_NORMAL_PRIORITY_CLASS);
		n = (n < (ncores / 2 - 1)) ? (n + 1) : 0;

		printf("proc_handle = %X, PID = %d, aff_mask = %X\n", (DWORD)proc_handle, c->pid, aff_mask);

		CloseHandle(proc_handle);

	}

	return 1;
}

static int refresh_client_list() {
	enum_windows();

	std::string num = std::to_string(wow_handles.size());

	SetWindowText(numclients_static_hWnd, num.c_str());
	ShowWindow(numclients_static_hWnd, SW_HIDE);
	ShowWindow(numclients_static_hWnd, SW_SHOW);
	return 1;
}


static int uninject_all() {

	// this doesn't really work, BTW :D

	printf("Uninjecting all clients!\n");

	for (auto &c : wow_handles) {
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, c->pid);
		if (hProcess == NULL) {
			printf("OpenProcess failed\n");
			return 0;
		}

		HMODULE hModule = GetModuleHandle("kernel32.dll");
		HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "FreeLibrary"),
			(LPVOID)c->library_handle, 0, NULL);

		if (hRemoteThread == NULL) {
			printf("CreateRemoteThread fail\n");
			return 0;
		}
		DWORD r;
		WaitForSingleObject(hRemoteThread, INFINITE);
		GetExitCodeThread(hRemoteThread, &r);

		if (r == TRUE) {
			printf("freelibrary returned true for pid %d!\n", c->pid);
		}
		else {
			printf("freelibrary failed!\n");
		}

		CloseHandle(hProcess);

	}

	return 1;
}


static LRESULT handle_checkbox_action(HWND hWnd) {

	LRESULT state = SendMessage(hWnd, BM_GETCHECK, 0, 0);
	num_characters_selected = state == BST_UNCHECKED ? num_characters_selected + 1 : num_characters_selected - 1;

	std::string numstr = std::to_string(num_characters_selected);

	SendMessage(edit_num_clients_hWnd, WM_SETTEXT, NULL, (LPARAM)numstr.c_str());
	SendMessage(updown_hWnd, UDM_SETRANGE32, num_characters_selected, 25);


	//	printf("num_checked = %d\n", num_checked);

	return SendMessage(hWnd, BM_SETCHECK, state == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED, 0);
}

static HWND create_static_text(const std::string &text, int x, int y, HWND parent) {
	HWND rw = CreateWindowEx(0,
		"STATIC",
		text.c_str(),
		WS_CHILD | WS_VISIBLE,
		x, y, text.length() * 5 + 10, 18,
		parent,
		NULL,
		GetModuleHandle(NULL),
		NULL);

	SendMessage(rw,
		WM_SETFONT,
		(WPARAM)GetStockObject(DEFAULT_GUI_FONT),
		MAKELPARAM(FALSE, 0));

	return rw;

}

static HWND create_textedit(const std::string &default_text, int x, int y, int width, int RID, HWND parent) {
	HWND rw = CreateWindowEx(
		WS_EX_CLIENTEDGE, "EDIT",   // predefined class 
		NULL,         // no window title 
		WS_CHILD | WS_VISIBLE | ES_LEFT | WS_CLIPSIBLINGS,
		x, y, width, 18,
		parent,         // parent window 
		(HMENU)RID,   // edit control ID 
		GetModuleHandle(NULL),
		NULL);        // pointer not needed 


	SendMessage(rw, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	SendMessage(rw, WM_SETTEXT, 0, (LPARAM)default_text.c_str());

	return rw;
}

INT_PTR CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	int lo, hi;
	lo = LOWORD(wParam);
	hi = HIWORD(wParam);

	switch (message) {

	case WM_COMMAND:

		if (hi == BN_CLICKED) {

			if ((HWND)lParam == button_launch_hWnd) {

				launch_clients();
				return TRUE;
			}
			else if ((HWND)lParam == button_affinity_hWnd) {
				set_affinities();
				return TRUE;
			}
			else if ((HWND)lParam == button_inject_hWnd) {
				inject_to_all();
				return TRUE;
			}
			else if ((HWND)lParam == button_assign_hWnd) {
				create_account_assignments2();
				return TRUE;
			}
			else if ((HWND)lParam == button_refresh_hWnd) {
				refresh_client_list();
				return TRUE;
			}
			else {
				// then it's most likely from one of the checkboxes
				handle_checkbox_action((HWND)lParam);
				return TRUE;
			}
		}

		switch (lo) {
			//case ID_CHECKBOXBLAST:
			//	toggle_blast();
			//	return TRUE;
			//	break;
		case ID_UPDOWN_NUMCLIENTS:
			return TRUE;
			break;

		default:
			break;
		}

		break;




	case WM_CREATE: {
		edit_num_clients_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
			"EDIT",
			"",
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
			| ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NUMBER,
			180,
			42,
			60,
			20,
			hWnd,
			(HMENU)ID_EDIT_NUMCLIENTS,
			GetModuleHandle(NULL),
			NULL);

		SendMessage(edit_num_clients_hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
		SendMessage(edit_num_clients_hWnd, WM_SETTEXT, NULL, (LPARAM)"0");

		updown_hWnd = CreateWindowEx(WS_EX_LEFT | WS_EX_LTRREADING,
			UPDOWN_CLASS,
			NULL,
			WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS
			| UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
			0, 0,
			0, 0,         // Set to zero to automatically size to fit the buddy window.
			hWnd,
			NULL,
			GetModuleHandle(NULL),
			NULL);

		SendMessage(updown_hWnd, UDM_SETRANGE32, 0, 25);

		HWND num_clients_static = create_static_text("Number of clients to launch:", 25, 45, hWnd);

		HWND wowpath_static = create_static_text("Wow directory:", 25, 15, hWnd);
		pathedit_hWnd = create_textedit(config_state.client_exe_dir, 110, 15, 300, ID_EDIT_WOWDIR, hWnd);

		button_launch_hWnd = create_button("Launch!", 250, 37, 100, 30, hWnd);
		button_affinity_hWnd = create_button("Set CPU affinities", 30, 80, 100, 30, hWnd);
		button_assign_hWnd = create_button("Assign credentials", 30, 120, 100, 30, hWnd);
		button_inject_hWnd = create_button("Inject DLL", 150, 80, 100, 30, hWnd);

		button_refresh_hWnd = create_button("Refresh + set hotkeys", 500, 65, 115, 30, hWnd);
		HWND ac_hWnd = create_static_text("Active clients:", 505, 45, hWnd);
		numclients_static_hWnd = create_static_text("0", 575, 45, hWnd);

		break;
	}
	case WM_HOTKEY: {
		// ASSUMING LESS THAN 10 CLIENTS :D
		char c[2] = { HIWORD(lParam), '\0' };
		char *endptr;
		int index = strtol(c, &endptr, 10);
		SetForegroundWindow(wow_handles[index == 0 ? 9 : index - 1]->window_handle);
		break;
	}

	case WM_DESTROY:
		for (auto &h : cred_file_map_handles) {
			CloseHandle(h);
		}
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	}

	return FALSE;
}



static int setup_char_checkboxes(const potti_config &c) {
	int char_posx_offset = 25;
	int char_posy_offset = 225;
	int dx = CHAR_POS_DX;
	int dy = 25;

	std::unordered_map<std::string, int> class_indices = {
		{ "druid", 0 },
		{ "hunter", 1 },
		{ "mage", 2 },
		{ "paladin", 3 },
		{ "priest", 4 },
		{ "rogue", 5 },
		{ "shaman", 6 },
		{ "warlock", 7 },
		{ "warrior", 8 },
		{ "dk", 9 },
	};


	std::unordered_map<std::string, int> class_num_map = {
		{ "druid", 0 },
		{ "hunter", 0 },
		{ "mage", 0 },
		{ "paladin", 0 },
		{ "priest", 0 },
		{ "rogue", 0 },
		{ "shaman", 0 },
		{ "warlock", 0 },
		{ "warrior", 0 },
		{ "dk", 0 },
	};

	for (auto &k : c.accounts) {
		int *num = &class_num_map[k.class_name];

		int pos_x = char_posx_offset + class_indices[k.class_name] * dx;
		int pos_y = char_posy_offset + (*num) * dy;

		char_select_checkboxes[k.char_name] = create_checkbox(k.char_name, k.login_name, pos_x, pos_y, main_window_hWnd);

		++(*num);
		//printf("added %s:%s (num = %d, pos_x = %d, pos_y = %d)\n", k.char_name.c_str(), k.class_name.c_str(), *num, pos_x, pos_y);
	}

	for (auto &cl : class_indices) {
		int pos_x = char_posx_offset + cl.second * dx - 4;
		int pos_y = char_posy_offset - 3;
		int width = CHAR_POS_DX - 2;
		int height = class_num_map[cl.first] * dy;

		HWND static_frame = CreateWindow("STATIC", (cl.first + "_staticframe").c_str(), WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
			pos_x, pos_y, width, height, main_window_hWnd, NULL, (HINSTANCE)GetWindowLongPtr(main_window_hWnd, GWLP_HINSTANCE), NULL);

		std::string image_filename = "..\\images\\" + cl.first + ".bmp";
		HBITMAP class_image = (HBITMAP)LoadImage(GetModuleHandle(NULL), image_filename.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_SHARED);

		//printf("image_filename = %s, class_image: %X\n", image_filename.c_str(), (int)class_image);

		if (class_image != NULL) {
			HWND class_icon_hWnd = CreateWindow("STATIC", (cl.first + "_staticicon").c_str(), WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
				pos_x + 16, pos_y - 42, 32, 32, main_window_hWnd, NULL, (HINSTANCE)GetWindowLongPtr(main_window_hWnd, GWLP_HINSTANCE), NULL);

			SendMessage(class_icon_hWnd, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)class_image);

		}
		else {
			printf("loading the bitmap file %s failed. GetLastError(): %d\n", image_filename.c_str(), GetLastError());
		}

	}

	return 1;

}


BOOL InitInstance(HINSTANCE hInstance, int nShowCmd) {

	if (!config_state.read_from_file("..\\potti.conf")) return FALSE;

	hInst = hInstance; // Store instance handle in our global variable

	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbClsExtra = NULL;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = NULL;
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wClass.hIcon = NULL;
	wClass.hIconSm = NULL;
	wClass.hInstance = hInst;
	wClass.lpfnWndProc = (WNDPROC)WndProc;
	wClass.lpszClassName = "lole launcher";
	wClass.lpszMenuName = NULL;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wClass)) {
		error_box("RegisterClassEx() failed! Errcode: " + std::to_string(GetLastError()));
		return FALSE;
	}


	HWND hWnd = CreateWindowEx(NULL,
		wClass.lpszClassName,
		"lole launcher",
		WS_OVERLAPPEDWINDOW,
		200,
		200,
		850,
		480,
		NULL,
		NULL,
		hInst,
		NULL);

	if (!hWnd) {
		error_box("CreateWindowEx() failed! Errcode: " + std::to_string(GetLastError()));
		return FALSE;
	}

	main_window_hWnd = hWnd;


	setup_char_checkboxes(config_state);

	ShowWindow(hWnd, nShowCmd);


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

#ifdef DEBUG_CONSOLE
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
#endif

	hInst = hInstance;

	char DirPath[MAX_PATH];
	char FullPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, DirPath);

	sprintf_s(FullPath, MAX_PATH, "%s\\wowibottihookdll_rust.dll", DirPath);

	DLL_path = std::string(FullPath);

	if (!obtain_debug_privileges()) {
		error_box("Couldn't obtain debug privileges. Please re-run this program with administrator privileges.");
		return 0;
	}

	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

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

