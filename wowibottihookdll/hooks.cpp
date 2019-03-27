#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>
#include <process.h>

#include <queue>
#include <cassert>
#include <unordered_map>
#include <ctime>

#include <d3d9.h>

#include "hooks.h"
#include "addrs.h"
#include "defs.h"
#include "opcodes.h"
#include "ctm.h"
#include "creds.h"
#include "timer.h"
#include "lua.h"
#include "linalg.h"
#include "patch.h"
#include "dipcapture.h"
#include "wc3mode.h"
#include "input.h"
#include "patch.h"
#include "sslconn.h"

extern void close_console();

extern HINSTANCE inj_hModule;

static std::vector<std::string> ENABLED_PATCHES;

int should_unpatch = 0;

//static HRESULT(*EndScene)(void);
pipe_data PIPEDATA;

const UINT32 PIPE_PROTOCOL_MAGIC = 0xAB30DD13;

#define PUSHAD (BYTE)0x60
#define POPAD (BYTE)0x61
#define RET (BYTE)0xC3

template <typename T> trampoline_t &trampoline_t::operator << (const T& arg) {
	int size = sizeof(arg);
	BYTE buf[16];
	memcpy(buf, &arg, size);

	for (int i = 0; i < size; ++i) {
		bytes[this->length] = buf[i];
		++this->length;
	}

	return (*this);
}

trampoline_t & trampoline_t::append_hexstring(const char *hexstr) {

	// this expects the string to be in the format FFEEDDAA3355 etc.

	int length = strlen(hexstr);

	assert((length & 1) == 0);

	int i = 0;
	while (i < length) {
		BYTE r;
		sscanf(hexstr + i, "%2hhX", &r);
		bytes[this->length] = r;
		++this->length;
		i += 2;
	}

	return (*this);
}



trampoline_t &trampoline_t::append_relative_offset(DWORD offset) {

	DWORD jump = offset - ((DWORD)bytes + length) - 4;
	//PRINT("offset: %X, length: %ld, jump: 0x%X\n", offset, length, jump);
	(*this) << jump;

	return (*this);

}

trampoline_t &trampoline_t::append_CALL(DWORD funcaddr) {
	(*this) << (BYTE)0xE8;
	append_relative_offset(funcaddr);
	return (*this);

}

trampoline_t &trampoline_t::append_bytes(const BYTE* b, int size) {
	for (int i = 0; i < size; ++i) {
		(*this) << b[i];
	}
	return (*this);

}

trampoline_t &trampoline_t::append_default_return(const patch_t *p) {
	(*this) << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	(*this) << RET;

	return (*this);
}

trampoline_t &trampoline_t::append_original_opcodes(const patch_t *p) {
	// this works unless the original opcodes include a CALL or a JMP
	this->append_bytes(p->original, p->size);

	return (*this);
}


static void register_luafunc_if_not_registered() {
	//if (!lua_registered) {
		register_lop_exec();
	//}
}

static void update_hwevent_tick() {
	typedef int tick_count_t(void);
	//int ticks = ((tick_count_t*)GetOSTickCount)();
	DWORD ticks = *(DWORD*)CurrentTicks;

	*(DWORD*)(LastHardwareAction) = ticks;
	// this should make us immune to AFK ^^
}

void hook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
	h->patch.enable();
}

void unhook_DrawIndexedPrimitive() {
	hookable_t *h = find_hookable("DrawIndexedPrimitive");
	h->patch.disable();
}


static void __stdcall call_pylpyr() {
	if (wc3mode_enabled())
		wc3_draw_pylpyrs();
}


static void __cdecl unload_DLL(LPVOID lpParameter) {

	close_console();
	FreeLibraryAndExitThread(inj_hModule, 0);
}

extern time_t in_world; // from opcodes.cpp

static int report_client_status() {

	// this only reports login screen and char select, world status is coming from LOLE (the addon)

	if (time(NULL) - in_world > 15) {
		const lua_rvals_t R = dostring_getrvals("IsConnectedToServer()");
		if (R.size() < 1 || R[0] == "nil") {
			const char* msg = "status STATUS_LOGIN_SCREEN";
			send_to_governor(msg, strlen(msg) + 1);
			return 1;
		}
		else {
			const lua_rvals_t R2 = dostring_getrvals("GetCharacterInfo(1)");
			std::string msg("status STATUS_CHAR_SELECT:" + R2[0]);
			send_to_governor(msg.c_str(), msg.length() + 1);
			return 2;
		}
	}
}


static int dbg_shown = 0;

