#pragma once

#include <Windows.h>
#include <D3D9.h>
#include <string>
#include <vector>

int hook_all();
int unhook_all();

int prepare_patches_and_pipe_data();

struct patch_serialized {
	UINT32 buffer_size;
	BYTE *buffer;
	patch_serialized(UINT32 patch_addr, UINT32 patch_size, const BYTE *original_opcodes, const BYTE *patch);
};

extern const UINT32 PIPE_PROTOCOL_MAGIC;

struct pipe_data {
	
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
};

extern pipe_data PIPEDATA;

struct patchbuffer_t {
	BYTE bytes[128];
	size_t length;
	void append_relative_offset(DWORD offset);
	void append_CALL(DWORD funcaddr);
	patchbuffer_t() {
		DWORD oldprotect;
		VirtualProtect((LPVOID)bytes, sizeof(bytes), PAGE_EXECUTE_READWRITE, &oldprotect);
	}

	template <typename T> patchbuffer_t &operator << (const T& arg);

};

extern pipe_data PIPEDATA;

void reset_camera();
