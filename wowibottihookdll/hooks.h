#pragma once

#include <Windows.h>
#include <D3D9.h>
#include <string>
#include <vector>

#include "patch.h"

struct hookable_t;
struct trampoline_t;
struct patch_t;

extern int should_unpatch;

int prepare_pipe_data();

typedef struct patch_serialized {
	UINT32 buffer_size;
	BYTE *buffer;
	patch_serialized(UINT32 patch_addr, UINT32 patch_size, const BYTE *original_opcodes, const BYTE *patch);
	patch_serialized() { memset(this, 0x0, sizeof(*this)); }
} patch_serialized;

extern const UINT32 PIPE_PROTOCOL_MAGIC;

typedef struct pipe_data {
	
	UINT32 num_patches;
	std::vector<BYTE> data;
	int add_patch(const patch_serialized &p);
	pipe_data() {
		data.resize(3*sizeof(UINT32)); // reserve data for [meta]
		UINT32 meta_size = 3 * sizeof(UINT32);
		memcpy(&data[0], &PIPE_PROTOCOL_MAGIC, sizeof(UINT32));
		memcpy(&data[4], &num_patches, sizeof(UINT32));
		memcpy(&data[8], &meta_size, sizeof(UINT32));
	}
} pipe_data;

extern pipe_data PIPEDATA;

typedef struct trampoline_t {
	BYTE bytes[128];
	size_t length;
	trampoline_t &append_relative_offset(DWORD offset);
	trampoline_t &append_CALL(DWORD funcaddr);

	trampoline_t &append_bytes(const BYTE* b, int size);
	trampoline_t &append_hexstring(const char *hexstr);

	trampoline_t &append_default_return(const patch_t *p);
	trampoline_t &append_original_opcodes(const patch_t *p);
	
	trampoline_t() : length(0) {
		memset(bytes, 0x0, sizeof(bytes));
		DWORD oldprotect;
		VirtualProtect((LPVOID)bytes, sizeof(bytes), PAGE_EXECUTE_READWRITE, &oldprotect);
	}

	template <typename T> trampoline_t &operator << (const T& arg);
} trampoline_t;

extern pipe_data PIPEDATA;

void hook_DrawIndexedPrimitive();
void unhook_DrawIndexedPrimitive();

typedef struct inpevent_t {
	DWORD event;
	int param;
	int x;
	int y;
	DWORD unk1;
} inpevent_t;


hookable_t *find_hookable(const std::string &funcname);

int unpatch_all();