static void __stdcall Present_hook() {

	init_custom_d3d(); // this doesn't do anything if it's already initialized
	
	if (!dbg_shown) {
		IDirect3DDevice9 *d = get_wow_ID3D9();
		PRINT("Present: %X, BeginScene: %X, EndScene: %X, DrawIndexedPrimitive: %X\n", get_Present(), get_BeginScene(), get_EndScene(), get_DrawIndexedPrimitive());
		dbg_shown = 1;
		srand(time(NULL));
		//connect_to_governor();
	}

	do_wc3mode_stuff();

	if (wc3mode_enabled()) {
		if (mouse_pressed) {
			need_mouseup = 1;
			add_mouseup();
			need_mouseup = 0;
		}
	}

	if (!ctm_check_direction()) {
		ctm_cancel();
	}

	register_luafunc_if_not_registered();

	static timer_interval_t fifty_ms(50);
	static timer_interval_t half_second(500);
	static timer_interval_t fifteen_seconds(15000);

	if (half_second.passed()) {
		update_hwevent_tick();

		if (credentials.valid && !credentials.logged_in) credentials.try_login();

		half_second.reset();
	}

	ctm_handle_delayed_posthook();
	ctm_abort_if_not_moving();


	if (fifty_ms.passed()) {
		ctm_purge_old();

		refollow_if_needed();
		ctm_act();

		fifty_ms.reset();
	}

	if (fifteen_seconds.passed()) {
		report_client_status(); //
		fifteen_seconds.reset();
	}

	//if (capture.active) {
	//	capture.start();
	//	capture.active = 0;
	//}
	//else if (capture.need_to_stop) {
	//	capture.finish();
	//}
	
	if (should_unpatch) {
		PRINT("should unpatch! unpatching!\n");
		unpatch_all();
		DoString("ConsoleExec(\"reloadui\")");
		should_unpatch = 0;
	}

}

static int get_header_length(BYTE* pkt) {
	if (pkt[0] & 0x80) {
		return 5;
		//	h.content_length = (((((uint32_t)pkt[0]) & 0x7F) << 16) | (((uint32_t)pkt[1]) << 8) | pkt[2]);
		//memcpy(&h.opcode, &b[3], sizeof(uint16_t));
	}
	else {
		return 4;
		//	h.content_length = (((uint32_t)b[0]) << 8 | b[1]);
		//memcpy(&h.opcode, &b[2], sizeof(uint16_t));
	}
}

static uint32_t s_get_content_length(BYTE* pkt) {
	if (pkt[0] & 0x80) {
		return (((((uint32_t)pkt[0]) & 0x7F) << 16) | (((uint32_t)pkt[1]) << 8) | pkt[2]);
	}
	else {
		return (((uint32_t)pkt[0]) << 8 | pkt[1]);
	}
}

static uint32_t c_get_content_length(BYTE *pkt) {
	return (((uint32_t)pkt[0]) << 8 | pkt[1]);
}

static int HAVE_NEW_PACKET;
static uint8_t WARDEN_PACKET[8096];
static int WARDEN_PACKET_LENGTH;
static int WARDEN_THREAD_RUNNING;

static SOCKET WARDEN_SOCKET = INVALID_SOCKET;

