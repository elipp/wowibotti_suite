// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <D3D9.h>
#include <string>
#include <cstdio>

#define MYMENU_EXIT         (WM_APP + 101)
#define MYMENU_DUMP			(WM_APP + 102) 
#define MYMENU_HOOK			(WM_APP + 103)
#define MYMENU_UNHOOK		(WM_APP + 104)
#define MYMENU_CTM			(WM_APP + 105)
#define MYMENU_CTM_CONSTANT (WM_APP + 106)

HINSTANCE  inj_hModule;          //Injected Modules Handle
HWND       prnt_hWnd;            //Parent Window Handle
								 //WndProc for the new window

#define MKEY_G 0x47

LRESULT CALLBACK DLLWindowProc(HWND, UINT, WPARAM, LPARAM);

static FILE *CTM_debug;

static HANDLE glhProcess;

static const unsigned long DelIgnore_aux = 0x540D10;
static const unsigned long ClosePetStables = 0x4FACA0;

static unsigned int * const PLAYER_TARGET_ADDR = (unsigned int* const)0xC6E960;
static unsigned int * const PLAYER_TARGET_ADDR2 = (unsigned int* const)0xC6E968;
static unsigned int * const PLAYER_TARGET_ADDR3 = (unsigned int* const)0xC6E970; // this usually contains a copy of the GUID, also is kept even after target is changed

static BYTE original_opcodes[8];

static int BLAST_ENABLED = 0;
static int CONSTANT_CTM_TARGET = 0;

static HRESULT(*EndScene)(void);
static const int(*LUA_DoString)(const char*, const char*, const char*) = (const int(*)(const char*, const char*, const char*)) 0x706C80;

static void __stdcall walk_to_target();

enum {
	CTM_MOVE = 0x4,
	CTM_TALK_NPC = 0x5,
	CTM_LOOT = 0x6,
	CTM_MOVE_AND_ATTACK = 0xA
};

static int STOP_SIGNALED = 0;

typedef unsigned long long GUID_t;

typedef unsigned int uint; // looks kinda messy with all the "unsigned int"s

void __stdcall print_errcode(int code) {
	printf("Spell cast failed, errcode %d\n", code);
}


static int patch_moveforwardstart(HANDLE hProcess) {
	LPVOID jz_addr = (LPVOID)0x00534604;
	BYTE jnz = 0x75;
	WriteProcessMemory(hProcess, jz_addr, &jnz, 1, NULL);
	return 1;
}

static int patch_LUA_prot(HANDLE hProcess) {

	static LPVOID lua_prot_addr = (LPVOID)0x49DBA0;

	static BYTE patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };

	WriteProcessMemory(hProcess, lua_prot_addr, patch, sizeof(patch), NULL);
	return 1;
}


static SIZE_T readAddr(unsigned int mem, void *to, size_t bytes) {
	SIZE_T read;
	ReadProcessMemory(glhProcess, (LPVOID)mem, to, bytes, &read);
	return read;
}
static SIZE_T writeAddr(unsigned int mem, void *data, size_t bytes) {
	SIZE_T read;
	WriteProcessMemory(glhProcess, (LPVOID)mem, data, bytes, &read);
	return read;
}

const std::string type_names[] = {
	"0 : OBJECT",
	"1 : ITEM",
	"2 : CONTAINER",
	"3 : UNIT",
	"4 : PLAYER",
	"5 : GAMEOBJECT",
	"6 : DYNAMICOBJECT",
	"7 : CORPSE",
	"8 : AREATRIGGER",
	"9 : SCENEOBJECT"
};


class WowObject {
private:
	// these are offsets from the base addr of the object
	
	const unsigned int
		GUID = 0x30,
		Next = 0x3C,
		Type = 0x14,
		X = 0xBF0,
		Y = X + 4,
		Z = X + 8,
		R = X + 12,
		Field = 0x120,
		Name = 0xDB8,
		NPCcurrentHealth = 0x11E8,
		NPCcurrentMana = 0x11EC,
		NPCcurrentRage = 0x11F0,
		NPCcurrentEnergy = 0x11F4,
		NPCcurrentFocus = 0x11F8,

		NPCmaxHealth = 0x1200,
		NPCmaxMana = 0x1204,
		NPCmaxRage = 0x1208,
		NPCmaxEnergy = 0x120C,
		NPCmaxFocus = 0x1210,

		UnitTargetGUID = 0x2680;

