#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>

#include "defs.h"

SOCKET get_wow_socket_handle();
int encrypt_packet_header(BYTE* packet);

inline void dump_packet(BYTE *packet, size_t len) {
	for (int i = 0; i < len; ++i) {
		PRINT("%02X ", packet[i]);
	}

	PRINT("\n");
}