static DWORD WINAPI create_warden_socket() {
	WARDEN_SOCKET = INVALID_SOCKET;
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	int r = 0;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	r = getaddrinfo("li1406-250.members.linode.com", "6337", &hints, &result);
	if (r != 0) {
		printf("getaddrinfo failed with error: %d\n", r);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		WARDEN_SOCKET = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (WARDEN_SOCKET == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		r = connect(WARDEN_SOCKET, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (r == SOCKET_ERROR) {
			closesocket(WARDEN_SOCKET);
			WARDEN_SOCKET = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (WARDEN_SOCKET == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	PRINT("created socket for warden packet relaying!\n");

	return 0;
}

static void __stdcall EndScene_hook() {
	draw_custom_d3d();
}

enum {
	WARDEN_SOURCE_IN = 0,
	WARDEN_SOURCE_OUT = 1
};

enum {
	CMSG_CAST_SPELL = 0x12E,
	CMSG_SET_SELECTION = 0x13D,
	SMSG_WARDEN_DATA = 0x2E6,
	CMSG_WARDEN_DATA = 0x2E7,
};

static void print_bytes(const char* title, const uint8_t *bytes, int length, int modulo) {
	PRINT("%s:\n", title);
	for (int i = 0; i < length; ++i) {
		PRINT("%02X ", bytes[i]);
		if ((i % modulo) == (modulo - 1)) PRINT("\n");
	}
	PRINT("\n\n");
}


static void relay_warden_packet(BYTE *pkt, int pktlen, uint8_t source) {
	if (WARDEN_SOCKET == INVALID_SOCKET) { create_warden_socket(); }
	
	PRINT("relay: pktlen = %d, source = %d\n", pktlen, source);

	uint8_t buf[8192];
	buf[0] = source;
	memcpy(buf + 1, pkt, pktlen);

	print_bytes("warden packet", buf, pktlen + 1, 16);

	int r = send(WARDEN_SOCKET, (const char*)buf, pktlen + 1, 0);
	if (r < 0) {
		PRINT("send() returned %d, closing connection!\n", r);
		closesocket(WARDEN_SOCKET);
		WARDEN_SOCKET = INVALID_SOCKET;
	}
	else {
		PRINT("sent %d bytes\n", pktlen + 1);
	}
}


static void __stdcall dump_sendpacket(BYTE *packet) {

	uint32_t total_bytes = c_get_content_length(packet) + 2; 

	uint32_t opcode = 0;
	memcpy(&opcode, packet + 2, sizeof(opcode)); 
	// ^ interestingly, the client packets have 4 bytes for the opcode while the server packets only have 2.
	// the largest opcode value in mangos/src/game/Opcodes.h is something like 0x500 so no worries

	//PRINT("packet length: total: %d (actual data size without header: %d), opcode: 0x%X\n", length, length - 6, opcode);

	switch (opcode) {
	case CMSG_CAST_SPELL: {
		BYTE cast_count = packet[6];
		DWORD spellID;
		memcpy(&spellID, &packet[7], 4);
		BYTE flags = packet[11];
		PRINT("got CMSG_SPELL_CAST. total bytes: %d, spellID = %d, cast_count = %d, flags = 0x%X\n raw dump:\n", total_bytes, spellID, cast_count, flags);
		for (int i = 0; i < total_bytes; ++i) {
			PRINT("%02X ", packet[i]);
		}
		PRINT("\n\n");
		break;
	}

	case CMSG_WARDEN_DATA: 
		PRINT("GOT CMSG_WARDEN_DATA\n\n");
		//relay_warden_packet(packet, total_bytes, WARDEN_SOURCE_OUT);
		PRINT("\n");
		break;
	default:
		PRINT("opcode: 0x%04X (total_length %d)\n", opcode, total_bytes);
		for (int i = 6; i < total_bytes; ++i) {
			PRINT("%02X ", packet[i]);
		}
		PRINT("\n\n");
		break;
	}
}

static void __stdcall dump_recvpacket(BYTE *packet) {

	int header_length = get_header_length(packet);
	uint32_t total_bytes = s_get_content_length(packet) - (header_length - 2);

	uint16_t opcode;
	memcpy(&opcode, packet + (header_length - 2), sizeof(uint16_t));

	switch (opcode) {
		case SMSG_WARDEN_DATA:
				PRINT("GOT SMSG_WARDEN_DATA\n");
				relay_warden_packet(packet, total_bytes, WARDEN_SOURCE_IN);
				PRINT("\n");
				break;
		default:
			break;
	}



}

static void __stdcall ClosePetStables_hook() {
	lua_registered = 0;
	enable_wc3mode(0);
}

static void __stdcall broadcast_CTM(float *coords, int action) {

	float x, y, z;

	x = coords[0];
	y = coords[1];
	z = coords[2];

	PRINT("broadcast_CTM: got CTM coords: (%f, %f, %f), action = %d\n", x, y, z, action);

	DoString("RunMacroText(\"/lole broadcast ctm %.1f %.1f %.1f\")", x, y, z);
}


static void __stdcall CTM_finished_hookfunc() {

	CTM_t *c = ctm_get_current_action();
	if (!c) { return; }
	
	PRINT("called CTM_finished with ID = %ld\n", c->ID);

	c->handle_posthook();
}

static const trampoline_t *prepare_EndScene_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing EndScene patch...\n");

	DWORD EndScene = get_EndScene();
	PRINT("Found EndScene at 0x%X\n", EndScene);
	p->patch_addr = EndScene;
	
	tr << PUSHAD;

	tr.append_CALL((DWORD)EndScene_hook);
	tr << POPAD;
		
	memcpy(p->original, (LPVOID)EndScene, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; 
	
	return &tr;

}


static const trampoline_t *prepare_ClosePetStables_patch(patch_t *p) {

	static trampoline_t tr;

	tr << PUSHAD; 

	tr.append_CALL((DWORD)ClosePetStables_hook);
	tr << POPAD;

	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; 
	
	return &tr;

}


static const trampoline_t *prepare_CTM_finished_patch(patch_t *p) {
	static trampoline_t tr;

	tr << PUSHAD; // PUSHAD

	tr.append_CALL((DWORD)CTM_finished_hookfunc);
	tr << POPAD; // POPAD

	tr.append_bytes(p->original, p->size);

// NOTE: COULD BE THIS INSTEAD OF (p->patch_addr + p->size): DWORD ret_addr = 0x7273EF;

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET

	return &tr;
}

static const trampoline_t *prepare_sendpacket_patch(patch_t *p) {

	static trampoline_t tr;
	
	PRINT("dump_sendpacket offset: 0x%X, trampoline: 0x%X\n", dump_sendpacket, tr.bytes);

	DWORD retaddr = 0x467773;
	DWORD some_crypt_func = 0x4665B0;

	tr << PUSHAD // PUSHAD
	   << (BYTE)0x52; // push EDX (the packet address)

	tr.append_CALL((DWORD)dump_sendpacket);
	tr << POPAD; // POPAD

	tr.append_CALL(some_crypt_func); // from the original opcodes (append_original_opcodes doesn't work here, because CALL is relative)
	tr.append_default_return(p);

	return &tr;

}

static const trampoline_t *prepare_recvpacket_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("dump_recvpacket offset: 0x%X, trampoline: 0x%X\n", dump_recvpacket, tr.bytes);

	DWORD retaddr = 0x467ECD;

	tr << PUSHAD; // PUSHAD
	
	tr.append_hexstring("8B461C50"); // mov eax, dword ptr ds : [esi + 1c], push eax

	tr.append_CALL((DWORD)dump_recvpacket);
	tr << POPAD; // POPAD

	tr.append_original_opcodes(p);
	tr.append_default_return(p);

	return &tr;

}

enum {
	PDUMP_OUT = 0x1,
	PDUMP_IN = 0x2,
	PDUMP_REALM = 0x4,
	PDUMP_WORLD = 0x8
};



static std::string get_datestring() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[256];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d%m%Y%I%M%S", timeinfo);

	return buffer;
}

static std::string DIRECTORY_PREFIX;

static void create_dump_directory() {
	static int already_created = 0;
	if (already_created) return;

	DIRECTORY_PREFIX = "PACKETDUMP_" + get_datestring() + "\\";
	 
	if (CreateDirectory(DIRECTORY_PREFIX.c_str(), NULL)) {
		PRINT("created directory %s for packet dumps!\n", DIRECTORY_PREFIX.c_str());
	}

	already_created = 1;
}

static void dump_to_file(const std::string &filename, const uint8_t *buffer, int length) {
	FILE *fp;
	if (fopen_s(&fp, filename.c_str(), "wb")) { // 0 is success
		PRINT("failed to create file %s!\n", filename.c_str());
		return;
	}

	fwrite(buffer, 1, length, fp);
	fclose(fp);
	PRINT("dumped packet to file %s (len %d)\n", filename.c_str(), length);

}

static void dump_packet(const uint8_t *buffer, int length, int flags) {
	static int realm_pnum_in = 0;
	static int realm_pnum_out = 0;

	static int world_pnum_in = 0;
	static int world_pnum_out = 0;

	create_dump_directory(); // will do nothing if it's already created

	std::string prefix = DIRECTORY_PREFIX + ((flags & PDUMP_REALM) ? "REALM_" : "WORLD_") + ((flags & PDUMP_IN) ? "IN_" : "OUT_");

	std::string filename;

	if (flags & PDUMP_REALM) {
		if (flags & PDUMP_IN) {
			++realm_pnum_in;
			filename = prefix + std::to_string(realm_pnum_in);
		}
		else {
			++realm_pnum_out;
			filename = prefix + std::to_string(realm_pnum_out);
		}
	}
	else { // PDUMP_WORLD
		if (flags & PDUMP_IN) {
			++world_pnum_in;
			filename = prefix + std::to_string(world_pnum_in);
		}
		else {
			++world_pnum_out;
			filename = prefix + std::to_string(world_pnum_out);
		}
	}

//	dump_to_file(filename, buffer, length);
}

static void __stdcall dump_WS2recv(DWORD *args, DWORD recvlength) {

	static uint8_t packetbuf[4096 * 4];
	static char* worldpacket_buf;

	DWORD retaddr = args[-1]; // XD
	SOCKET s = args[0];
	char* outbuf = (char*)args[1];
	int length = args[2];
	int flags = args[3];

	//PRINT("%X, %X, %X, %X, %X, %d\n", retaddr, s, outbuf, length, flags, recvlength);

	switch (retaddr) {
	case 0x466F38:
		print_bytes("recvbuf (REALM packet)", (uint8_t*)outbuf, recvlength, 16);
		dump_packet((const uint8_t *)outbuf, recvlength, PDUMP_IN | PDUMP_REALM);
		break;

	case 0x467E64:
		if (length == 2) {
			// then we're probably dealing with "World" packets. it appears that the client calls recv() twice:
			// once for the first 2 bytes, and decrypts them to get the packet length (and in case of a big packet, probably another 1 byte)
			worldpacket_buf = outbuf;
			memcpy(packetbuf, outbuf, 2);
		}
		else {
			if (outbuf != worldpacket_buf + 2) {
				PRINT("dump_WS2recv: 2nd stage: WARNING: outbuf != worldpacket_buf + 2! (SOCKET: %X)\n", s);
				return;
			}
			else {
				memcpy(packetbuf + 2, outbuf, length);
				print_bytes("recvbuf (WORLD packet)", packetbuf, length + 2, 16);
				dump_packet(packetbuf, length, PDUMP_IN | PDUMP_WORLD);
			}
		}
		break;

	default:
		// these pop up sometimes, originating from the WININET module, so probably nothing to be worried about
		PRINT("dealing with ??? (%X)\n", retaddr);
		break;
	}
}

static void __stdcall dump_WS2send(DWORD *args) {
	DWORD retaddr = args[-1];
	SOCKET s = args[0];
	const char* buf = (const char*)args[1];
	int length = args[2];
	int flags = args[3];

	switch (retaddr) {
	case 0x467A39:
		PRINT("send(): have REALM packet\n");
		dump_packet((const uint8_t*)buf, length, PDUMP_REALM | PDUMP_OUT);
		break;
	case 0x467787:
		PRINT("send(): have WORLD packet\n");
		dump_packet((const uint8_t*)buf, length, PDUMP_WORLD | PDUMP_OUT);
		break;
	default:
		PRINT("send(): have ???? packet\n");
		break;
	}

}

static const trampoline_t *prepare_Present_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing Present patch...\n");

	DWORD Present = get_Present();
	PRINT("Found Present at 0x%X\n", Present);
	p->patch_addr = Present;

	tr << PUSHAD; // PUSHAD

	tr.append_CALL((DWORD)Present_hook);
	tr << POPAD; // POPAD
	
	memcpy(p->original, (LPVOID)Present, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET


	return &tr;
		
}

static void __stdcall DrawIndexedPrimitive_hook() {

	IDirect3DDevice9 *d3dd = (IDirect3DDevice9*)get_wow_d3ddevice();

	IDirect3DSwapChain9 *sc;
	if (FAILED(d3dd->GetSwapChain(0, &sc))) {
		PRINT("GetSwapChain failed\n");
		return;
	}
	IDirect3DSurface9 *s;

	if (FAILED(sc->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &s))) {
		PRINT("GetBackBuffer failed\n");
		return;
	}

	D3DLOCKED_RECT r;

	if (FAILED(s->LockRect(&r, NULL, D3DLOCK_DONOTWAIT))) {
		PRINT("%d LockRect failed\n", GetTickCount());
		return;
	}

	D3DSURFACE_DESC d;
	s->GetDesc(&d);

	rdmpheader_t hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.MAGIC = 0x700FF000;
	hdr.pitch = r.Pitch;
	hdr.width = d.Width;
	hdr.height = d.Height;
	hdr.stagenum = capture.drawcall_count;

	PRINT("stagenum: %d, pitch: %d\n", hdr.stagenum, hdr.pitch);

	CaptureStackBackTrace(0, 8, (PVOID*)&hdr.callstack[0], NULL); 

	capture.append_to_file(&hdr, r.pBits, r.Pitch * d.Height);

	s->UnlockRect();

	s->Release();
	sc->Release();

	++capture.drawcall_count;

}

static const trampoline_t *prepare_DrawIndexedPrimitive_patch(patch_t *p) {

	static trampoline_t tr;

	PRINT("Preparing DrawIndexedPrimitive patch...\n");

	DWORD DrawIndexedPrimitive = get_DrawIndexedPrimitive();
	PRINT("Found DrawIndexedPrimitive at 0x%X\n", DrawIndexedPrimitive);

	p->patch_addr = DrawIndexedPrimitive;
	tr << PUSHAD; 

	tr.append_CALL((DWORD)DrawIndexedPrimitive_hook);
	tr << POPAD; 

	memcpy(p->original, (LPVOID)DrawIndexedPrimitive, p->size);
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr

	tr << RET; // RET

	return &tr;

}

static int __stdcall mbuttondown_hook(DWORD wParam) {
	return 1;
}

// there's a mouse button (r/l) down handler at 8697E0

static const trampoline_t *prepare_mbuttondown_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttondown patch...\n");

	tr << PUSHAD; // PUSHAD
	tr << (BYTE)0x56; // push esi (contains WndProc/wParam)
	tr.append_CALL((DWORD)mbuttondown_hook);
	tr.append_hexstring("83F800"); // cmp eax, 0
	tr.append_hexstring("0F840D000000"); // jz branch 2
	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET; // ret
	
	// branch 2
	tr << POPAD; // POPAD
	tr << (BYTE)0x68 << (DWORD)0x86A80A;
	tr << RET; // ret

	return &tr;

}

static void __stdcall mbuttonup_hook(DWORD wParam) {
	
	
}

static const trampoline_t *prepare_mbuttonup_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mbuttonup patch...\n");

	tr << PUSHAD; // PUSHAD
	tr << (BYTE)0x56; // push esi (contains WndProc/wParam)
	tr.append_CALL((DWORD)mbuttonup_hook);
	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET; 

	return &tr;
}

