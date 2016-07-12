#pragma once

#include <string>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"

typedef void(*hubfunc_t)(const std::string &);

// lole opcodes. also defined in lole.lua (let's just hope they match XD)
// turns this shit into a nice jump table

#define LOLE_OPCODE_NOP					0x0 
#define LOLE_OPCODE_TARGET_GUID			0x1
#define LOLE_OPCODE_BLAST				0x2
#define LOLE_OPCODE_CASTER_RANGE_CHECK	0x3
#define LOLE_OPCODE_GATHER_FOLLOW		0x4
#define LOLE_OPCODE_FACE				0x5
#define LOLE_OPCODE_CTM_BROADCAST		0x6
#define LOLE_OPCODE_COOLDOWNS			0x7
#define LOLE_OPCODE_CC					0x8
#define LOLE_OPCODE_DUNGEON_SCRIPT		0x9
#define LOLE_OPCODE_TARGET_MARKER		0xA
#define LOLE_OPCODE_DRINK				0xB
#define LOLE_OPCODE_MELEE_BEHIND		0xC
#define LOLE_OPCODE_LEAVE_PARTY			0xD
#define LOLE_OPCODE_AFK_JUMP			0xE
#define LOLE_OPCODE_RELEASE_SPIRIT		0xF
#define LOLE_OPCODE_MAIN_TANK			0x10
#define LOLE_OPCODE_AVOID_SPELL_OBJECT	0x11
#define LOLE_OPCODE_HUG_SPELL_OBJECT	0x12
#define LOLE_OPCODE_SPREAD				0x13
#define LOLE_OPCODE_PULL_MOB			0x14
#define LOLE_OPCODE_REPORT_LOGIN		0x15

#define LOLE_DEBUG_OPCODE_NOP			0x80
#define LOLE_DEBUG_OPCODE_DUMP			0x81
#define LOLE_DEBUG_OPCODE_LOOT_ALL		0x82
#define LOLE_DEBUG_OPCODE_QUERY_INJECTED 0x83

int opcode_call(int opcode, const std::string &arg);
int opcode_debug_call(int debug_opcode, const std::string &arg);
const std::string &opcode_get_funcname(int opcode);
const std::string &debug_opcode_get_funcname(int opcode_unmasked);

int opcode_get_num_args(int opcode);

void refollow_if_needed();