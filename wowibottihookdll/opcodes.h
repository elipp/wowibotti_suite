#pragma once

#include <string>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"

typedef void(*hubfunc_t)(const std::string &);

// lole opcodes. also defined in lole.lua (let's just hope they match XD)
// turns this shit into a nice jump table

#define LOLE_OPCODE_NOP 0x0 
#define LOLE_OPCODE_TARGET_GUID 0x1
#define LOLE_OPCODE_BLAST 0x2
#define LOLE_OPCODE_CASTER_RANGE_CHECK 0x3
#define LOLE_OPCODE_GATHER_FOLLOW 0x4
#define LOLE_OPCODE_FACE 0x5
#define LOLE_OPCODE_CTM_BROADCAST 0x6
#define LOLE_OPCODE_COOLDOWNS 0x7
#define LOLE_OPCODE_CC 0x8
#define LOLE_OPCODE_DUNGEON_SCRIPT 0x9
#define LOLE_OPCODE_TARGET_MARKER 0xA
#define LOLE_OPCODE_DRINK 0xB
#define LOLE_OPCODE_MELEE_BEHIND 0xC

#define LOLE_DEBUG_OPCODE_DUMP 0xF1

int opcode_call(int opcode, const std::string &arg);
int opcode_debug_call(int debug_opcode, const std::string &arg);
const std::string &opcode_get_funcname(int opcode);
int opcode_get_num_args(int opcode);

void refollow_if_needed();