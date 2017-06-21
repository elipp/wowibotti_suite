#include "patch.h"

patch_t::patch_t(DWORD paddr, DWORD psize, preparepatchfunc f) : patch_addr(paddr), size(psize), preparef(f) {
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
	assert((DEREF(original) | 0x0) > 0);

	DWORD tr_offset = ((DWORD)tramp->bytes - (DWORD)patch_addr - 5);
	memcpy(patch + 1, &tr_offset, sizeof(tr_offset));
}