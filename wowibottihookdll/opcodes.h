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

#define LDOP_MASK 0x800

enum class LOP : int {
	NOP = 0x0,
	TARGET_GUID,
	CASTER_RANGE_CHECK,
	FOLLOW,
	CLICK_TO_MOVE,
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
	
	LDOP_NOP = LDOP_MASK,
	LDOP_DEBUG_TEST,
	LDOP_DUMP,
	LDOP_LOOT_ALL,
	LDOP_PULL_TEST,
	LDOP_LUA_REGISTER,
	LDOP_LOS_TEST,
	LDOP_CAPTURE_FRAME_RENDER_STAGES,
	LDOP_CONSOLE_PRINT,
	LDOP_REPORT_CONNECTED,
	LDOP_EJECT_DLL,

	// from the old "LOP_EXT"

	LDOP_SL_RESETCAMERA,
	LDOP_WC3MODE,
	LDOP_SL_SETSELECT,
};

inline constexpr bool IS_DEBUG_OPCODE(LOP op) {
	return ((int)op & LDOP_MASK);
}

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