static const trampoline_t *prepare_pylpyr_patch(patch_t *p) {

 // THE SELECTION RING DRAWING HAPPENS AT 7E41FB!!
 // 4F6F90 could be of interest :>

 // A BREAKPOINT AT 0x4F6FAB is the unique code path for pylpyr (fires ONLY if an unit is selected)
 // 725980 is some member function probably to draw a pylpyr under an unit / npc :D


	static trampoline_t tr;

	PRINT("Preparing pylpyr patch...\n");

	tr << PUSHAD;
	tr.append_CALL((DWORD)call_pylpyr);

	tr << POPAD; // POPAD
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET;

	return &tr;

}

static void __stdcall mwheel_hook(DWORD wParam) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (zDelta < 0) customcamera.increment_s();
	if (zDelta > 0) customcamera.decrement_s();
}

static void __stdcall SARC4_hook(uint* args) {
	// the arguments are at args[0:3]
	printf("arg1: %X, arg2: %X, arg3: %X, arg4: %X\n", args[0], args[1], args[2], args[3]);

}

static const trampoline_t *prepare_mwheel_patch(patch_t *p) {
	static trampoline_t tr;

	PRINT("Preparing mwheel patch...\n");

	tr << PUSHAD;
	tr << (BYTE)0x8B << (BYTE)0x45 << (BYTE)0x10; // move wParam to EAX
	tr << (BYTE)0x50; // push eax
	tr.append_CALL((DWORD)mwheel_hook);

	tr << POPAD; // POPAD
	tr << (BYTE)0x68 << (DWORD)0x86A8D1; // skip right to return
	tr << RET;

	return &tr;

}

