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
	GET_PREVIOUS_CAST_MSG, // TODO: merge with GET_LAST_SPELL_ERRMSG (or replace with)
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

#define LOP_EXT_MASK 0x40000000

enum class LOP_EXT : int {
	NOP = 0x00 | LOP_EXT_MASK,
	SL_RESETCAMERA = 0x1 | LOP_EXT_MASK,
	WC3MODE = 0x2 | LOP_EXT_MASK,
	SL_SETSELECT = 0x3 | LOP_EXT_MASK,
};

#define LDOP_MASK 0x80000000

enum class LDOP : int {
	 NOP = 0x00 | LDOP_MASK,
	 DUMP = 0x01 | LDOP_MASK,
	 LOOT_ALL = 0x02 | LDOP_MASK,
	 PULL_TEST = 0x03 | LDOP_MASK,
	 LUA_REGISTERED = 0x04 | LDOP_MASK,
	 LOS_TEST = 0x05 | LDOP_MASK,
	 NOCLIP = 0x06 | LDOP_MASK,
	 TEST = 0x07 | LDOP_MASK,
	 CAPTURE_FRAME_RENDER_STAGES = 0x08 | LDOP_MASK,
	 CONSOLE_PRINT = 0x09 | LDOP_MASK,
	 REPORT_CONNECTED = 0x0A | LDOP_MASK,
	 EJECT_DLL = 0x0B | LDOP_MASK,
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