#pragma once

#include <Windows.h>
#include <cassert>
#include "defs.h"
#include "hooks.h"

#define PATCHADDR_LATER 0xFFFFFFFF

typedef struct patch_t;

typedef const trampoline_t* (*preparepatchfunc)(patch_t *);

typedef struct patch_t {
	BYTE original[16];
	BYTE patch[16];
	int size;
	DWORD patch_addr;
	const trampoline_t *tramp;
	preparepatchfunc preparef;

	patch_t() {}
	
	// the psize argument must be ABSOLUTELY correct!

	patch_t(DWORD paddr, DWORD psize, preparepatchfunc f) : patch_addr(paddr), size(psize), preparef(f) {
		// get original opcodes

		if (paddr != PATCHADDR_LATER) {
			memcpy(original, (LPVOID)patch_addr, size);
		}

		memset(patch, 0x90, sizeof(patch)); // the debugger gets fucked up without this
		patch[0] = 0xE9;

		tramp = preparef(this);

		// preparepatchfunc must set patch_addr to something else than PATCHADDR_LATER.
		assert(patch_addr != PATCHADDR_LATER);

		// just look at the first 4 bytes of the original instructions
		assert((*(DWORD*)(original) | 0x0) > 0); 

		DWORD tr_offset = ((DWORD)tramp->bytes - (DWORD)patch_addr - 5);
		memcpy(patch + 1, &tr_offset, sizeof(tr_offset));
	}

	void enable() { WriteProcessMemory(glhProcess, (LPVOID)patch_addr, patch, size, NULL); }
	void disable() { WriteProcessMemory(glhProcess, (LPVOID)patch_addr, original, size, NULL); }
};

typedef struct hookable_t {
	std::string funcname;
	patch_t patch;
};

//
//extern const BYTE LUA_prot_original[];
//extern BYTE LUA_prot_patch[];
//extern const size_t LUA_prot_patchsize;
//
//extern const BYTE EndScene_original[];
//extern BYTE EndScene_patch[];
//extern const size_t EndScene_patchsize;
//
//extern const BYTE DelIgnore_original[];
//extern BYTE DelIgnore_patch[];
//extern const size_t DelIgnore_patchsize;
//
//extern const BYTE ClosePetStables_original[];
//extern BYTE ClosePetStables_patch[];
//extern const size_t ClosePetStables_patchsize;
//
//extern const BYTE CTM_main_original[];
//extern BYTE CTM_main_patch[];
//extern const size_t CTM_main_patchsize;
//
//extern const BYTE CTM_aux_original[];
//extern BYTE CTM_aux_patch[];
//extern const size_t CTM_aux_patchsize;
//
//extern const BYTE CTM_finished_original[];
//extern BYTE CTM_finished_patch[];
//extern const size_t CTM_finished_patchsize;
//
//extern const BYTE spell_errmsg_original[];
//extern BYTE spell_errmsg_patch[];
//extern const size_t spell_errmsg_patchsize;
//
//extern const BYTE sendpacket_original[];
//extern BYTE sendpacket_patch[];
//extern const size_t sendpacket_patchsize;
//
//extern const BYTE recvpacket_original[];
//extern BYTE recvpacket_patch[];
//extern const size_t recvpacket_patchsize;
//
//extern const BYTE present_original[];
//extern BYTE present_patch[];
//extern const size_t present_patchsize;
//
//extern const BYTE mbuttondown_original[];
//extern BYTE mbuttondown_patch[];
//extern const size_t mbuttondown_patchsize;
//
//extern const BYTE mbuttonup_original[];
//extern BYTE mbuttonup_patch[];
//extern const size_t mbuttonup_patchsize;
//
//extern const BYTE drawindexedprimitive_original[];
//extern BYTE drawindexedprimitive_patch[];
//extern const size_t drawindexedprimitive_patchsize;