		unsigned int base;

public:
	bool valid() const {
		return ((this->get_base() != 0) && ((this->get_base() & 1) == 0));
	}

	WowObject getNextObject() const {
		unsigned int next_base;
		readAddr(base + this->Next, &next_base, sizeof(next_base));
		return WowObject(next_base);
	}

	float get_pos_x() const {
		float x;
		readAddr(base + X, &x, sizeof(x));
		return x;
	}
	float get_pos_y() const {
		float y;
		readAddr(base + Y, &y, sizeof(y));
return y;
	}
	float get_pos_z() const {
		float z;
		readAddr(base + Z, &z, sizeof(z));
		return z;
	}
	float get_rot() const {
		float r;
		readAddr(base + R, &r, sizeof(r));
		return r;
	}
	GUID_t get_GUID() {
		GUID_t g;
		readAddr(base + GUID, &g, sizeof(g));
		return g;
	}

	int get_type() {
		int t;
		readAddr(base + Type, &t, sizeof(t));
		return t;
	}

	int NPC_getCurHealth() {
		int h;
		readAddr(base + NPCcurrentHealth, &h, sizeof(h));
		return h;
	}

	int NPC_getMaxHealth() {
		int h;
		readAddr(base + NPCmaxHealth, &h, sizeof(h));
		return h;
	}

	int NPC_getCurMana() {
		int h;
		readAddr(base + NPCcurrentMana, &h, sizeof(h));
		return h;
	}

	int NPC_getMaxMana() {
		int h;
		readAddr(base + NPCmaxMana, &h, sizeof(h));
		return h;
	}

	int NPC_getCurRage() {
		int h;
		readAddr(base + NPCcurrentRage, &h, sizeof(h));
		return h;
	}

	int NPC_getRageMax() {
		int h;
		readAddr(base + NPCmaxRage, &h, sizeof(h));
		return h;
	}
	int NPC_getCurEnergy() {
		int h;
		readAddr(base + NPCcurrentHealth, &h, sizeof(h));
		return h;
	}

	int NPC_getMaxEnergy() {
		int h;
		readAddr(base + NPCmaxEnergy, &h, sizeof(h));
		return h;
	}

	int NPC_getCurFocus() {
		int h;
		readAddr(base + NPCcurrentFocus, &h, sizeof(h));
		return h;
	}

	int NPC_getMaxFocus() {
		int h;
		readAddr(base + NPCmaxFocus, &h, sizeof(h));
		return h;
	}

	char *NPC_get_name() {
		unsigned int name;
		readAddr(base + Name, &name, sizeof(name));
		return *(char**)(name + 0x40);
	}

	char *unit_get_name(FILE *fp) {

#define DEREF(x) *(uint*)(x)

		// 0xD29BB0 is some kind of static / global string-related thing, resides in the .data segment. (look at Wow.exe:0x6D9850)
		const uint ecx = 0xD29BB0;
		
		uint ECX24 = ecx + 0x24;
		readAddr(ECX24, &ECX24, sizeof(ECX24));

		uint ECX1C = ecx + 0x1C;
		readAddr(ECX1C, &ECX1C, sizeof(ECX1C));

		uint ESI_GUID = (uint)get_GUID();
		uint AND = ESI_GUID & ECX24;
		
		uint eax = AND * 0x2 + AND;
	
		eax = eax * 0x4 + ECX1C;
		eax = eax + 0x4;
		eax = eax + 0x4;

		eax = DEREF(eax);

		//fprintf(fp, "ECX + 0x24 = %X, ECX + 0x1C = %X, AND = %X, eax = %X, [eax] = %X\n", ECX24, ECX1C, AND, eax, DEREF(eax));
	// absolutely no fucking idea how this works :DD

		while (DEREF(eax) != ESI_GUID) {
			//fprintf(fp, "eax = %X, GUID ([eax]) = %X\n", eax, DEREF(eax));
			uint edx = AND * 0x2 + AND;
			edx = edx * 0x4 + ECX1C;
			edx = DEREF(edx) + eax;
			eax = DEREF(edx + 0x4);

			// THERES STILL THAT WEIRD EAX+0x18 thing to figure out. the GUID is at [eax], but is also at [eax + 0x18]? :D
		}

		char *ret = (char*)(eax + 0x20);
		fprintf(fp, "ret = %p\n", ret);
		
		return ret; 
	}

	GUID_t unit_get_target_GUID() {
		GUID_t target_GUID;
		readAddr(base + UnitTargetGUID, &target_GUID, sizeof(target_GUID));
		return target_GUID;
	}

