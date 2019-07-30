#pragma once

#define PIPE_WRITE_BUF_SIZE 1024
#define PIPE_READ_BUF_SIZE 1024

typedef struct PPIPE_t {
	HANDLE hPipe;
	char *write_buf;
	char *read_buf;
	std::string path;

	int read() {

		DWORD sc, num_bytes;
		sc = ReadFile(hPipe, read_buf, PIPE_READ_BUF_SIZE, &num_bytes, NULL);

		if (!sc || num_bytes == 0) {
			PRINT("Reading from pipe %s failed, error: 0x%X\n", path.c_str(), GetLastError());
			return 0;
		}
		else {
			read_buf[num_bytes] = '\0';
			PRINT("Got response %s from pipe server\n", read_buf);
		}

		return 1;
	}

	int write(const BYTE* data, int data_size) {
		DWORD sc, num_bytes;
		sc = WriteFile(hPipe, data, data_size, &num_bytes, NULL);
		if (!sc || num_bytes == 0) {
			if (GetLastError() == ERROR_PIPE_LISTENING) {
				//PRINT("pipe thread: WARNING: ERROR_PIPE_LISTENING!\n");
				return 0;
			}
			else if (GetLastError() == ERROR_BROKEN_PIPE) {
				PRINT("pipe thread: ERROR_BROKEN_PIPE\n");
				return -1;
			}
			else {
				PRINT("pipe thread: ERROR: 0x%X\n", GetLastError());
				return -2;
			}
		}
		else {
			return 1; // successe'd
		}



	}

} PPIPE_t;

PPIPE_t *create_pipe();
void destroy_pipe(PPIPE_t *p);

extern PPIPE_t *PIPE;

extern HINSTANCE inj_hModule;

extern std::string DLL_path;
extern std::string DLL_base_path;
