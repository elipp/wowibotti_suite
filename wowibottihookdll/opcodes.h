#pragma once

#include <string>
#include <unordered_map>
#include <math.h>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"
#include "lua.h"

// lole opcodes. also defined in lole.lua (let's just hope they match XD)

enum LOP_OPCODES {
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
	LOP_GET_WALKING_STATE,
	LOP_GET_CTM_STATE,
	LOP_GET_PREVIOUS_CAST_MSG,
	LOP_STOPFOLLOW,
	LOP_CAST_GTAOE,
	LOP_HAS_AGGRO,
	
	LOP_DUMMY_LAST,
	LOP_NUM_OPCODES = (LOP_DUMMY_LAST - LOP_NOP)
};

enum LOP_EXT_OPCODES {
	LOP_EXT_NOP = 0x70,
	LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND,
	
	LOP_EXT_DUMMY_LAST,
	LOP_EXT_NUM_OPCODES = (LOP_EXT_DUMMY_LAST - LOP_EXT_NOP)
};

enum LDOP_OPCODES {
 LDOP_NOP = 0xE0,
 LDOP_DUMP,
 LDOP_LOOT_ALL,
 LDOP_PULL_TEST,
 LDOP_LUA_REGISTERED,
 LDOP_LOS_TEST,

 LDOP_DUMMY_LAST,
 LDOP_NUM_OPCODES = (LDOP_DUMMY_LAST - LDOP_NOP)
};

void refollow_if_needed();

int lop_exec(lua_State *s);

extern struct cast_msg_t {
	int msg;
	LONG timestamp;
} previous_cast_msg;

