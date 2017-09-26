#pragma once

#include <Windows.h>
#include <cassert>
#include "defs.h"
#include "hooks.h"

#define PATCHADDR_LATER 0xFFFFFFFF

struct trampoline_t;
struct patch_t;

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

	patch_t(DWORD paddr, DWORD psize, preparepatchfunc f);
	void enable() { WriteProcessMemory(glhProcess, (LPVOID)patch_addr, patch, size, NULL); }
	void disable() { WriteProcessMemory(glhProcess, (LPVOID)patch_addr, original, size, NULL); }
} patch_t;

typedef struct hookable_t {
	std::string funcname;
	patch_t patch;
} hookable_t;


