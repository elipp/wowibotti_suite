-- opcodes for DelIgnore :P
local
LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID,
LOLE_OPCODE_BLAST,   -- this is basically deprecated now
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to/towards the target
LOLE_OPCODE_FACE, 	 -- also deprecated
LOLE_OPCODE_CTM_BROADCAST,
LOLE_OPCODE_COOLDOWNS,
LOLE_OPCODE_CC,
LOLE_OPCODE_DUNGEON_SCRIPT,
LOLE_OPCODE_TARGET_MARKER,
LOLE_OPCODE_DRINK,
LOLE_OPCODE_MELEE_BEHIND,
LOLE_OPCODE_LEAVE_PARTY,
LOLE_OPCODE_AFK_CLEAR,
LOLE_OPCODE_RELEASE_SPIRIT,
LOLE_OPCODE_MAIN_TANK, -- 0x10
LOLE_OPCODE_AVOID_SPELL_OBJECT,
LOLE_OPCODE_HUG_SPELL_OBJECT,
LOLE_OPCODE_SPREAD,
LOLE_OPCODE_PULL_MOB,
LOLE_OPCODE_REPORT_LOGIN,
LOLE_OPCODE_WALK_TO_PULLING_RANGE,
LOLE_OPCODE_GET_BEST_CHAINHEAL_TARGET,
LOLE_OPCODE_MAULGAR_GET_UNBANISHED_FELHOUND,
LOLE_OPCODE_OFF_TANK,
LOLE_OPCODE_SET_ALL,
LOLE_OPCODE_MELEE_AVOID_AOE_BUFF

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04",
"LOP_05", "LOP_06", "LOP_07", "LOP_08",
"LOP_09", "LOP_0A", "LOP_0B", "LOP_0C",
"LOP_0D", "LOP_0E", "LOP_0F", "LOP_10",
"LOP_11", "LOP_12", "LOP_13", "LOP_14",
"LOP_15", "LOP_16", "LOP_17", "LOP_18",
"LOP_19", "LOP_1A", "LOP_1B"

local
LOLE_DEBUG_OPCODE_NOP,
LOLE_DEBUG_OPCODE_DUMP,
LOLE_DEBUG_OPCODE_LOOT_ALL,
LOLE_DEBUG_OPCODE_QUERY_INJECTED,
LOLE_DEBUG_OPCODE_PULL_TEST

= "LOP_80", "LOP_81", "LOP_82", "LOP_83",
"LOP_84"

----------------------------
---- public functions ------
----------------------------

function send_opcode_addonmsg(opcode, message)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "RAID");
end

function send_opcode_addonmsg_guild(opcode, message)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "GUILD");
end

function send_opcode_addonmsg_to(opcode, message, to)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "WHISPER", to)
end

function broadcast_target_GUID(GUID_str)
	send_opcode_addonmsg(LOLE_OPCODE_TARGET_GUID, cipher_GUID(GUID_str));
end

function broadcast_follow_target(target_GUID)
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, cipher_GUID(target_GUID))
end

function broadcast_modeattrib_setall(attrib, state)
	send_opcode_addonmsg(LOLE_OPCODE_SET_ALL, attrib .. "," .. tostring(state));
end

function broadcast_cooldowns()
	send_opcode_addonmsg(LOLE_OPCODE_COOLDOWNS, "");
end

function broadcast_CTM(mode, arg)
	if mode == CTM_MODES.LOCAL then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. arg);

	elseif mode == CTM_MODES.TARGET then
		local target_GUID = UnitGUID("target")
		if not target_GUID then return end

		echo("sending CTM to target " .. target_GUID)
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. target_GUID .. "," .. arg)

		-- kinda redundant.
	elseif mode == CTM_MODES.EVERYONE then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)

	elseif mode == CTM_MODES.HEALERS then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)

	elseif mode == CTM_MODES.CASTERS then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)

	elseif mode == CTM_MODES.MELEE then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)
	else
		lole_error("lole_ctm: invalid mode: " .. tostring(mode));
		return false;
	end

end

function broadcast_drink()
	send_opcode_addonmsg(LOLE_OPCODE_DRINK, "");
end

function broadcast_main_tank(arg)
	send_opcode_addonmsg(LOLE_OPCODE_MAIN_TANK, arg)
end

function broadcast_off_tank(arg)
	send_opcode_addonmsg(LOLE_OPCODE_OFF_TANK, arg)
end

function set_target(target_GUID)
	--echo("calling set_target")
  	DelIgnore(LOLE_OPCODE_TARGET_GUID .. ":" .. target_GUID); -- this does a C TargetUnit call :P
    BLAST_TARGET_GUID = target_GUID;
	FocusUnit("target")
	update_target_text(UnitName("target"), UnitGUID("target"));

end

function clear_target()
	--echo("calling clear_target!");
	BLAST_TARGET_GUID = NOTARGET;
	ClearFocus();
	ClearTarget();
	update_target_text("-none-", "");