	WowObject(unsigned int addr) : base(addr) {};
	WowObject() {};

	unsigned int get_base() const { return base; }

	const WowObject& operator=(const WowObject &o) {
		this->base = o.base;
		return *this;
	}

};


class ObjectManager {

private:
	const unsigned int clientConnection_addr_static = 0x00D43318,
		clientConnection = 0,
		objectManagerOffset = 0x2218, // 0x46E134 has this [edx + 0x2218] thing in it
		localGUIDOffset = 0xC0,                                             // offset from the object manager to the local guid

		firstObjectOffset = 0xAC,                                          // offset from the object manager to the first object
		nextObjectOffset = 0x3C;                                       // offset from one object to the next
	

	void LoadAddresses() {
		unsigned int currentManager_pre = 0;
		readAddr(clientConnection_addr_static, &currentManager_pre, sizeof(currentManager_pre));
		readAddr(currentManager_pre + objectManagerOffset, &objectManagerBase, sizeof(objectManagerBase));
		readAddr(objectManagerBase + localGUIDOffset, &localGUID, sizeof(localGUID));
	}

public:
	unsigned int objectManagerBase;                       // the address of the object manager
	GUID_t localGUID;                                   // the local guid.

	WowObject getFirstObject() {
		unsigned int baseAddr;
		readAddr(objectManagerBase + firstObjectOffset, &baseAddr, sizeof(baseAddr));
		return WowObject(baseAddr);
	}

	ObjectManager() {
		LoadAddresses();
	}

	WowObject get_object_by_GUID(GUID_t GUID) {
		WowObject next = getFirstObject();

		while (next.valid()) {
			if (next.get_GUID() == GUID) {
				return next;
			}
			next = next.getNextObject();
		}
		return next;

	}

	GUID_t get_localGUID() const { return localGUID; }


};

static void click_to_move(float x, float y, float z, uint action, GUID_t interact_GUID) {
	static const uint 
		CTM_X = 0xD68A18,
		CTM_Y = 0xD68A1C,
		CTM_Z = 0xD68A20,
		CTM_push = 0xD689BC,
		CTM_mystery = 0xD689AE,
		CTM_GUID = 0xD689C0; // this is for interaction

	// seems like addresses D689A0 D689A4 D689A8 are floats, maybe some angles?
	// A0 = the angle where the character should be walking?
	// D689A8 and D689AC are weird..
	// for walking, "mystery" is a constant float 0.5

	writeAddr(CTM_X, &x, sizeof(x));
	writeAddr(CTM_Y, &y, sizeof(y));
	writeAddr(CTM_Z, &z, sizeof(z));

	if (interact_GUID != 0) {
		writeAddr(CTM_GUID, &interact_GUID, sizeof(GUID));
	}

//	writeAddr(CTM_mystery, &mystery, 2);
	writeAddr(CTM_push, &action, sizeof(action));

}


int dump_wowobjects_to_log() {

	ObjectManager OM;

	if (!OM.objectManagerBase) {
		fprintf(stderr, "WowObject dumping failed: ObjectManager base addr seems to be NULL.\n(Is your character in the world?)\n");
		return 0;
	}

	FILE *fp = fopen("C:\\Users\\elias\\Desktop\\out.log", "a");

	if (fp) {
		WowObject next = OM.getFirstObject();
		GUID_t target_GUID;
		readAddr((unsigned int)PLAYER_TARGET_ADDR, &target_GUID, sizeof(target_GUID));
		fprintf(fp, "ObjectManager base address: %X, local GUID = 0x%llX, player target: %llX\n", OM.objectManagerBase, OM.get_localGUID(), target_GUID);

		while (next.valid()) {

			if (next.get_type() == 3 || next.get_type() == 4) { // 3 = NPC, 4 = Unit
				fprintf(fp, "object GUID: 0x%llX, base addr = %X, type: %s, coords = (%f, %f, %f), rot: %f\n",
					next.get_GUID(), next.get_base(), type_names[next.get_type()].c_str(), next.get_pos_x(), next.get_pos_y(), next.get_pos_z(), next.get_rot());

				// print bytes
				if (next.get_type() == 3) {
					fprintf(fp, "name: %s, health: %d/%d\n\n", next.NPC_get_name(), next.NPC_getCurHealth(), next.NPC_getMaxHealth());
				}
				else if (next.get_type() == 4) {
					fprintf(fp, "name: %s, target guid: %llX\n\n", next.unit_get_name(fp), next.unit_get_target_GUID());
				}
			}

			next = next.getNextObject();
		}
		fclose(fp);

	}
	return 1;
}

