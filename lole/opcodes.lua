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
local LOP_CAST_GTAOE = 0x14
local LOP_HAS_AGGRO = 0x15
local LOP_INTERACT_GOBJECT = 0x16
local LOP_GET_BISCUITS = 0x17

local LOP_EXT_NOP = 0x70
local LOP_EXT_MAULGAR_GET_UNBANISHED_FELHOUND = 0x71

local LDOP_NOP = 0xE0
local LDOP_DUMP = 0xE1
local LDOP_LOOT_ALL = 0xE2
local LDOP_PULL_TEST = 0xE3
local LDOP_LUA_REGISTERED = 0xE4
local LDOP_LOS_TEST = 0xE5
local LDOP_NOCLIP = 0xE6


----------------------------
---- public functions ------
----------------------------

function enable_cc_target(name, marker, spellID)
	lole_subcommands.sendmacro_to(name, "/lole cc enable", marker, spellID)
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
	if not playermode then
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
	if not playermode() then
		lop_exec(LOP_MELEE_BEHIND);
	end
end

function melee_avoid_aoe_buff(buff_spellID)
	if not playermode() then
		lop_exec(LOP_MELEE_AVOID_AOE_BUFF, buff_spellID);
	end

end

function hug_spell_with_spellID(spellID)
	if not playermode() then
		lop_exec(LOP_HUG_SPELL_OBJECT, spellID)
	end
end

function avoid_spell_with_spellID(spellID, radius)
	if not playermode() then
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
	return lop_exec(LOP_CHAIN_HEAL_TARGET, heals_in_progress);
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

function walk_to(x, y, z, prio)
	-- the last argument is the priority level
	lop_exec(LOP_CTM, x, y, z, prio)
end

function follow_unit(name)
	lop_exec(LOP_FOLLOW, name)
end

function stopfollow()
	lop_exec(LOP_STOPFOLLOW)
end

function is_walking()
    return lop_exec(LOP_GET_WALKING_STATE)
end

function cast_GTAOE(spellID, x, y, z)
	lop_exec(LOP_CAST_GTAOE, spellID, x, y, z);
end

function has_aggro()
	return lop_exec(LOP_HAS_AGGRO)
end

function get_cast_failed_msgid()
    return lop_exec(LOP_GET_PREVIOUS_CAST_MSG);
end

function interact_with_object(...)

	if select('#', ...) < 1 then
		lole_error("interact_with_object: no argument!")
		return
	end

	local name_concatenated = select(1, ...)

	for i = 2, select('#', ...) do
			local arg = select(i, ...);
			name_concatenated = name_concatenated .. " " .. arg
	end

	return lop_exec(LOP_INTERACT_GOBJECT, name_concatenated)
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

function get_biscuits()
	lop_exec(LOP_GET_BISCUITS)
end

function lole_encrypt_test()
	lop_exec(LDOP_ENCRYPT_TEST)
end

function lole_debug_pull_test()
	lop_exec(LDOP_PULL_TEST)
end

function noclip()
	lop_exec(LDOP_NOCLIP)
end
