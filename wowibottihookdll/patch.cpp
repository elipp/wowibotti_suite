#include "patch.h"

// NOP for padding is a lot better, since the debugger gets fucked up with 0x00s

//const BYTE LUA_prot_original[] = {
//	// we're just making an early exit, so nevermind opcode boundaries
//	0x55,
//	0x8B, 0xEC,
//	0x83, 0x3D, 0x40 // this one is cut in the middle
//};
//
//BYTE LUA_prot_patch[] = {
//	0xB8, 0x01, 0x00, 0x00, 0x00, // MOV EAX, 1
//	0xC3						  // RET
//};
//
//size_t LUA_prot_patchsize = sizeof(LUA_prot_patch) / sizeof(LUA_prot_patch[0]);
//
//const BYTE EndScene_original[] = {
//	0x6A, 0x20, // push 20
//	0xB8, 0xD8, 0xB9, 0x08, 0x6C // MOV EAX, 6C08B9D8 after this should be mu genitals :D
//};
//
//BYTE EndScene_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
//	0x90, 0x90
//};
//
//const size_t LUA_prot_patchsize = sizeof(LUA_prot_patch) / sizeof(LUA_prot_patch[0]);
//
//
//const BYTE DelIgnore_original[] = {
//	0x6A, 0x01, // push 1
//	0x6A, 0x01, // push 1
//	0x8D, 0x4D, 0xFC, // LEA ECX, [LOCAL.1]
//};
//
//BYTE DelIgnore_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, // jmp address to be inserted to bytes 1-5
//	0x90, 0x90
//};
//
//const size_t DelIgnore_patchsize = sizeof(DelIgnore_patch) / sizeof(DelIgnore_patch[0]);
//
//const BYTE ClosePetStables_original[] = {
//	0xE8, 0x3B, 0x1E, 0xF3, 0xFF // CALL 004FAC60. this wouldn't actually even need a trampoline lol
//};
//
//BYTE ClosePetStables_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00
//};
//
//const size_t ClosePetStables_patchsize = sizeof(ClosePetStables_patch) / sizeof(ClosePetStables_patch[0]);
//
//
//const BYTE CTM_main_original[] = {
//	0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18
//};
//
//BYTE CTM_main_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90
//};
//
//const size_t CTM_main_patchsize = sizeof(CTM_main_patch) / sizeof(CTM_main_patch[0]);
//
//
//const BYTE CTM_aux_original[] = {
//	0x55, 0x8B, 0xEC, 0x8B, 0x41, 0x38
//};
//
//// this is a patch for func 0x7B8940
//BYTE CTM_aux_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90
//};
//
//const size_t CTM_aux_patchsize = sizeof(CTM_aux_patch) / sizeof(CTM_aux_patch[0]);
//
//
//const BYTE CTM_finished_original[] = {
//	0xC7, 0x05, 0xBC, 0x89, 0xD6, 0x00, 0x0D, 0x00, 0x00, 0x00
//};
//
//BYTE CTM_finished_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90
//};
//
//const size_t CTM_finished_patchsize = sizeof(CTM_finished_patch) / sizeof(CTM_finished_patch[0]);
//
//const BYTE spell_errmsg_original[] = {
//	0x0F, 0xB6, 0xC0, 0x3D, 0xA8, 0x00, 0x00, 0x00
//};
//
//BYTE spell_errmsg_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90
//};
//
//const size_t spell_errmsg_patchsize = sizeof(spell_errmsg_patch) / sizeof(spell_errmsg_patch[0]);
//
//const BYTE sendpacket_original[] = {
//	0xE8, 0x3D, 0xEE, 0xFF, 0xFF
//};
//
//BYTE sendpacket_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00
//};
//
//const size_t sendpacket_patchsize = sizeof(sendpacket_patch) / sizeof(sendpacket_patch[0]);
//
//const BYTE recvpacket_original[] = {
//	0x01, 0x5E, 0x20, 0x8B, 0x4D, 0xF8,
//};
//
//BYTE recvpacket_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00, 0x90
//};
//
//const size_t recvpacket_patchsize = sizeof(recvpacket_patch) / sizeof(recvpacket_patch[0]);
//
//const BYTE present_original[] = {
//	0x8B, 0xFF, 0x55, 0x8B, 0xEC
//};
//
//BYTE present_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00
//};
//
//const size_t present_patchsize = sizeof(present_patch) / sizeof(present_patch[0]);
//
//
//const BYTE mbuttondown_original[] = {
//	0x55, 0x8B, 0xEC, 0x51, 0x56
//};
//
//BYTE mbuttondown_patch[] = {
//	0xE9, 0x90, 0x90, 0x90, 0x90,
//};
//
//const size_t mbuttondown_patchsize = sizeof(mbuttondown_patch) / sizeof(mbuttondown_patch[0]);
//
//
//const BYTE mbuttonup_original[] = {
//	0x55, 0x8B, 0xEC, 0x51, 0x56,
//
//};
//
//BYTE mbuttonup_patch[] = {
//	0xE9, 0x90, 0x90, 0x90, 0x90,
//};
//
//const size_t mbuttonup_patchsize = sizeof(mbuttonup_patch) / sizeof(mbuttonup_patch[0]);
//
//
//const BYTE drawindexedprimitive_original[] = {
//	0x8B, 0xFF, 0x55, 0x8B, 0xEC,
//};
//
//BYTE drawindexedprimitive_patch[] = {
//	0xE9, 0x00, 0x00, 0x00, 0x00
//};
//
//const size_t drawindexedprimitive_patchsize = sizeof(drawindexedprimitive_patch) / sizeof(drawindexedprimitive_patch[0]);