typedef struct {
	DWORD action;
	GUID_t *GUID;
	float *coords;
	DWORD s2;
} CTM_final_args_t;

int __stdcall CTM_main_hook(CTM_final_args_t *a) {
	PRINT("CTM: action %X, s1: %X, coords: %X, s2: %X\n", a->action, a->GUID, (DWORD)a->coords, a->s2);
	
	if (!wc3mode_enabled()) return 1;

	if (a->action == CTM_MOVE) {
		float *c = a->coords;
		DoString("RunMacroText(\"/lole broadcast ctm %.3f %.3f %.3f\")", c[0], c[1], c[2]);
		return 0;
	}
	else if (a->action == CTM_MOVE_AND_ATTACK) {
		ctm_queue_reset();
		stopfollow();
		DoString("RunMacroText(\"/lole broadcast attack 0x%16llX\")", *a->GUID);
		return 0;
	}
	else return 1;
}

static const trampoline_t *prepare_CTM_main_patch(patch_t *p) {
	static trampoline_t tr;
	
	tr << PUSHAD;
	tr.append_hexstring("8D54242452"); // lea edx, [esp + 24]; push edx; because LOLZ
	tr.append_CALL((DWORD)CTM_main_hook);
	tr.append_hexstring("83F800"); // cmp eax, 0
	tr.append_hexstring("0F840D000000"); // jz branch 2
	tr << POPAD;

	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size);
	tr << RET;

	// branch 2
	tr << POPAD;
	tr.append_hexstring("C21000"); // retn 10

	return &tr;
}



