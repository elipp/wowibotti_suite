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

#ifdef _DEBUG
#define DEBUG_CONSOLE
#endif

static HINSTANCE inj_hModule;          // HANDLE for injected module
HANDLE glhProcess;
HWND wow_hWnd;

int afkjump_keyup_queued = 0;
static int enter_world = 0;

struct cred_t {
	std::string account, password, char_name;
	std::string login_script;
	cred_t(const std::string &acc, const std::string &pw, const std::string &ch) : account(acc), password(pw), char_name(ch) {
		char buf[512];

		sprintf(buf, "if (AccountLoginUI and AccountLoginUI:IsShown()) then AccountLoginUI:Hide(); DefaultServerLogin('%s', '%s');\
					 elseif (CharacterSelectUI and CharacterSelectUI:IsShown()) \
					then for i=0,GetNumCharacters() do local name = GetCharacterInfo(i);\
					if (name and name == '%s') then CharacterSelect_SelectCharacter(i); end end EnterWorld(); end", account.c_str(), password.c_str(), char_name.c_str());
		
		login_script = std::string(buf);
	}

	cred_t() {
		account = "";
		password = "";
		char_name = "";
		login_script = "do end";
	}
};

static cred_t credentials;

static BYTE original_opcodes[8];

void __stdcall print_errcode(int code) {
	printf("Spell cast failed, errcode %d\n", code);
}

static int patch_LUA_prot() {
	printf("Patching LUA protection function...");
	static LPVOID lua_prot_addr = (LPVOID)0x49DBA0;
	static BYTE patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
	WriteProcessMemory(glhProcess, lua_prot_addr, patch, sizeof(patch), NULL);
	printf("OK!\n");
	return 1;
}


static void update_hwevent_tick() {
	typedef int tick_count_t(void);
	int ticks = ((tick_count_t*)GetOSTickCount)();

	*(int*)(TicksSinceLastHWEvent) = ticks; 
	// this should make us immune to AFK ^^
}


static void attempt_login() {
	//printf("attempting login with account %s\n", credentials.account.c_str());
	DoString(credentials.login_script.c_str());
}

static void update_debug_positions() {
	ObjectManager OM;
	auto p = OM.get_local_object();
	vec3 ppos = p.get_pos();
	char buf[128];

	sprintf(buf, "(%.1f, %.1f, %.1f)", ppos.x, ppos.y, ppos.z);
	DoString("SetCVar(\"movieSubtitle\", \"%s\", \"player_pos\")", buf);

	if (get_target_GUID() != 0) {
		auto t = OM.get_object_by_GUID(get_target_GUID());
		vec3 tpos = t.get_pos();
		sprintf(buf, "(%.1f, %.1f, %.1f)", tpos.x, tpos.y, tpos.z);

		DoString("SetCVar(\"movieSubtitle\", \"%s\", \"target_pos\")", buf);
	}
	else {
		DoString("SetCVar(\"movieSubtitle\", \"-\", \"target_pos\")");
	}
}

static void __stdcall EndScene_hook() {

	static int every_third_frame = 0;
	static int every_thirty_frames = 0;
	
	if (every_third_frame == 0) {
		refollow_if_needed();
		ctm_act();
	}

	if (every_thirty_frames == 0) {
		update_debug_positions();
		update_hwevent_tick();
		attempt_login(); 
	}

	every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;
	every_thirty_frames = every_thirty_frames > 29 ? 0 : every_thirty_frames + 1;

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

static void __stdcall CTM_finished_hookfunc() {
	ctm_commit();
}


static int hook_all() {
	
	install_hook("EndScene", EndScene_hook);
	install_hook("DelIgnore", DelIgnore_hub);
	//install_hook("ClosePetStables", melee_behind_target);
	//install_hook("CTM_aux", broadcast_CTM);
	install_hook("CTM_main", broadcast_CTM);
	install_hook("CTM_update", CTM_finished_hookfunc);
	
	return 1;
}

static int unhook_all() {
	
	uninstall_hook("EndScene");
	uninstall_hook("DelIgnore");
	uninstall_hook("ClosePetStables");
	uninstall_hook("CTM_main");
	uninstall_hook("CTM_update");
	
	return 1;
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
	if (err != NO_ERROR) {
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

		credentials = cred_t(cred_tokens[0], cred_tokens[1], cred_tokens[2]);

	}
	else {
		printf("warning: OpenFileMapping returned error %d for account credential handling! fd name: %s\n", err, cred_fd.c_str());
	}

	CloseHandle(login_map);

	return 1;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	DWORD processID = GetCurrentProcessId();
	//std::thread hook_thread;

	glhProcess = hProcess;


	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {

		inj_hModule = hModule;
#ifdef DEBUG_CONSOLE
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
#endif
		DebugActiveProcess(processID); // this is like a TINKERING_BEGIN

			handle_login_creds();
			patch_LUA_prot();
			hook_all();

		DebugActiveProcessStop(processID); // TINKERING_END

		DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D


		break;
	}
	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");

//		PostThreadMessage(GetThreadId(hook_thread), WM_DESTROY, 0, 0);
		//WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

