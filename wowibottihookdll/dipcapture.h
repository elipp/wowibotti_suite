#pragma once

#include <Windows.h>

#include <string>
#include <cstdlib>
#include <cstdio>

typedef struct rdmpheader_t {
	DWORD MAGIC;
	DWORD pitch;
	DWORD width;
	DWORD height;
	DWORD stagenum;
	DWORD callstack[8];
} rmpheader_t;

typedef struct {
	int active;
	int need_to_stop;
	int drawcall_count;
	FILE *capture_outfile;
	std::string name_base;
	int file_index;

	int open_dumpfile(const std::string &name);
	void reset();

	inline std::string get_chunkpostfix(int index);
	inline std::string get_basename();

	void start();
	void finish();
	int new_chunk();
	int append_to_file(const rdmpheader_t *hdr, const LPVOID data, size_t data_size_bytes);

} capture_t;

extern capture_t capture;

void enable_capture_render();