static const trampoline_t *prepare_AddInputEvent_patch(patch_t *p) {

	static trampoline_t tr;

	tr.append_hexstring("608D5C242453"); // pushad; lea ebx, [esp + 0x24]; push ebx
	tr.append_CALL((DWORD)AddInputEvent_hook);
	tr.append_hexstring("83F800"); // cmp eax, 0
	tr.append_hexstring("0F840F000000"); // jz branch 2

	tr << POPAD;
	tr.append_bytes(p->original, p->size);
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size);
	tr << RET;

	// branch 2

	tr << POPAD;
	tr << RET; // straight ret if we want to filter this shit B)

	return &tr;
}

static const trampoline_t *prepare_SARC4_patch(patch_t *p) {
	static trampoline_t tr;
	tr << PUSHAD;
	tr.append_hexstring("8D5C242453"); // lea ebx, [esp + 0x24]; push ebx (to get all the four arguments)
	tr.append_CALL((DWORD)SARC4_hook);
	tr << POPAD;
	tr.append_bytes(p->original, p->size);
	
	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET;

	return &tr;
}

static const trampoline_t *prepare_WS2send_patch(patch_t *p) {
	static trampoline_t tr;
	tr << PUSHAD;
	tr.append_hexstring("8D5C242453"); // lea ebx, [esp + 0x24]; push ebx (to get all the four arguments)
	tr.append_CALL((DWORD)dump_WS2send);
	tr << POPAD;
	tr.append_bytes(p->original, p->size);

	tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	tr << RET;

	return &tr;
}

