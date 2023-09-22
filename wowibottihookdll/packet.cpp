#include "packet.h"
#include "defs.h"

#define DEREFD(X) *(DWORD*)((X))
#define DEREFB(X) *(BYTE*)((X))

static DWORD get_sockobj() {
	//[0xD4332C + 2198]

	//DWORD A = DEREFD(0xD4332C);
	//if (A == 0) {
	//	return 0;
	//}

	//return DEREFD(A + 0x2198);

	DWORD S1 = DEREF<DWORD>(0xC79CF4);
	DWORD sockobj = DEREF<DWORD>(S1+0x2E38);
	return sockobj;

}

SOCKET get_wow_socket_handle() {
	// the address expression we're following here is [[0xD4332C + 2198] + 4]

	DWORD sockobj = get_sockobj();
	if (!sockobj) return NULL;

	return DEREF<SOCKET>(sockobj + 0x4);
}

struct encrypt_t {

};

int call_encrypt_header(BYTE* packet) {
	// encrypt header: 0x4665B0
	// for outgoing packets, it's called at 46776E
	DWORD sockobj = get_sockobj();
	DWORD encrypt_addr = 0x4665B0;
	
	__asm {
		mov ecx, sockobj;
		push 0x6;
		push packet;
		call encrypt_addr;
	}

	return 1;

}

int encrypt_packet_header(BYTE* packet) {

	call_encrypt_header(packet);

	return 1;
}

