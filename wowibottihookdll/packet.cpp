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

	DWORD S1;
	readAddr(0xC79CF4, &S1, sizeof(S1));
	
	DWORD sockobj;
	readAddr(S1 + 0x2E38, &sockobj, sizeof(sockobj));

	return sockobj;

}

SOCKET get_wow_socket_handle() {
	// the address expression we're following here is [[0xD4332C + 2198] + 4]

	DWORD sockobj = get_sockobj();
	if (!sockobj) return NULL;

	SOCKET s;
	readAddr(sockobj + 0x4, &s, sizeof(s));

	return s;
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
	DWORD sockobj = get_sockobj();

	//PRINT("sockobj = %X\n", sockobj);

	// for WOTLK, the encryption happens at around 0x774FC3
	// the increment byte is at [sockobj + 0x1C] + 0x100 and 0x101

	//for (int i = 0; i < 6; ++i) {
	//
	//	BYTE *S121 = (BYTE*)(sockobj + 0x121);
	//	BYTE *S122 = (BYTE*)(sockobj + 0x122);

	//	BYTE EAX1 = *S121;
	//	BYTE EDX1 = DEREFB(sockobj + EAX1 + 0x127);
	//	BYTE DLXOR1 = packet[i] ^ EDX1;
	//	
	//	DLXOR1 = (DLXOR1 + *S122);
	//	packet[i] = DLXOR1;
	//	
	//	*S121 += 1;
	//	BYTE DL2 = *S121;
	//	
	//	*S122 = packet[i];
	//	
	//	*S121 = DL2 % (BYTE)0x14;

	//}

	call_encrypt_header(packet);

	return 1;
}