static const trampoline_t *prepare_WS2recv_patch(patch_t *p) {
	static trampoline_t tr;
	//tr.append_bytes(p->original, p->size);
	tr.append_hexstring("8BE55D"); // MOV ESP, EBP; POP EBP (from the original func)
	tr << PUSHAD;
	tr << (BYTE)0x50;
	tr.append_hexstring("8D5C242853"); // lea ebx, [esp + 0x28]; push ebx (to get all the four arguments)
	tr.append_CALL((DWORD)dump_WS2recv);
	tr << POPAD;

	tr.append_hexstring("C21000"); //retn 10

	//tr << (BYTE)0x68 << (DWORD)(p->patch_addr + p->size); // push RET addr
	//tr << RET;


	return &tr;
}

static hookable_t hookable_functions[] = {
	//{ "EndScene", 0x0, EndScene_original, EndScene_patch, EndScene_original, prepare_EndScene_patch },
	//{ "ClosePetStables", (LPVOID)ClosePetStables, ClosePetStables_original, ClosePetStables_patch, ClosePetStables_original, prepare_ClosePetStables_patch },
	//{ "CTM_update", (LPVOID)CTM_update_hookaddr, CTM_finished_original, CTM_finished_patch, CTM_finished_original, prepare_CTM_finished_patch },
	//{ "SpellErrMsg", (LPVOID)SpellErrMsg, spell_errmsg_original, spell_errmsg_patch, spell_errmsg_original, prepare_spell_errmsg_patch },
	//{ "SendPacket", (LPVOID)SendPacket_hookaddr, sendpacket_original, sendpacket_patch, sendpacket_original, prepare_sendpacket_patch },
	//{ "RecvPacket", (LPVOID)RecvPacket_hookaddr, recvpacket_original, recvpacket_patch, recvpacket_original, prepare_recvpacket_patch },
	//{ "Present", (LPVOID)0x0, present_original, present_patch, present_original, prepare_present_patch },
	//{ "mbuttondown_handler", (LPVOID)mbuttondown_handler, mbuttondown_original, mbuttondown_patch, mbuttondown_patch, prepare_mbuttondown_patch },
	//{ "mbuttonup_handler", (LPVOID)mbuttonup_handler, mbuttonup_original, mbuttonup_patch, mbuttonup_patch, prepare_mbuttonup_patch },
	//{ "DrawIndexedPrimitive", 0x0, drawindexedprimitive_original, drawindexedprimitive_patch, drawindexedprimitive_patch, prepare_drawindexedprimitive_patch },

	{ "EndScene", patch_t(PATCHADDR_LATER, 7, prepare_EndScene_patch) },
	{ "Present", patch_t(PATCHADDR_LATER, 5, prepare_Present_patch) },
	{ "CTM_finished", patch_t(CTM_finished_patchaddr, 10, prepare_CTM_finished_patch) },
	{ "ClosePetStables", patch_t(ClosePetStables_patchaddr, 5, prepare_ClosePetStables_patch) },
	{ "mbuttondown_handler", patch_t(mbuttondown_handler, 6, prepare_mbuttondown_patch) },
	{ "mbuttonup_handler", patch_t(mbuttonup_handler, 7, prepare_mbuttonup_patch) },
	{ "DrawIndexedPrimitive", patch_t(PATCHADDR_LATER, 5, prepare_DrawIndexedPrimitive_patch) },
	{ "pylpyr", patch_t(pylpyr_patchaddr, 9, prepare_pylpyr_patch) },
	{ "mwheel_handler", patch_t(mwheel_hookaddr, 6, prepare_mwheel_patch) },
	{ "CTM_main", patch_t(CTM_main_hookaddr, 6, prepare_CTM_main_patch)},
	{ "AddInputEvent", patch_t(AddInputEvent, 8, prepare_AddInputEvent_patch) },
	{ "SendPacket", patch_t(SendPacket_hookaddr, 5, prepare_sendpacket_patch) },
	{ "RecvPacket", patch_t(RecvPacket_hookaddr, 6, prepare_recvpacket_patch) },
	{ "SARC4_encrypt", patch_t(SARC4_encrypt, 6, prepare_SARC4_patch)},
	{ "WS2_send", patch_t((DWORD)&send, 5, prepare_WS2send_patch)}, 
	{ "WS2_recv", patch_t(((DWORD)&recv) + 0x18A, 6, prepare_WS2recv_patch) },

};

