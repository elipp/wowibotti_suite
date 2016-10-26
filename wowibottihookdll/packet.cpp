#include "packet.h"
#include "defs.h"

#define DEREF(X) *(DWORD*)((X))
#define DEREFB(X) *(BYTE*)((X))

static DWORD get_sockobj() {
	//[0xD4332C + 2198]

	DWORD A = DEREF(0xD4332C);
	if (A == 0) {
		return 0;
	}

	return DEREF(A + 0x2198);
}

SOCKET get_wow_socket_handle() {
	// the address expression we're following here is [[0xD4332C + 2198] + 4]

	DWORD sockobj = get_sockobj();
	if (!sockobj) return NULL;

	DWORD S = DEREF(sockobj + 0x4);

	PRINT("socket = %X\n", S);

	return (SOCKET)S;
}

int encrypt_packet(BYTE* packet) {
	DWORD sockobj = get_sockobj();

	//PRINT("sockobj = %X\n", sockobj);

	for (int i = 0; i < 6; ++i) {
	
		BYTE *S121 = (BYTE*)(sockobj + 0x121);
		BYTE *S122 = (BYTE*)(sockobj + 0x122);

		BYTE EAX1 = *S121;
		BYTE EDX1 = DEREFB(sockobj + EAX1 + 0x127);
		BYTE DLXOR1 = packet[i] ^ EDX1;
		
		DLXOR1 = (DLXOR1 + *S122);
		packet[i] = DLXOR1;
		
		*S121 += 1;
		BYTE DL2 = *S121;
		
		*S122 = packet[i];
		
		*S121 = DL2 % (BYTE)0x14;

	}

	return 1;
}

