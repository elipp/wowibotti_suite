#pragma once

#include <string>
#include <unordered_map>
#include <math.h>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"

typedef void(*hubfunc_t)(const std::string &);

// lole opcodes. also defined in lole.lua (let's just hope they match XD)
// turns this shit into a nice jump table

#define LOLE_OPCODE_NOP								0x00 
#define LOLE_OPCODE_TARGET_GUID						0x01
#define LOLE_OPCODE_BLAST							0x02
#define LOLE_OPCODE_CASTER_RANGE_CHECK				0x03
#define LOLE_OPCODE_GATHER_FOLLOW					0x04
#define LOLE_OPCODE_FACE							0x05
#define LOLE_OPCODE_CTM_BROADCAST					0x06
#define LOLE_OPCODE_COOLDOWNS						0x07
#define LOLE_OPCODE_CC								0x08
#define LOLE_OPCODE_DUNGEON_SCRIPT					0x09
#define LOLE_OPCODE_TARGET_MARKER					0x0A
#define LOLE_OPCODE_DRINK							0x0B
#define LOLE_OPCODE_MELEE_BEHIND					0x0C
#define LOLE_OPCODE_LEAVE_PARTY						0x0D
#define LOLE_OPCODE_AFK_JUMP						0x0E
#define LOLE_OPCODE_RELEASE_SPIRIT					0x0F
#define LOLE_OPCODE_MAIN_TANK						0x10
#define LOLE_OPCODE_AVOID_SPELL_OBJECT				0x11
#define LOLE_OPCODE_HUG_SPELL_OBJECT				0x12
#define LOLE_OPCODE_SPREAD							0x13
#define LOLE_OPCODE_PULL_MOB						0x14
#define LOLE_OPCODE_REPORT_LOGIN					0x15
#define LOLE_OPCODE_WALK_TO_PULLING_RANGE			0x16
#define LOLE_OPCODE_GET_BEST_CHAINHEAL_TARGET		0x17
#define LOLE_OPCODE_MAULGAR_GET_UNBANISHED_FELHOUND 0x18
#define LOLE_OPCODE_OFF_TANK						0x19
#define LOLE_OPCODE_SET_ALL							0x1A
#define LOLE_OPCODE_MELEE_AVOID_AOE_BUFF			0x1B

#define LOLE_DEBUG_OPCODE_NOP						0x80
#define LOLE_DEBUG_OPCODE_DUMP						0x81
#define LOLE_DEBUG_OPCODE_LOOT_ALL					0x82
#define LOLE_DEBUG_OPCODE_QUERY_INJECTED			0x83
#define LOLE_DEBUG_OPCODE_PULL_TEST					0x84

int opcode_call(int opcode, const std::string &arg);
int opcode_debug_call(int debug_opcode, const std::string &arg);
const std::string &opcode_get_funcname(int opcode);
const std::string &debug_opcode_get_funcname(int opcode_unmasked);

int opcode_get_num_args(int opcode);

void refollow_if_needed();