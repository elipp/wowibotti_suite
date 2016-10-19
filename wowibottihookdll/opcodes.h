#pragma once

#include <string>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"
#include "lua.h"

// lole opcodes. also defined in lole.lua (let's just hope they match XD)

enum {
	LOP_NOP,
	LOP_TARGET_GUID,
	LOP_CASTER_RANGE_CHECK,
	LOP_FOLLOW,
	LOP_CTM,
	LOP_DUNGEON_SCRIPT,
	LOP_TARGET_MARKER,
	LOP_MELEE_BEHIND,
	LOP_AVOID_SPELL_OBJECT,
	LOP_HUG_SPELL_OBJECT,
	LOP_SPREAD,
	LOP_CHAIN_HEAL_TARGET,
	LOP_MELEE_AVOID_AOE_BUFF,
	LOP_TANK_FACE,
	LOP_WALK_TO_PULLING_RANGE,
	LOP_GET_UNIT_POSITION,
	LOP_NUM_OPCODES
};

#define LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND 0x70

#define LDOP_NOP 0xE0
#define LDOP_DUMP 0xE1
#define LDOP_LOOT_ALL 0xE2
#define LDOP_QUERY_INJECTED 0xE3
#define LDOP_PULL_TEST 0xE4
#define LDOP_LUA_REGISTERED 0xE5

void refollow_if_needed();

int lop_exec(lua_State *s);