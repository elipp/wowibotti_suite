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
local LOP_LOOT_BADGE = 0x18
local LOP_LUA_UNLOCK = 0x19
local LOP_LUA_LOCK = 0x1A
local LOP_EXECUTE = 0x1B
local LOP_FOCUS = 0x1C
local LOP_CAST_SPELL = 0x1D
local LOP_GET_COMBAT_TARGETS = 0x1E
local LOP_GET_AOE_FEASIBILITY = 0x1F
local LOP_AVOID_NPC_WITH_NAME = 0x20
local LOP_BOSS_ACTION = 0x21
local LOP_INTERACT_SPELLNPC = 0x22
local LOP_GET_LAST_SPELL_ERRMSG = 0x23
local LOP_ICCROCKET = 0x24

local LOP_EXT_NOP = 0x70
local LOP_SL_RESETCAMERA = 0x71
local LOP_WC3MODE = 0x72
local LOP_SL_SETSELECT = 0x73

local LDOP_NOP = 0xE0
local LDOP_DUMP = 0xE1
local LDOP_LOOT_ALL = 0xE2
local LDOP_PULL_TEST = 0xE3
local LDOP_LUA_REGISTERED = 0xE4
local LDOP_LOS_TEST = 0xE5
local LDOP_NOCLIP = 0xE6
local LDOP_TEST = 0xE7
local LDOP_CAPTURE_FRAME_RENDER_STAGES = 0xE8
local LDOP_CONSOLE_PRINT = 0xE9
local LDOP_REPORT_CONNECTED = 0xEA
local LDOP_EJECT_DLL = 0xEB


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

function caster_range_check(minrange, maxrange)
	if not playermode() then
		if lop_exec(LOP_CASTER_RANGE_CHECK, minrange, maxrange) then
			return true
		else
			return false
		end
	end
end

function target_unit_with_GUID(GUID)
	lop_exec(LOP_TARGET_GUID, GUID)
end

function melee_attack_behind(minrange) -- magic value is 1.5
	if not playermode() then
		lop_exec(LOP_MELEE_BEHIND, minrange);
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
	L_CastSpellByName("Avenger's Shield")
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

function lole_debug_dump_wowobjects(name_filter, type_filter)
	lop_exec(LDOP_DUMP, name_filter, type_filter);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log (if you're injected!) ;)")
	return true;
end

function lole_debug_loot_all()
	lop_exec(LDOP_LOOT_ALL);
end

function walk_to(x, y, z, prio)
	-- the last argument is the priority level
	--echo(x .. ", " .. y .. ", " .. z)
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

function set_selection(names_commaseparated)
	return lop_exec(LOP_SL_SETSELECT, names_commaseparated)
end

function interact_with_object(...)

	local name_concatenated = concatenate_args(" ", ...)

	if not name_concatenated then
		lole_error("interact_with_object: no argument!")
		return
	end

	return lop_exec(LOP_INTERACT_GOBJECT, name_concatenated)
end

function interact_with_spellnpc(...)
	local name_concatenated = concatenate_args(" ", ...)

	if not name_concatenated then
		lole_error("interact_with_spellnpc: no argument!")
		return
	end

	return lop_exec(LOP_INTERACT_SPELLNPC, name_concatenated)
end

function loot_badge(corpse_GUID)
	-- unitexists and stuff has already been checked in subcommands.lole_loot_badge
		lop_exec(LOP_LOOT_BADGE, corpse_GUID)
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

function iccrocket(mirror_data)
	lop_exec(LOP_ICCROCKET, mirror_data)
end

function get_biscuits()
	lop_exec(LOP_GET_BISCUITS)
end


function lua_unlock()
	return lop_exec(LOP_LUA_UNLOCK)
end

function lua_lock()
	return lop_exec(LOP_LUA_LOCK)
end

function lole_debug_test()
	lop_exec(LDOP_TEST, angle)
end

function lole_debug_pull_test()
	lop_exec(LDOP_PULL_TEST)
end

function cast_spell_packet(spellID)
	if not UnitGUID("target") then return end
	lop_exec(LOP_CAST_SPELL, spellID, UnitGUID("target"))
end

function get_combat_targets()
	return lop_exec(LOP_GET_COMBAT_TARGETS)
end

function execute_script(script)
	lop_exec(LOP_EXECUTE, script)
end

function enable_wc3mode(enabled)
	lop_exec(LOP_WC3MODE, tonumber(enabled))
end

function get_aoe_feasibility(range)
	return lop_exec(LOP_GET_AOE_FEASIBILITY, range)
end

function L_TargetUnit(name)
    lop_exec(LOP_TARGET_GUID, UnitGUID(name))
end

function L_ClearTarget()
	lop_exec(LOP_TARGET_GUID, "0x0000000000000000")
end

function L_CastSpellByName(spellname)
	execute_script("CastSpellByName(\"" .. spellname .. "\")")
end

function L_StartAttack()
	execute_script("StartAttack()")
end

function L_RunScript(script)
	execute_script(script)
end

function L_PetAttack()
	execute_script("PetAttack()")
end

function L_PetFollow()
	execute_script("PetFollow()")
end

function L_PetPassiveMode()
	execute_script("PetPassiveMode()")
end

function L_RunMacroText(text)
	execute_script("RunMacroText(\"" .. text .. "\")")
end

function L_RunMacro(macroname)
	execute_script("RunMacro(\"" .. macroname .. "\")")
end

function L_TargetUnit(unitname)
	execute_script("TargetUnit(\"" .. unitname .. "\")")
end

function L_FocusUnit(unitname)
	execute_script("FocusUnit(\"" .. unitname .. "\")")
end

function L_UseItemByName(itemname)
	execute_script("UseItemByName(\"" .. itemname .. "\")")
end

function L_ClearFocus()
	execute_script("ClearFocus()")
end

function L_AcceptTrade()
	execute_script("AcceptTrade()")
end

function L_UseInventoryItem(slotnumber)
	execute_script("UseInventoryItem(" .. tostring(item) .. ")")
end

function L_SpellStopCasting()
	execute_script("SpellStopCasting()")
end

function L_InviteUnit(name)
	execute_script("InviteUnit(" .. tostring(name) .. ")")
end

function L_focus_target()
	lop_exec(LOP_FOCUS, UnitGUID("target"))
end

function L_clear_focus()
	lop_exec(LOP_FOCUS, "0x0000000000000000")
end

function L_target_focus()
	lop_exec(LOP_TARGET_GUID, UnitGUID("focus"))
end

function L_TargetTotem(slot)
	execute_script("TargetTotem(" .. tostring(slot) .. ")")
end

function reset_camera()
	lop_exec(LOP_SL_RESETCAMERA)
end

function avoid_npc_with_name(name, radius)
	local RADIUS = 0

	if not radius then RADIUS = 8
	else RADIUS = radius end

	return lop_exec(LOP_AVOID_NPC_WITH_NAME, name, RADIUS)
end

function get_last_spell_error()
	return lop_exec(LOP_GET_LAST_SPELL_ERRMSG)
end

function boss_action(name)
	lop_exec(LOP_BOSS_ACTION, name)
end


function capture_render_stages()
	lop_exec(LDOP_CAPTURE_FRAME_RENDER_STAGES)
end

function report_status_to_governor()
	local msg = UnitName("player") .. "," .. GetRealmName() .. "," .. GetMinimapZoneText()
	lop_exec(LDOP_REPORT_CONNECTED, msg)
end

function console_print(msg)
	lop_exec(LDOP_CONSOLE_PRINT, msg)
end

function eject_DLL()
	lop_exec(LDOP_EJECT_DLL)
end
