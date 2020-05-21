local LOP = { NUM_OPCODES = 0, NUM_DEBUG_OPCODES = 0, LDOP_MASK = 0x40000000 }

function LOP:add_opcode(name)
	self[name] = self.NUM_OPCODES
	self.NUM_OPCODES = self.NUM_OPCODES + 1
end

function LOP:add_debug_opcode(name)
	self[name] = BITWISE.OR(self.NUM_DEBUG_OPCODES, self.LDOP_MASK)
	self.NUM_DEBUG_OPCODES = self.NUM_DEBUG_OPCODES + 1
end

-- kinda KlunKy, but helps keep track of these

LOP:add_opcode("NOP")
LOP:add_opcode("TARGET_GUID")
LOP:add_opcode("CASTER_RANGE_CHECK")
LOP:add_opcode("FOLLOW")
LOP:add_opcode("CTM")
LOP:add_opcode("TARGET_MARKER")
LOP:add_opcode("MELEE_BEHIND")
LOP:add_opcode("AVOID_SPELL_OBJECT")
LOP:add_opcode("HUG_SPELL_OBJECT")
LOP:add_opcode("SPREAD")
LOP:add_opcode("CHAIN_HEAL_TARGET")
LOP:add_opcode("MELEE_AVOID_AOE_BUFF")
LOP:add_opcode("TANK_FACE")
LOP:add_opcode("TANK_PULL")
LOP:add_opcode("GET_UNIT_POSITION")
LOP:add_opcode("GET_WALKING_STATE")
LOP:add_opcode("GET_CTM_STATE")
LOP:add_opcode("GET_PREVIOUS_CAST_MSG")
LOP:add_opcode("STOPFOLLOW")
LOP:add_opcode("CAST_GTAOE")
LOP:add_opcode("HAS_AGGRO")
LOP:add_opcode("INTERACT_GOBJECT")
LOP:add_opcode("EXECUTE")
LOP:add_opcode("GET_COMBAT_TARGETS")
LOP:add_opcode("GET_AOE_FEASIBILITY")
LOP:add_opcode("AVOID_NPC_WITH_NAME")
LOP:add_opcode("BOSS_ACTION")
LOP:add_opcode("INTERACT_SPELLNPC")
LOP:add_opcode("GET_LAST_SPELL_ERRMSG")
LOP:add_opcode("ICCROCKET")
LOP:add_opcode("HCONFIG")
LOP:add_opcode("TANK_TAUNT_LOOSE")
LOP:add_opcode("READ_FILE")

LOP:add_debug_opcode("LDOP_NOP")
LOP:add_debug_opcode("LDOP_DUMP")
LOP:add_debug_opcode("LDOP_LOOT_ALL")
LOP:add_debug_opcode("LDOP_PULL_TEST")
LOP:add_debug_opcode("LDOP_LUA_REGISTER")
LOP:add_debug_opcode("LDOP_LOS_TEST")
LOP:add_debug_opcode("LDOP_TEST")
LOP:add_debug_opcode("LDOP_CAPTURE_FRAME_RENDER_STAGES")
LOP:add_debug_opcode("LDOP_CONSOLE_PRINT")
LOP:add_debug_opcode("LDOP_REPORT_CONNECTED")
LOP:add_debug_opcode("LDOP_EJECT_DLL")

LOP:add_debug_opcode("LDOP_SL_RESETCAMERA")
LOP:add_debug_opcode("LDOP_WC3MODE")
LOP:add_debug_opcode("LDOP_SL_SETSELECT")


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
	lop_exec(LOP.DUMP, type_filter, name_filter);
	echo("|cFF00FF96Dumped WowObjects to terminal (if you're injected!) Valid type filters are ITEM, NPC, UNIT, DYNAMICOBJECT, GAMEOBJECT")
	return true;
end

function lole_debug_loot_all()
	lop_exec(LOP.LDOP_LOOT_ALL);
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

local INJECTED_STATUS = 0

function query_injected()
	if INJECTED_STATUS == 1 then return 1 end

	local lexec = _G.lop_exec; -- see if the function is registered
	if not lexec then
		return 0;
	else
		lop_exec(LOP.LDOP_LUA_REGISTER);
		INJECTED_STATUS = 1;
		return 1;
	end

end

function iccrocket(mirror_data)
	lop_exec(LOP.ICCROCKET, mirror_data)
end

local function get_indent(indentlevel)
	local indent = ""
	for i = 0, indentlevel - 1 do
		indent = indent .. "    "
	end
	return indent
end

local function print_nested_table(tbl, indentlevel)
	if not indentlevel then indentlevel = 0 
	else indentlevel = indentlevel + 1 end

	for k,v in pairs(tbl) do 
		if type(v) == "table" then
			print(get_indent(indentlevel), k)
			print_nested_table(v, indentlevel)
		else
			print(get_indent(indentlevel), k, v)
		end
    end

end

function lole_debug_test()
	local s = read_file(".\\Interface\\AddOns\\lole\\encounter_scripts\\encounter_template.json.tmpl")
	local j = json.decode(s)
	print_nested_table(j)
end

function lole_debug_pull_test()
	lop_exec(LOP.LDOP_PULL_TEST)
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
	lop_exec(LOP.LDOP_CAPTURE_FRAME_RENDER_STAGES)
end

function report_status_to_governor()
	local msg = UnitName("player") .. "," .. GetRealmName() .. "," .. GetMinimapZoneText()
	lop_exec(LOP.LDOP_REPORT_CONNECTED, msg)
end

function console_print(msg)
	lop_exec(LOP.LDOP_CONSOLE_PRINT, msg)
end

function eject_DLL()
	lop_exec(LOP.LDOP_EJECT_DLL)
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

function read_file(filename)
	return lop_exec(LOP.READ_FILE, filename)
end