local LOP_NOP = 0x0
local LOP_TARGET_GUID = 0x1
local LOP_CASTER_RANGE_CHECK = 0x2
local LOP_FOLLOW = 0x3  -- this also includes walking to/towards the target
local LOP_CTM = 0x4
local LOP_DUNGEON_SCRIPT = 0x5
local LOP_TARGET_MARKER = 0x6
local LOP_MELEE_BEHIND = 0x7
local LOP_AVOID_SPELL_OBJECT = 0x8
local LOP_HUG_SPELL_OBJECT = 0x9
local LOP_SPREAD = 0xA
local LOP_CHAIN_HEAL_TARGET = 0xB
local LOP_MELEE_AVOID_AOE_BUFF = 0xC
local LOP_TANK_FACE = 0xD
local LOP_WALK_TO_PULLING_RANGE = 0xE
local LOP_GET_UNIT_POSITION = 0xF
local LOP_GET_WALKING_STATE = 0x10
local LOP_GET_CTM_STATE = 0x11
local LOP_GET_PREVIOUS_CAST_MSG = 0x12
local LOP_STOPFOLLOW = 0x13

local LOP_EXT_NOP = 0x70
local LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND = 0x71

local LDOP_NOP = 0xE0
local LDOP_DUMP = 0xE1
local LDOP_LOOT_ALL = 0xE2
local LDOP_PULL_TEST = 0xE3
local LDOP_LUA_REGISTERED = 0xE4
local LDOP_LOS_TEST = 0xE5


----------------------------
---- public functions ------
----------------------------

function enable_cc_target(name, marker, spell)
	lole_subcommands.sendmacro_to(name, "/lole cc enable", marker, spell)
end

function disable_cc_target(name, marker)
	lole_subcommands.sendmacro_to(name, "/lole cc disable", marker)
end

function disable_all_cc_targets()
	lole_subcommands.sendmacro_to(name, "/lole cc clear")
end

function target_unit_with_marker(marker)
	lop_exec(LOP_TARGET_MARKER, marker)
end

function caster_range_check(minrange)
	if lole_subcommands.get("playermode") == 0 then
		if lop_exec(LOP_CASTER_RANGE_CHECK, minrange) then
			return true
		else
			return false
		end
	end
end

function target_unit_with_GUID(GUID)
	lop_exec(LOP_TARGET_GUID, GUID)
end

function melee_attack_behind()
	if lole_subcommands.get("playermode") == 0 then
		lop_exec(LOP_MELEE_BEHIND);
	end
end

function melee_avoid_aoe_buff(buff_spellID)
	if lole_subcommands.get("playermode") == 0 then
		lop_exec(LOP_MELEE_AVOID_AOE_BUFF, buff_spellID);
	end

end

function hug_spell_with_spellID(spellID)
	if (lole_subcommands.get("playermode") == 0) then
		lop_exec(LOP_HUG_SPELL_OBJECT, spellID)
	end
end

function avoid_spell_with_spellID(spellID, radius)
	if (lole_subcommands.get("playermode") == 0) then
		lop_exec(LOP_AVOID_SPELL_OBJECT, spellID, radius)
	end
end

function pull_target()
	CastSpellByName("Avenger Shield")
end

function walk_to_pulling_range()
	-- assuming the pull target is already selected
	lop_exec(LOP_WALK_TO_PULLING_RANGE)
end

function tank_face()
	lop_exec(LOP_TANK_FACE)
end

function get_CH_target_trio(heals_in_progress)
	local t = lop_exec(LOP_CHAIN_HEAL_TARGET, heals_in_progress);
	return t;
end

function warlock_maulgar_get_felhound()
	local t = lop_exec(LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND)
	return t;
end

function dscript(command, scriptname)
	lop_exec(LOP_DUNGEON_SCRIPT, command, scriptname);
end

function get_unit_position(unitname)
	return lop_exec(LOP_GET_UNIT_POSITION, unitname)
end

function lole_debug_dump_wowobjects()
	lop_exec(LDOP_DUMP);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log (if you're injected!) ;)")
	return true;
end

function lole_debug_loot_all()
	lop_exec(LDOP_LOOT_ALL);
end

function walk_to(x, y, z)
	lop_exec(LOP_CTM, x, y, z)
end

function follow_unit(name)
	lop_exec(LOP_FOLLOW, name)
end

function stopfollow()
	lop_exec(LOP_STOPFOLLOW)
end

local INJECTED_STATUS = 0

function query_injected()
	if INJECTED_STATUS == 1 then return 1 end

	local lexec = _G.lop_exec; -- see if the function is registered
	if not lexec then
		return 0;
	else
		lop_exec(LDOP_LUA_REGISTERED);
		INJECTED_STATUS = 1;
		return 1;
	end

end

function lole_debug_pull_test()
	lop_exec(LDOP_PULL_TEST)
end
