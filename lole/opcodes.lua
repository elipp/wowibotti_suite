local LOP = {
	NOP = 0x0,
	TARGET_GUID = 0x1,
	CASTER_RANGE_CHECK = 0x2,
	FOLLOW = 0x3,  -- this also includes walking to/towards the target
	CTM = 0x4,
	DUNGEON_SCRIPT = 0x5,
	TARGET_MARKER = 0x6,
	MELEE_BEHIND = 0x7,
	AVOID_SPELL_OBJECT = 0x8,
	HUG_SPELL_OBJECT = 0x9,
	SPREAD = 0xA,
	CHAIN_HEAL_TARGET = 0xB,
	MELEE_AVOID_AOE_BUFF = 0xC,
	TANK_FACE = 0xD,
	WALK_TO_PULLING_RANGE = 0xE,
	GET_UNIT_POSITION = 0xF,
	GET_WALKING_STATE = 0x10,
	GET_CTM_STATE = 0x11,
	GET_PREVIOUS_CAST_MSG = 0x12,
	STOPFOLLOW = 0x13,
	CAST_GTAOE = 0x14,
	HAS_AGGRO = 0x15,
	INTERACT_GOBJECT = 0x16,
	GET_BISCUITS = 0x17,
	LOOT_BADGE = 0x18,
	LUA_UNLOCK = 0x19,
	LUA_LOCK = 0x1A,
	EXECUTE = 0x1B,
	FOCUS = 0x1C,
	CAST_SPELL = 0x1D,
	GET_COMBAT_TARGETS = 0x1E,
	GET_AOE_FEASIBILITY = 0x1F,
	AVOID_NPC_WITH_NAME = 0x20,
	BOSS_ACTION = 0x21,
	INTERACT_SPELLNPC = 0x22,
	GET_LAST_SPELL_ERRMSG = 0x23,
	ICCROCKET = 0x24,
	HCONFIG = 0x25,
	TANK_TAUNT_LOOSE = 0x26
}

local LOP_EXT = {
	NOP = 0x70,
	SL_RESETCAMERA = 0x71,
	WC3MODE = 0x72,
	SL_SETSELECT = 0x73
}

local LDOP = {
	NOP = 0xE0,
	DUMP = 0xE1,
	LOOT_ALL = 0xE2,
	PULL_TEST = 0xE3,
	LUA_REGISTERED = 0xE4,
	LOS_TEST = 0xE5,
	NOCLIP = 0xE6,
	TEST = 0xE7,
	CAPTURE_FRAME_RENDER_STAGES = 0xE8,
	CONSOLE_PRINT = 0xE9,
	REPORT_CONNECTED = 0xEA,
	EJECT_DLL = 0xEB
}


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
	lop_exec(LOP.TARGET_MARKER, marker)
end

function caster_range_check(minrange, maxrange)
	if not playermode() then
		if lop_exec(LOP.CASTER_RANGE_CHECK, minrange, maxrange) then
			return true
		else
			return false
		end
	end
end

function target_unit_with_GUID(GUID)
	lop_exec(LOP.TARGET_GUID, GUID)
end

function melee_attack_behind(minrange) -- magic value is 1.5
	if not playermode() then
		lop_exec(LOP.MELEE_BEHIND, minrange);
	end
end

function melee_avoid_aoe_buff(buff_spellID)
	if not playermode() then
		lop_exec(LOP.MELEE_AVOID_AOE_BUFF, buff_spellID);
	end

end

function hug_spell_with_spellID(spellID)
	if not playermode() then
		lop_exec(LOP.HUG_SPELL_OBJECT, spellID)
	end
end

function avoid_spell_with_spellID(spellID, radius)
	if not playermode() then
		lop_exec(LOP.AVOID_SPELL_OBJECT, spellID, radius)
	end
end

function pull_target()
	L_CastSpellByName("Avenger's Shield")
end

function walk_to_pulling_range()
	-- assuming the pull target is already selected
	lop_exec(LOP.WALK_TO_PULLING_RANGE)
end

function tank_face()
	lop_exec(LOP.TANK_FACE)
end

function get_CH_target_trio(heals_in_progress)
	return lop_exec(LOP.CHAIN_HEAL_TARGET, heals_in_progress);
end

function warlock_maulgar_get_felhound()
	local t = lop_exec(LOP.EXT_MAULGAR_GET_UNBANISHED_FELHOUND)
	return t;
end

function dscript(command, scriptname)
	lop_exec(LOP.DUNGEON_SCRIPT, command, scriptname);
end

function get_unit_position(unitname)
	return lop_exec(LOP.GET_UNIT_POSITION, unitname)
end

function lole_debug_dump_wowobjects(type_filter, ...)
	local name_filter = concatenate_args(" ", ...)
	lop_exec(LDOP.DUMP, type_filter, name_filter);
	echo("|cFF00FF96Dumped WowObjects to terminal (if you're injected!) Valid type filters are ITEM, NPC, UNIT, DYNAMICOBJECT, GAMEOBJECT")
	return true;
end

function lole_debug_loot_all()
	lop_exec(LDOP.LOOT_ALL);
end

function walk_to(x, y, z, prio)
	-- the last argument is the priority level
	--echo(x .. ", " .. y .. ", " .. z)
	lop_exec(LOP.CTM, x, y, z, prio)
