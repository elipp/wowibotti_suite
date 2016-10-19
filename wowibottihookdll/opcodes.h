#pragma once

#include <string>
#include <unordered_map>
#include <math.h>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"
#include "lua.h"

// lole opcodes. also defined in lole.lua (let's just hope they match XD)

#define LOP_NOP 0x0
#define LOP_TARGET_GUID 0x1
#define LOP_CASTER_RANGE_CHECK 0x2
#define LOP_FOLLOW 0x3
#define LOP_CTM 0x4
#define LOP_DUNGEON_SCRIPT 0x5
#define LOP_TARGET_MARKER 0x6
#define LOP_MELEE_BEHIND 0x7
#define LOP_AVOID_SPELL_OBJECT 0x8
#define LOP_HUG_SPELL_OBJECT 0x9
#define LOP_SPREAD 0xA
#define LOP_CHAIN_HEAL_TARGET 0xB
#define LOP_MELEE_AVOID_AOE_BUFF 0xC
#define LOP_TANK_FACE 0xD
#define LOP_WALK_TO_PULLING_RANGE 0xE

#define LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND 0x70

#define LDOP_NOP 0xE0
#define LDOP_DUMP 0xE1
#define LDOP_LOOT_ALL 0xE2
#define LDOP_QUERY_INJECTED 0xE3
#define LDOP_PULL_TEST 0xE4
#define LDOP_LUA_REGISTERED 0xE5

void refollow_if_needed();

int lop_exec(lua_State *s);