hookable_t *find_hookable(const std::string &funcname) {
	for (auto &h : hookable_functions) {
		if (h.funcname == funcname) {
			return &h;
		}
	}
	return NULL;
}

static int get_patch_from_hookable(const std::string &funcname, patch_serialized *p) {
	hookable_t *hook = find_hookable(funcname);
	if (!hook) {
		PRINT("get_patch_from_hookable: find_hookable for %s failed, expecting a disaster.\n", funcname.c_str());
		return 0;
	}

	*p = patch_serialized(hook->patch.patch_addr, hook->patch.size, hook->patch.original, hook->patch.patch);
	return 1;
}

int prepare_pipe_data() {
	
	patch_serialized p;

#define ADD_PATCH_SAFE(patchname) do { int r = get_patch_from_hookable(patchname, &p); assert(r); PIPEDATA.add_patch(p); ENABLED_PATCHES.push_back(patchname); } while(0)

	ADD_PATCH_SAFE("EndScene");
	ADD_PATCH_SAFE("Present");
	ADD_PATCH_SAFE("CTM_finished");
	ADD_PATCH_SAFE("ClosePetStables");
	ADD_PATCH_SAFE("pylpyr");
	ADD_PATCH_SAFE("CTM_main");
	ADD_PATCH_SAFE("AddInputEvent");
//	ADD_PATCH_SAFE("SendPacket");
	//ADD_PATCH_SAFE("RecvPacket");
	//ADD_PATCH_SAFE("SARC4_encrypt");
//	ADD_PATCH_SAFE("WS2_send");
//	ADD_PATCH_SAFE("WS2_recv");

	// don't add DrawIndexedPrimitive to this list, it will be patched manually later by a /lole subcommand
	//ADD_PATCH_SAFE("DrawIndexedPrimitive");

	return 1;
}


patch_serialized::patch_serialized(UINT32 patch_addr, UINT32 patch_size, const BYTE *original_opcodes, const BYTE *patch_opcodes) {
	buffer_size = 2 * sizeof(UINT32) + 2 * patch_size;
	
	buffer = new BYTE[buffer_size];

	memcpy(buffer + 0 * sizeof(UINT32), &patch_addr, sizeof(UINT32));
	memcpy(buffer + 1 * sizeof(UINT32), &patch_size, sizeof(UINT32));
	memcpy(buffer + 2 * sizeof(UINT32), original_opcodes, patch_size);
	memcpy(buffer + 2 * sizeof(UINT32) + patch_size, patch_opcodes, patch_size);


}

int pipe_data::add_patch(const patch_serialized &p) {
	data.insert(data.end(), &p.buffer[0], &p.buffer[p.buffer_size]);
	++num_patches;
	memcpy(&data[4], &num_patches, sizeof(UINT32));

	return num_patches;
}

int unpatch_all() {
	for (auto &p : ENABLED_PATCHES) {
		hookable_t *h = find_hookable(p);
		WriteProcessMemory(glhProcess, (LPVOID)h->patch.patch_addr, h->patch.original, h->patch.size, NULL);
		PRINT("Unpatched %s at location %X, size %d\n", p.c_str(), h->patch.patch_addr, h->patch.size);
	}
	return 1;
}
