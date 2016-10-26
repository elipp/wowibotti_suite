#pragma once

#define WIN32_LEAN_AND_MEAN


#include <Windows.h>
#include <WinSock2.h>

SOCKET get_wow_socket_handle();
int encrypt_packet(BYTE* packet);

