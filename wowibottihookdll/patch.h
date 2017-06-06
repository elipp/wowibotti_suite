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