end

function follow_unit(name)
	lop_exec(LOP.FOLLOW, name)
end

function stopfollow()
	lop_exec(LOP.STOPFOLLOW)
end

function is_walking()
    return lop_exec(LOP.GET_WALKING_STATE)
end

function cast_GTAOE(spellID, x, y, z)
	lop_exec(LOP.CAST_GTAOE, spellID, x, y, z);
end

function has_aggro()
	return lop_exec(LOP.HAS_AGGRO)
end

function get_cast_failed_msgid()
    return lop_exec(LOP.GET_PREVIOUS_CAST_MSG);
end

function set_selection(names_commaseparated)
	return lop_exec(LOP.SL_SETSELECT, names_commaseparated)
end

function interact_with_object(...)

	local name_concatenated = concatenate_args(" ", ...)

	if not name_concatenated then
		lole_error("interact_with_object: no argument!")
		return
	end

	return lop_exec(LOP.INTERACT_GOBJECT, name_concatenated)
end

function interact_with_spellnpc(...)
	local name_concatenated = concatenate_args(" ", ...)

	if not name_concatenated then
		lole_error("interact_with_spellnpc: no argument!")
		return
	end

	return lop_exec(LOP.INTERACT_SPELLNPC, name_concatenated)
end

function loot_badge(corpse_GUID)
	-- unitexists and stuff has already been checked in subcommands.lole_loot_badge
		lop_exec(LOP.LOOT_BADGE, corpse_GUID)
end

local INJECTED_STATUS = 0

function query_injected()
	if INJECTED_STATUS == 1 then return 1 end

	local lexec = _G.lop_exec; -- see if the function is registered
	if not lexec then
		return 0;
	else
		lop_exec(LDOP.LUA_REGISTERED);
		INJECTED_STATUS = 1;
		return 1;
	end

end

function iccrocket(mirror_data)
	lop_exec(LOP.ICCROCKET, mirror_data)
end

function lole_debug_test()
	lop_exec(LDOP.TEST, {"persse", "onsso"}, {"pirri", "kalltu"})
end

function lole_debug_pull_test()
	lop_exec(LDOP.PULL_TEST)
end

function cast_spell_packet(spellID)
	if not UnitGUID("target") then return end
	lop_exec(LOP.CAST_SPELL, spellID, UnitGUID("target"))
end

function get_combat_targets()
	return lop_exec(LOP.GET_COMBAT_TARGETS)
end

function execute_script(script)
	lop_exec(LOP.EXECUTE, script)
end

function enable_wc3mode(enabled)
	lop_exec(LOP.WC3MODE, tonumber(enabled))
end

function get_aoe_feasibility(range)
	return lop_exec(LOP.GET_AOE_FEASIBILITY, range)
end

function L_TargetUnit(name)
    lop_exec(LOP.TARGET_GUID, UnitGUID(name))
end

function L_ClearTarget()
	lop_exec(LOP.TARGET_GUID, "0x0000000000000000")
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
	execute_script("UseInventoryItem(" .. tostring(slotnumber) .. ")")
end

function L_SpellStopCasting()
	execute_script("SpellStopCasting()")
end

function L_InviteUnit(name)
	execute_script("InviteUnit(" .. tostring(name) .. ")")
end

function L_focus_target()
	lop_exec(LOP.FOCUS, UnitGUID("target"))
end

function L_clear_focus()
	lop_exec(LOP.FOCUS, "0x0000000000000000")
end

function L_target_focus()
	lop_exec(LOP.TARGET_GUID, UnitGUID("focus"))
end

function L_TargetTotem(slot)
	execute_script("TargetTotem(" .. tostring(slot) .. ")")
end

function reset_camera()
	lop_exec(LOP_EXT.SL_RESETCAMERA)
end

function avoid_npc_with_name(name, radius)
	local RADIUS = 0

	if not radius then RADIUS = 8
	else RADIUS = radius end

	return lop_exec(LOP.AVOID_NPC_WITH_NAME, name, RADIUS)
end

function get_last_spell_error()
	return lop_exec(LOP.GET_LAST_SPELL_ERRMSG)
end

function boss_action(name)
	lop_exec(LOP.BOSS_ACTION, name)
end

function hconfig(args)
	lop_exec(LOP.HCONFIG, args)
end

function capture_render_stages()
	lop_exec(LDOP.CAPTURE_FRAME_RENDER_STAGES)
end

function report_status_to_governor()
	local msg = UnitName("player") .. "," .. GetRealmName() .. "," .. GetMinimapZoneText()
	lop_exec(LDOP.REPORT_CONNECTED, msg)
end

function console_print(msg)
	lop_exec(LDOP.CONSOLE_PRINT, msg)
end

function eject_DLL()
	lop_exec(LDOP.EJECT_DLL)
end

local function get_available_taunt(taunt_spells)
    for _,spellname in pairs(taunt_spells) do
        if GetSpellCooldown(spellname) == 0 then
			return spellname
		end
	end

	return nil

end

function get_loose_tank_target(taunt_spells, ignore_mobs_tanked_by_GUID)
	local spell = get_available_taunt(taunt_spells)
	if spell then
		return lop_exec(LOP.TANK_TAUNT_LOOSE, spell, ignore_mobs_tanked_by_GUID), spell
	end
end