end

function enable_cc_target(name, spell, marker)
	send_opcode_addonmsg_to(LOLE_OPCODE_CC, "1" .. "," .. spell .. "," .. marker, name)
end

function disable_cc_target(name, spell, marker)
	send_opcode_addonmsg_to(LOLE_OPCODE_CC, "0" .. "," .. spell .. "," .. marker, name)
end

function disable_all_cc_targets()
	send_opcode_addonmsg(LOLE_OPCODE_CC, "-1" .. "," .. "all" .. "," .. "all")
end

function target_unit_with_marker(marker)
	DelIgnore(LOLE_OPCODE_TARGET_MARKER .. ":" .. marker);
end

function caster_range_check(minrange)
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_CASTER_RANGE_CHECK .. ":" .. tostring(minrange));
	end
end

function caster_face_target()
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_FACE);
	end
end

function melee_attack_behind()
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_MELEE_BEHIND)
	end
end

function melee_avoid_aoe_buff(buff_spellID)
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_MELEE_AVOID_AOE_BUFF .. ":" .. tostring(buff_spellID))
	end

end

function leave_party_all()
	send_opcode_addonmsg_guild(LOLE_OPCODE_LEAVE_PARTY, "")
end

function release_spirit_all()
	send_opcode_addonmsg(LOLE_OPCODE_RELEASE_SPIRIT, "");
end

local AFK_clear_timestamp = time()

function afk_clear()
	RunMacroText("/afk")
	DelIgnore(LOLE_OPCODE_AFK_CLEAR);
	AFK_clear_timestamp = time()
end

function time_since_last_afk_clear()
	return time() - AFK_clear_timestamp
end

function hug_spell_with_spellID(spellID)
	DelIgnore(LOLE_OPCODE_HUG_SPELL_OBJECT .. ":" .. tostring(spellID))
end

function avoid_spell_with_spellID(spellID, radius)
	DelIgnore(LOLE_OPCODE_AVOID_SPELL_OBJECT .. ":" .. tostring(spellID) .. "," .. tostring(radius))
end

function pull_target()
	CastSpellByName("Avenger Shield")
end

function walk_to_pulling_range()
	-- assuming the pull target is already selected
	DelIgnore(LOLE_OPCODE_WALK_TO_PULLING_RANGE)
end

function report_login(flag)
	local str_arg = ""
	if flag == true then
		str_arg = "1"
	else
		str_arg = "0"
	end
	DelIgnore(LOLE_OPCODE_REPORT_LOGIN .. ":" .. str_arg)
end

function target_best_CH_target()
	DelIgnore(LOLE_OPCODE_GET_BEST_CHAINHEAL_TARGET)
end

function warlock_banish_felhound()
	DelIgnore(LOLE_OPCODE_MAULGAR_GET_UNBANISHED_FELHOUND)
end

function lole_debug_dump_wowobjects()
	DelIgnore(LOLE_DEBUG_OPCODE_DUMP);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log (if you're injected!) ;)")
	return true;
end

function lole_debug_loot_all()
	DelIgnore(LOLE_DEBUG_OPCODE_LOOT_ALL);
end

function lole_debug_query_injected()
	DelIgnore(LOLE_DEBUG_OPCODE_QUERY_INJECTED)
end

function lole_debug_pull_test()
	DelIgnore(LOLE_DEBUG_OPCODE_PULL_TEST)
end

------------------------------------------------------------------------------
----- OPCODE callback (or "OCB") funcs. called in lole.lua:handle_opcode -----
------------------------------------------------------------------------------

local function OCB_nop() return end

local function OCB_target_unit_with_GUID(GUID_str_ciphered)

	if lole_subcommands.get("playermode") == 1 then return; end

	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);

	if GUID_deciphered == NOTARGET then
		clear_target()
		return
	end

   	if (BLAST_TARGET_GUID ~= GUID_deciphered) then
    	set_target(GUID_deciphered)
		return
    end

end

local function OCB_follow_unit_with_GUID(GUID_str_ciphered)
	--if IsRaidLeader() then return end

	if lole_subcommands.get("playermode") == 0 then
		local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. GUID_deciphered);
	end
end

local function OCB_set_blast(mode)
 --  DelIgnore(LOLE_OPCODE_BLAST .. ":" .. mode);
	if mode == 1 or mode == "1" then
		lole_subcommands.set("blast", "1")
		gui_set_blast(true)
	else
		lole_subcommands.set("blast", "0")
		gui_set_blast(false)
		clear_target();
	end
end