static void __stdcall walk_to_target() {
	
	GUID_t GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!GUID) return;
	
	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(GUID);

	click_to_move(t.get_pos_x(), t.get_pos_y(), t.get_pos_z(), CTM_MOVE, 0);
	
}


//Register our windows Class
BOOL RegisterDLLWindowClass(char szClassName[])
{
	WNDCLASSEX wc;
	wc.hInstance = inj_hModule;
	wc.lpszClassName = (LPCSTR)szClassName;
	wc.lpfnWndProc = DLLWindowProc;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	if (!RegisterClassEx(&wc))
		return 0;
	return 1;
}
//Creating our windows Menu
HMENU CreateDLLWindowMenu()
{
	HMENU hMenu;
	hMenu = CreateMenu();
	HMENU hMenuPopup;
	if (hMenu == NULL)
		return FALSE;
	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_EXIT, TEXT("Exit"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("File"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_DUMP, TEXT("Dump wow objects to out.log"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("Dump"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_HOOK, TEXT("Patch EndScene/DelIgnore_aux/ClosePetStables"));
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_UNHOOK, TEXT("UN-patch EndScene/DelIgnore_aux/ClosePetStables"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("FUNK"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_CTM, TEXT("CTM to target coordinates"));
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_CTM_CONSTANT, TEXT("Toggle walk-to-target"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("CTM"));


	return hMenu;
}

 static void __stdcall hook_func() {
	 
	 static int every_third_frame = 0;
	 static const char* script = "RunMacro(\"lole\")";
	 static const char* n = "";

	 if (every_third_frame == 0) {
		 if (BLAST_ENABLED) {
			 LUA_DoString(script, script, n);
		 }
		 if (CONSTANT_CTM_TARGET) {
			 walk_to_target();
		 }
	 }

	 every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;
}

 static void __stdcall change_target(const char* GUID_str) {

	 const std::string prefix = "LOLE_TARGET_GUID:";

	 if (strncmp(prefix.c_str(), GUID_str, prefix.length())) {
		 printf("[WARNING]: change_target: argument string \"%s\" didn't have prefix \"%s\", ignoring\n", GUID_str, prefix.c_str());
		 return;
	 }

	 char *end;

	 std::string GUID_numstr = GUID_str + prefix.length() + 2; // better make a copy of it. the GUID_str still has the "0x" prefix in it 
	 GUID_t GUID = strtoull(GUID_numstr.c_str(), &end, 16);

	 printf("got LOLE_TARGET_GUID: GUID = %llX\nGUID_str + prefix.length() + 2 = \"%s\"\n", GUID, GUID_numstr.c_str());

	 if (end != GUID_numstr.c_str() + GUID_numstr.length()) {
		 printf("[WARNING]: change_target: couldn't wholly convert GUID string argument (strtoull(\"%s\", &end, 16) failed, bailing out\n", GUID_numstr.c_str());
		 return;
	 }

	 memcpy((LPVOID)PLAYER_TARGET_ADDR, &GUID, sizeof(GUID));
 }



 static const BYTE EndScene_original[] = {
	0x6A, 0x20, // push 20
	0xB8, 0xD8, 0xB9, 0x08, 0x6C // MOV EAX, 6C08B9D8 after this should be mu genitals :D
 };

 static BYTE EndScene_patch[] = {
	 0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	 0x90, 0x90
 };

 static const BYTE DelIgnore_aux_original[] = {
	 0x55, // push EBP
	 0x8B, 0xEC, // mov EBP, ESP
	 0x8B, 0x45, 0x10, // MOV EAX, DWORD PTR SS:[ARG.3]
 };

 static BYTE DelIgnore_aux_patch[] = {
	 0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	 0xC3 
 };

 static const BYTE ClosePetStables_original[] = {
	 0xE8, 0xBB, 0xFF, 0xFF, 0xFF // CALL 004FAC60. this wouldn't actually even need a trampoline lol
 };

 static BYTE ClosePetStables_patch[] = {
	 0xE9, 0x00, 0x00, 0x00, 0x00
 };

 static int hook_all() {
	 DWORD bytes;
	
	 printf("Patching EndScene at %p...", (LPVOID)EndScene);
	 WriteProcessMemory(glhProcess, EndScene, EndScene_patch, sizeof(EndScene_patch), &bytes);
	 printf("OK!\n");
	
	 printf("Patching DelIgnore_aux at %p...", (LPVOID)DelIgnore_aux);
	 WriteProcessMemory(glhProcess, (LPVOID)DelIgnore_aux, DelIgnore_aux_patch, sizeof(DelIgnore_aux_patch), &bytes);
	 printf("OK!\n");
	
	 printf("Patching ClosePetStables at %p...", ClosePetStables);
	 WriteProcessMemory(glhProcess, (LPVOID)ClosePetStables, ClosePetStables_patch, sizeof(ClosePetStables_patch), &bytes);
	 printf("OK!\n", (LPVOID)ClosePetStables);

	 return 1;
 }

 static int unhook_all() {
	 DWORD bytes;
	
	 WriteProcessMemory(glhProcess, EndScene, EndScene_original, sizeof(EndScene_original), &bytes);
	 printf("Unpatched EndScene at %p!\n", EndScene);
	
	 WriteProcessMemory(glhProcess, (LPVOID)DelIgnore_aux, DelIgnore_aux_original, sizeof(DelIgnore_aux_original), &bytes);
	 printf("Unpatched DelIgnore_aux at %p!\n", (LPVOID)DelIgnore_aux);

	 WriteProcessMemory(glhProcess, (LPVOID)ClosePetStables, ClosePetStables_original, sizeof(ClosePetStables_original), &bytes);
	 printf("Unpatched ClosePetStables at %p!\n", (LPVOID)ClosePetStables);

	 return 1;
 }

static int prepare_EndScene_patch() {

	// this actually seems to work :)

	LPDIRECT3D9 c = Direct3DCreate9(D3D_SDK_VERSION);

	if (!c) {
		printf("Direct3DCreate9 failed.\n");
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	LPDIRECT3DDEVICE9 temp_dev;
	HRESULT r;
	if (FAILED(r = c->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &temp_dev))) {
		printf("CreateDevice failed with error: %lX\n", r);
		c->Release();
		return 0;
	}
	unsigned long* vtable = (unsigned long*)*((unsigned long*)temp_dev);

	printf("EndScene: %X\n", vtable[0x2A]); // d3d9.h, line 476

	EndScene = (HRESULT(*)(void))vtable[0x2A];

	temp_dev->Release();
	c->Release();


	static BYTE EndScene_trampoline[] = {
		// original EndScene opcodes follow
		0x6A, 0x20, // push 20
		0xB8, 0xD8, 0xB9, 0x08, 0x6C, // MOV EAX, 6C08B9D8 after this should be mu genitals :D
		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (endscene + 7) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call hook_func
		0x61, // popad
		0xC3 //ret
	};


	DWORD tr_offset = ((DWORD)EndScene_trampoline - (DWORD)EndScene - 5);
	memcpy(EndScene_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)EndScene + 7;
	memcpy(EndScene_trampoline + 8, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)&hook_func - (DWORD)EndScene_trampoline - 18;
	memcpy(EndScene_trampoline + 14, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)EndScene_trampoline, sizeof(EndScene_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("EndScene trampoline: %p\n", &EndScene_trampoline);
	
	return 1;

}



static int prepare_DelIgnore_aux_patch() {

	// DelIgnore == 5BA4B0, 
	// but a more easy-to-hook func is at 540D10 (EAX = string arg passed to DelIgnore). 
	// This is called at DelIgnore:5BA4D2


	static BYTE DelIgnore_trampoline[] = {
		// from the original DelIgnore-related func
		0x55, // push EBP
		0x8B, 0xEC, // mov EBP, ESP
		0x8B, 0x45, 0x10, // MOV EAX, DWORD PTR SS:[ARG.3], actually trashes EAX
		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (540D10 + 6 = 540D16)
		0x60, // pushad
		0x8B, 0x45, 0x08, // this is supposed to load the SS:[ARG.1] to EAX
		0x50, // push EAX, we know that eax contains string address
		0xE8, 0x00, 0x00, 0x00, 0x00, // call targeting function :D loloz
		0x61, // popad
		0xC3 //ret
	};


	DWORD tr_offset = ((DWORD)DelIgnore_trampoline - (DWORD)DelIgnore_aux - 5);
	memcpy(DelIgnore_aux_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)DelIgnore_aux + 6;
	memcpy(DelIgnore_trampoline + 7, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)&change_target - (DWORD)DelIgnore_trampoline - 21;
	memcpy(DelIgnore_trampoline + 17, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)DelIgnore_trampoline, sizeof(DelIgnore_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("DelIgnore_aux trampoline: %p\n", &DelIgnore_trampoline);

	return 1;
}

static int prepare_ClosePetStables_patch() {
	
	// ClosePetStables = 0x4FACA0

	static BYTE ClosePetStables_trampoline[] = {
		// original opcodes from ClosePetStables
		0xE8, 0x00, 0x00, 0x00, 0x00, // CALL 004FAC60. need to insert address relative to this trampoline 

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (ClosePetStables + 5) onto stack for ret
		0x60, // pushad
		0xE8, 0x00, 0x00, 0x00, 0x00, // call own function
		0x61, // popad
		0xC3 //ret
	};

	DWORD orig_CALL_target = 0x4FAC60;
	DWORD CALL_target_offset = ((DWORD)orig_CALL_target - ((DWORD)ClosePetStables_trampoline + 5));
	memcpy(ClosePetStables_trampoline + 1, &CALL_target_offset, sizeof(CALL_target_offset));

	DWORD tr_offset = ((DWORD)ClosePetStables_trampoline - (DWORD)ClosePetStables - 5);
	memcpy(ClosePetStables_patch + 1, &tr_offset, sizeof(tr_offset));


	DWORD ret_addr = (DWORD)ClosePetStables + 5;
	memcpy(ClosePetStables_trampoline + 6, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)&walk_to_target - (DWORD)ClosePetStables_trampoline - 16;
	memcpy(ClosePetStables_trampoline + 12, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)ClosePetStables_trampoline, sizeof(ClosePetStables_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("ClosePetStables trampoline: %p\n", &ClosePetStables_trampoline);

	return 1;

}


DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	MSG messages;
	char *pString = reinterpret_cast<char * > (lpParam);
	HMENU hMenu = CreateDLLWindowMenu();
	RegisterDLLWindowClass("DLLWindowClass");
	
	HWND hwnd = CreateWindowEx(0, "DLLWindowClass", pString, WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, hMenu, inj_hModule, NULL);
	ShowWindow(hwnd, SW_SHOWNORMAL);

	RegisterHotKey(hwnd, 100, MOD_ALT, 'G');	// G is a special case. (blast XD)

	prepare_EndScene_patch();
	prepare_DelIgnore_aux_patch();
	prepare_ClosePetStables_patch();
	
	while (GetMessage(&messages, NULL, 0, 0) && !STOP_SIGNALED) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 1;
}


LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int lo, hi;

	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MYMENU_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case MYMENU_DUMP:
			dump_wowobjects_to_log();
			break;
		case MYMENU_HOOK: 
			hook_all();
			break;
			
		case MYMENU_UNHOOK:
			unhook_all();
			break;
		case MYMENU_CTM: {
			
			ObjectManager OM;
			WowObject o = OM.get_object_by_GUID(OM.get_localGUID());
			if (!o.valid()) {
				break;
			}
		
			WowObject t = OM.get_object_by_GUID(o.unit_get_target_GUID());
			if (!t.valid()) {
				break;
			}
			click_to_move(t.get_pos_x(), t.get_pos_y(), t.get_pos_z(), CTM_MOVE, 0);

		   }
		case MYMENU_CTM_CONSTANT: {
			CONSTANT_CTM_TARGET = !CONSTANT_CTM_TARGET;
			printf("CONSTANT_CTM: %d\n", CONSTANT_CTM_TARGET);
			break;
		}
		}
		break;
	case WM_HOTKEY:
		lo = LOWORD(lParam);
		hi = HIWORD(lParam);

		if (lo == MOD_ALT) {
			if (hi == MKEY_G) {
				BLAST_ENABLED = !BLAST_ENABLED;
				printf("G pressed BLAST = %d\n", BLAST_ENABLED);
				return TRUE;
			}
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{

	HANDLE hProcess = GetCurrentProcess();
	HANDLE windowThread = INVALID_HANDLE_VALUE;

	glhProcess = hProcess;

	AllocConsole();
	freopen("CONOUT$", "wb", stdout);

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: 
		patch_LUA_prot(hProcess);
		windowThread = CreateThread(0, NULL, ThreadProc, (LPVOID)"Dump", NULL, NULL);
		inj_hModule = hModule;
		break;
	
	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");
		// might want to suspend the thread :DD
		STOP_SIGNALED = 1;
		WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

