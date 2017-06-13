#include "dipcapture.h"
#include "defs.h"
#include "hooks.h"

capture_t capture;

void enable_capture_render() {
	capture.active = 1;
}


int capture_t::open_dumpfile(const std::string &name) {

	fopen_s(&capture_outfile, name.c_str(), "wb");
	if (!capture_outfile) {
		PRINT("Couldn't open frame dump file %s!\n", name.c_str());
		return 0;
	}

	PRINT("Opened frame dump file \"%s\"!\n", name.c_str());

	return 1;

}

void capture_t::reset() {
	active = 0;
	need_to_stop = 0;
	drawcall_count = 0;
	if (capture_outfile) {
		fclose(capture_outfile);
		capture_outfile = NULL;
	}
	name_base = "";
	file_index = 1;

}

inline std::string capture_t::get_chunkpostfix(int index) {
	char buf[128];
	sprintf_s(buf, "_chunk%02d", file_index);

	return std::string(buf);
}

inline std::string capture_t::get_basename() {
	SYSTEMTIME t;
	GetLocalTime(&t);

	char buf[128];
	sprintf_s(buf, "D:\\wowframe\\%02d%02d_%02d%02d%02d.rdmp", t.wDay, t.wMonth, t.wHour, t.wMinute, t.wSecond);

	return std::string(buf);
}

void capture_t::start() {

	reset();

	name_base = get_basename();

	if (open_dumpfile(name_base + get_chunkpostfix(file_index))) {
		hook_DrawIndexedPrimitive();
		need_to_stop = 1; // this is to signal Present to unhook shit
		active = 1;
	}
}

void capture_t::finish() {
	PRINT("Frame dump %s complete! (%d stages in %d chunks)\n", name_base.c_str(), drawcall_count, file_index);
	unhook_DrawIndexedPrimitive();
	reset();
}

int capture_t::new_chunk() {
	++file_index;
	fclose(capture_outfile);
	if (!open_dumpfile(name_base + get_chunkpostfix(file_index))) {
		PRINT("new_chunk failed!\n");
		capture_outfile = NULL;
		return 0;
	}
	return 1;
}

int capture_t::append_to_file(const rdmpheader_t *hdr, const LPVOID data, size_t data_size_bytes) {
#define MAX_CHUNKSIZE (1 << 31)
	if (!capture_outfile) return 0;

	if (ftell(capture_outfile) + data_size_bytes > MAX_CHUNKSIZE) {
		if (!new_chunk()) return 0;
	}

	fwrite(hdr, sizeof(rdmpheader_t), 1, capture_outfile);
	fwrite(data, 1, data_size_bytes, capture_outfile);

	return 1;
}