local function OCB_act_on_CTM_broadcast(args)

	local modestr,GUID,x,y,z = unpack(tokenize_string(args, ","))

	if lole_subcommands.get("playermode") == 1 then
		return;
	end

	local coords = x .. "," .. y .. "," .. z

	local mode = tonumber(modestr)

	if mode == CTM_MODES.LOCAL then
		if GUID == UnitGUID("player") then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.TARGET then
		if GUID == UnitGUID("player") then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end

	elseif mode == CTM_MODES.EVERYONE then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords)

	-- elseif mode == CTM_MODES.HEALERS then
	-- 	if get_current_config().role == ROLES.HEALER then
	-- 		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
	-- 	end
	-- elseif mode == CTM_MODES.CASTERS then
	-- 	if get_current_config().role == ROLES.CASTER then
	-- 		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
	-- 	end
	-- elseif mode == CTM_MODES.MELEE then
	-- 	if get_current_config().role == ROLES.MELEE then
	-- 		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
	-- 	end
	else
		lole_error("act_on_CTM_broadcast: invalid mode " .. modestr);
	end


end

local function OCB_range_check(minrange)
	caster_range_check(minrange)
end

local function OCB_face_target()
	caster_face_target();
end

local function OCB_stopfollow()
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. NOTARGET);
	end
end

local function OCB_blow_cooldowns()
	lole_subcommands.cooldowns()
end

local function OCB_set_cc_target(arg)

	local _enabled, _spell, _marker = strsplit(",", arg);
	local enabled, spell, marker = tonumber(trim_string(_enabled)), trim_string(_spell), trim_string(_marker)

	echo("set_cc_target: got args: " .. enabled .. ", " .. spell .. ", " .. marker);

	if (enabled == 1) then
		set_CC_job(spell, marker)
		new_CC(UnitName("player"), get_CC_spellID(spell), marker);
	elseif (enabled == 0) then
		unset_CC_job(marker)
		delete_CC_entry(marker)
	elseif (enabled == -1) then
		unset_all_CC_jobs()
		delete_all_CC_entries()
	else
		lole_error("set_cc_target: invalid argument! (enabled must be 1, 0 or -1)");
		return false
	end

	return false;
end

local function OCB_set_all(arg)
	local attrib, state = strsplit(",", arg)
	local gui_arg = nil

	if state == "1" then
		gui_arg = true
	else gui_arg = false end

	if attrib == "blast" then
		gui_set_blast(gui_arg)
	elseif attrib == "heal_blast" then
		gui_set_heal_blast(gui_arg)
	end

	lole_subcommands.set(attrib, state)
end

local function OCB_drink()
	lole_subcommands.drink()
end

local function OCB_load_dungeon_script(script)
	-- nyi as fuck :D
end

local function OCB_melee_behind()
	DelIgnore(LOLE_OPCODE_MELEE_BEHIND);
end

local function OCB_leave_party()
	LeaveParty()
end

local function OCB_afk_clear()
	DelIgnore(LOLE_OPCODE_AFK_CLEAR)
end

local function OCB_release_spirit()
	RepopMe()
end

local function OCB_main_tank(arg)
	MAIN_TANK = arg
	update_main_tank(MAIN_TANK)
	echo("Main tank set to " .. MAIN_TANK .. ". (change with /lole mt <mtname>)")
end

local function OCB_off_tank(arg)
	OFF_TANK = arg
	update_off_tank(OFF_TANK)
	echo("Off tank set to " .. OFF_TANK .. ". (change with /lole ot <otname>)")
end

lole_opcode_funcs = {
	[LOLE_OPCODE_NOP] = 				OCB_nop,
	[LOLE_OPCODE_TARGET_GUID] = 		OCB_target_unit_with_GUID,
	[LOLE_OPCODE_BLAST] = 				OCB_set_blast,
	[LOLE_OPCODE_CASTER_RANGE_CHECK] = 	OCB_range_check,
	[LOLE_OPCODE_FOLLOW] = 				OCB_follow_unit_with_GUID,
	[LOLE_OPCODE_FACE] = 				OCB_face_target,
	[LOLE_OPCODE_CTM_BROADCAST] = 		OCB_act_on_CTM_broadcast,
	[LOLE_OPCODE_COOLDOWNS] = 			OCB_blow_cooldowns,
	[LOLE_OPCODE_CC] = 					OCB_set_cc_target,
	[LOLE_OPCODE_DUNGEON_SCRIPT] = 		OCB_load_dungeon_script,
	[LOLE_OPCODE_DRINK] = 				OCB_drink,
	[LOLE_OPCODE_MELEE_BEHIND] = 		OCB_melee_behind,
	[LOLE_OPCODE_LEAVE_PARTY] = 		OCB_leave_party,
	[LOLE_OPCODE_AFK_CLEAR] = 			OCB_afk_clear,
	[LOLE_OPCODE_RELEASE_SPIRIT] =		OCB_release_spirit,
	[LOLE_OPCODE_MAIN_TANK] =			OCB_main_tank,
	[LOLE_OPCODE_PULL_MOB] = 			OCB_pull_mob,
	[LOLE_OPCODE_OFF_TANK] =			OCB_off_tank,
	[LOLE_OPCODE_SET_ALL] =				OCB_set_all,
}
