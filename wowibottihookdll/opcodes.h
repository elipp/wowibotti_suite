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

enum class LOP : int {
	NOP = 0x0,
	TARGET_GUID,
	CASTER_RANGE_CHECK,
	FOLLOW,
	CTM,
	DUNGEON_SCRIPT,
	TARGET_MARKER,
	MELEE_BEHIND,
	AVOID_SPELL_OBJECT,
	HUG_SPELL_OBJECT,
	SPREAD,
	CHAIN_HEAL_TARGET,
	MELEE_AVOID_AOE_BUFF,
	TANK_FACE,
	TANK_PULL,
	GET_UNIT_POSITION,
	GET_WALKING_STATE,
	GET_CTM_STATE,
	GET_PREVIOUS_CAST_MSG,
	STOPFOLLOW,
	CAST_GTAOE,
	HAS_AGGRO,
	INTERACT_GOBJECT,
	GET_BISCUITS,
	LOOT_BADGE,
	LUA_UNLOCK,
	LUA_LOCK,
	EXECUTE,
	GET_COMBAT_TARGETS,
	GET_AOE_FEASIBILITY,
	AVOID_NPC_WITH_NAME,
	BOSS_ACTION,
	INTERACT_SPELLNPC,
	GET_LAST_SPELL_ERRMSG,
	ICCROCKET,
	HCONFIG,
	TANK_TAUNT_LOOSE,
	READ_FILE,
	NUM_OPCODES,
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
	 LDOP_CONSOLE_PRINT,
	 LDOP_REPORT_CONNECTED,
	 LDOP_EJECT_DLL,

	 LDOP_DUMMY_LAST,
	 LDOP_NUM_OPCODES = (LDOP_DUMMY_LAST - LDOP_NOP)
};

void refollow_if_needed();
void stopfollow();

int lop_exec(lua_State *L);

typedef struct cast_msg_t {
	int msg;
	LONG timestamp;
} cast_msg_t;

extern struct cast_msg_t previous_cast_msg;

extern Timer since_noclip;
extern int noclip_enabled;

void target_unit_with_GUID(GUID_t GUID);

enum INTERACT_TYPES {
	INTERACT_GAMEOBJECT = 1,
	INTERACT_NPC = 2,
};