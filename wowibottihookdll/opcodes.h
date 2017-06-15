#pragma once

#include <string>
#include <unordered_map>
#include <math.h>
#include "defs.h"
#include "wowmem.h"
#include "addrs.h"
#include "lua.h"
#include "timer.h"

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
	LOP_INTERACT_GOBJECT,
	LOP_GET_BISCUITS,
	LOP_LOOT_BADGE,
	LOP_LUA_UNLOCK,
	LOP_LUA_LOCK,
	LOP_EXECUTE,
	LOP_FOCUS,
	LOP_CAST_SPELL,
	LOP_GET_COMBAT_TARGETS,

	LOP_DUMMY_LAST,
	LOP_NUM_OPCODES = (LOP_DUMMY_LAST - LOP_NOP)
};

enum LOP_EXT_OPCODES {
	LOP_EXT_NOP = 0x70,
	LOP_SL_RESETCAMERA = 0x71,
	LOP_WC3MODE = 0x72,
	LOP_SL_SETSELECT = 0x73,

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
 LDOP_NOCLIP,
 LDOP_TEST,
 LDOP_CAPTURE_FRAME_RENDER_STAGES,

 LDOP_DUMMY_LAST,
 LDOP_NUM_OPCODES = (LDOP_DUMMY_LAST - LDOP_NOP)
};

void refollow_if_needed();
void stopfollow();

int lop_exec(lua_State *s);

typedef struct cast_msg_t {
	int msg;
	LONG timestamp;
} cast_msg_t;

extern struct cast_msg_t previous_cast_msg;

extern Timer since_noclip;
extern int noclip_enabled;

void enable_noclip();
void disable_noclip();
