#include "hooks.h"
#include "addrs.h"
#include "defs.h"

static HRESULT(*EndScene)(void);

struct hookable {
	std::string funcname;
	LPVOID address;
	const BYTE *original_opcodes;
	BYTE *patch;
	size_t patch_size;
	int(*prepare_patch)(LPVOID, hookable&);
};


static const BYTE EndScene_original[] = {
	0x6A, 0x20, // push 20
	0xB8, 0xD8, 0xB9, 0x08, 0x6C // MOV EAX, 6C08B9D8 after this should be mu genitals :D
};

static BYTE EndScene_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	0x90, 0x90
};

static const BYTE DelIgnore_original[] = {
	0x6A, 0x01, // push 1
	0x6A, 0x01, // push 1
	0x8D, 0x4D, 0xFC, // LEA ECX, [LOCAL.1]
};

static BYTE DelIgnore_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
	0x90, 0x90
};

static const BYTE ClosePetStables_original[] = {
	0xE8, 0xBB, 0xFF, 0xFF, 0xFF // CALL 004FAC60. this wouldn't actually even need a trampoline lol
};

static BYTE ClosePetStables_patch[] = {
	0xE9, 0x00, 0x00, 0x00, 0x00
};



static int prepare_EndScene_patch(LPVOID hook_func_addr, hookable &h) {

	// this actually seems to work :)

	printf("Preparing EndScene patch...\n");

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

	printf("Found EndScene at: %X\n", vtable[0x2A]); // d3d9.h, line 476

	EndScene = (HRESULT(*)(void))vtable[0x2A];

	h.address = EndScene;

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

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)EndScene_trampoline - 18;
	memcpy(EndScene_trampoline + 14, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)EndScene_trampoline, sizeof(EndScene_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("OK. EndScene trampoline: %p, hookfunc addr: %p\n", &EndScene_trampoline, hook_func_addr);

	return 1;

}



static int prepare_DelIgnore_patch(LPVOID hook_func_addr, hookable &h) {

	// DelIgnore == 5BA4B0, 
	// but the string appears in EAX after a call to 72DFF0 (called at 5BA4BC)
	// so hook func at 7BA4C1!

	printf("Preparing DelIgnore patch...\n");


	uint PATCH_ADDR = 0x7BA4C1;

	static BYTE DelIgnore_trampoline[] = {
		// from the original DelIgnore func
		0x6A, 0x01, // push 1
		0x6A, 0x01, // push 1
		0x8D, 0x4D, 0xFC, // LEA ECX, [LOCAL.1]

		0x68, 0x00, 0x00, 0x00, 0x00, // push return address (5BA4C1 + 7 = 5BA4C8)
		0x60, // pushad
		0x50, // push EAX, we know that eax contains string address at this point
		0xE8, 0x00, 0x00, 0x00, 0x00, // call targeting function :D loloz
		0x61, // popad
		0xC3 //ret
	};


	DWORD tr_offset = ((DWORD)DelIgnore_trampoline - (DWORD)DelIgnore_hookaddr - 5);
	memcpy(DelIgnore_patch + 1, &tr_offset, sizeof(tr_offset));

	DWORD ret_addr = (DWORD)DelIgnore_hookaddr + 7;
	memcpy(DelIgnore_trampoline + 8, &ret_addr, sizeof(ret_addr)); // add return address

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)DelIgnore_trampoline - 19;
	memcpy(DelIgnore_trampoline + 15, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)DelIgnore_trampoline, sizeof(DelIgnore_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("OK.\nDelIgnore trampoline: %p, hookfunc addr: %p\n", &DelIgnore_trampoline, hook_func_addr);


	return 1;
}

static int prepare_ClosePetStables_patch(LPVOID hook_func_addr, hookable &h) {

	printf("Preparing ClosePetStables patch...\n");

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

	DWORD hookfunc_offset = (DWORD)hook_func_addr - (DWORD)ClosePetStables_trampoline - 16;
	memcpy(ClosePetStables_trampoline + 12, &hookfunc_offset, sizeof(DWORD)); // add hookfunc offset

	DWORD oldprotect;
	VirtualProtect((LPVOID)ClosePetStables_trampoline, sizeof(ClosePetStables_trampoline), PAGE_EXECUTE_READWRITE, &oldprotect);

	printf("OK.\nClosePetStables trampoline: %p, hook_func_addr: %p\n", &ClosePetStables_trampoline, hook_func_addr);

	return 1;

}

static hookable hookable_functions[] = {
	{ "EndScene", 0x0, EndScene_original, EndScene_patch, sizeof(EndScene_original), prepare_EndScene_patch},
	{ "DelIgnore", (LPVOID)DelIgnore_hookaddr, DelIgnore_original, DelIgnore_patch, sizeof(DelIgnore_original), prepare_DelIgnore_patch },
	{ "ClosePetStables", (LPVOID)ClosePetStables, ClosePetStables_original, ClosePetStables_patch, sizeof(ClosePetStables_original), prepare_ClosePetStables_patch }
};


int install_hook(const std::string &funcname, LPVOID hook_func_addr) {
	//for (int i = 0; i < sizeof(hookable_functions) / sizeof(hookable_functions[0]); ++i) {
	for (auto &h : hookable_functions) {
		//const hookable &h = hookable_functions[i];
		if (funcname == h.funcname) {
			h.prepare_patch(hook_func_addr, h);
			DWORD bytes;
			printf("Patching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.patch, h.patch_size, &bytes);
			printf("OK!\n");
			return bytes;
		}
	}
	return 0;
}

int uninstall_hook(const std::string &funcname) {
	//for (int i = 0; i < sizeof(hookable_functions) / sizeof(hookable_functions[0]); ++i) {
	for (auto &h : hookable_functions) {
		//	const hookable &h = hookable_functions[i];
		if (funcname == h.funcname) {
			DWORD bytes;
			h.prepare_patch(0x0, h);
			printf("Unpatching func \"%s\" at %X...", funcname.c_str(), (DWORD)h.address);
			WriteProcessMemory(glhProcess, h.address, h.original_opcodes, h.patch_size, &bytes);
			printf("OK!\n");
			return bytes;
		}
	}
	return 0;
}

