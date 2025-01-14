local lop_exec = ClosePetStables -- Can't actually lua_register a new function, because of warden and vfp_max :D

local LOP = Opcode

function LOP:call(opcode, ...)
    if query_injected() then
        return lop_exec(opcode, ...)
    else
        lole_error("LOP:call(" .. tostring(opcode) .. ") called, but we're not injected!")
    end
end

----------------------------
----- public functions -----
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
    LOP:call(LOP.TARGET_MARKER, marker)
end

function caster_range_check(minrange, maxrange)
    if not playermode() then
        if LOP:call(LOP.CasterRangeCheck, minrange, maxrange) then
            return true
        else
            return false
        end
    end
end

function healer_range_check(maxrange)
    if not playermode() then
        if LOP:call(LOP.HealerRangeCheck, maxrange) then
            return true
        else
            return false
        end
    end
end

function playback_path(name, reversed)
    LOP:call(LOP.PlaybackPath, name, reversed)
end

function target_unit_with_GUID(GUID)
    LOP:call(LOP.TargetGuid, GUID)
end

function refresh_hwevent_timestamp()
    LOP:call(LOP.RefreshHwEventTimestamp)
end

function melee_attack_behind(minrange) -- magic value is 1.5
    if not playermode() and not UnitIsDead("player") then
        LOP:call(LOP.MeleeBehind, minrange);
    end
end

function melee_avoid_aoe_buff(buff_spellID)
    if not playermode() then
        LOP:call(LOP.MELEE_AVOID_AOE_BUFF, buff_spellID);
    end
end

function hug_spell_with_spellID(spellID)
    if not playermode() then
        LOP:call(LOP.HUG_SPELL_OBJECT, spellID)
    end
end

function avoid_spell_with_spellID(spellID, radius)
    if not playermode() then
        LOP:call(LOP.AVOID_SPELL_OBJECT, spellID, radius)
    end
end

function pull_target()
    L_CastSpellByName("Avenger's Shield")
end

function walk_to_pulling_range()
    -- assuming the pull target is already selected
    LOP:call(LOP.WALK_TO_PULLING_RANGE)
end

function get_combat_participants()
    return LOP:call(LOP.GetCombatParticipants)
end

function get_combat_mobs()
    return LOP:call(LOP.GetCombatMobs)
end

function tank_face()
    LOP:call(LOP.TANK_FACE)
end

function get_CH_target_trio(heals_in_progress)
    return LOP:call(LOP.CHAIN_HEAL_TARGET, heals_in_progress);
end

function warlock_maulgar_get_felhound()
    local t = LOP:call(LOP.EXT_MAULGAR_GET_UNBANISHED_FELHOUND)
    return t;
end

function dscript(command, scriptname)
    LOP:call(LOP.DUNGEON_SCRIPT, command, scriptname);
end

function get_unit_position(unitname)
    return LOP:call(LOP.GetUnitPosition, unitname)
end

function lole_debug_dump_wowobjects(type_filter, ...)
    local name_filter = concatenate_args(" ", ...)
    LOP:call(LOP.Dump, type_filter, name_filter);
    echo(
        "|cFF00FF96Dumped WowObjects to terminal (if you're injected!) Valid type filters are ITEM, NPC, UNIT, DYNAMICOBJECT, GAMEOBJECT")
    return true;
end

function lole_debug_dump_wowobject_memory(n_bytes)
    if UnitName("target") == nil then
        return echo("Please select a target")
    end
    LOP:call(LOP.DumpWowObject, bytes)
end

function lole_debug_loot_all()
    LOP:call(LOP.LDOP_LOOT_ALL);
end

function walk_to(x, y, z, prio)
    -- the last argument is the priority level
    --echo(x .. ", " .. y .. ", " .. z)
    LOP:call(LOP.ClickToMove, x, y, z, prio)
end

function loot_mob()
    LOP:call(LOP.LootMob)
end

PATH_RECORDER = {
    frame = CreateFrame("Frame", "lolePathRecorder"),
    points = {},
    num_points = 0,
    recording = false,
    last_timestamp = 0,
    zonetext = nil,
    sample_interval = 1.5
}

function PATH_RECORDER:OnUpdate()
    local t = GetTime()
    if self.recording and (t - self.last_timestamp) > self.sample_interval then
        self:AddPoint()
    end
end

function PATH_RECORDER:AddPoint()
    local x, y, z, _ = get_unit_position("player")
    table.insert(self.points, { x = x, y = y, z = z })
    self.last_timestamp = GetTime()
    self.num_points = self.num_points + 1
    echo("inserted path point ", x, y, z, " - num_points:", self.num_points)
end

PATH_RECORDER.frame:SetScript("OnUpdate", function() PATH_RECORDER:OnUpdate() end)

function PATH_RECORDER:Start(sample_interval)
    if self.recording then
        return echo("Recording in progress; stop ongoing recording first!")
    end
    self.points = {}
    self.num_points = 0
    self:AddPoint()
    self.recording = true
    self.zonetext = GetZoneText()
    self.sample_interval = sample_interval or 1.5
end

function PATH_RECORDER:Stop(name)
    if (self.num_points == 0) and (not self.recording) then
        return echo("PATH_RECORDER is not recording or no waypoints stored")
    end
    if not name then
        return echo("PATH_RECORDER: `name` argument is mandatory; rerun :Stop(\"name\")")
    end
    self.recording = false
    self:AddPoint()
    LOP:call(LOP.StorePath, name, self.zonetext, self.points)
    self.points = {}
end

local TRYING_TO_FOLLOW = nil

function follow_unit(name)
    TRYING_TO_FOLLOW = name
    LOP:call(LOP.Follow, name)
end

function stopfollow(spread)
    if (not playermode()) and TRYING_TO_FOLLOW ~= nil then
        -- healers are calling stopfollow() in `cast_heal`, the TRYING_TO_FOLLOW guard is necessary, as not to interrupt all heals
        LOP:call((spread and LOP.StopFollowSpread) or LOP.StopFollow)
    end
    TRYING_TO_FOLLOW = nil
end

function is_walking()
    return LOP:call(LOP.GetWalkingState)
end

local last_gtaoe_timestamp = GetTime()

function cast_gtaoe(spell_name, x, y, z)
    local spellID = get_AOE_spellID(spell_name)
    if spellID ~= nil and IsSpellKnown(spellID) then
        if GetTime() - last_gtaoe_timestamp > 1.5 then
            LOP:call(LOP.CastGtAoe, spellID, x, y, z);
            last_gtaoe_timestamp = GetTime()
        end
    else
        return lole_error("cast_gtaoe: no valid ground-target-AOE spell \"" .. spell_name .. "\"!")
    end
end

function has_aggro()
    return LOP:call(LOP.HasAggro)
end

function set_selection(names_commaseparated)
    return LOP:call(LOP.SL_SETSELECT, names_commaseparated)
end

function interact_with_object(...)
    local name_concatenated = concatenate_args(" ", ...)

    if not name_concatenated then
        lole_error("interact_with_object: no argument!")
        return
    end

    return LOP:call(LOP.INTERACT_GOBJECT, name_concatenated)
end

function interact_with_spellnpc(...)
    local name_concatenated = concatenate_args(" ", ...)

    if not name_concatenated then
        lole_error("interact_with_spellnpc: no argument!")
        return
    end

    return LOP:call(LOP.INTERACT_SPELLNPC, name_concatenated)
end

function query_injected()
    -- return _G.lop_exec ~= nil
    return lop_exec(LOP.QueryInjected) -- must be lop_exec (not LOP:Call) because LOP:call calls query_injected :D
end

function iccrocket(mirror_data)
    LOP:call(LOP.ICCROCKET, mirror_data)
end

local function get_indent(indentlevel)
    local indent = ""
    for i = 0, indentlevel - 1 do
        indent = indent .. "    "
    end
    return indent
end

local function print_nested_table(tbl, indentlevel)
    if not indentlevel then
        indentlevel = 0
    else
        indentlevel = indentlevel + 1
    end

    for k, v in pairs(tbl) do
        if type(v) == "table" then
            print(get_indent(indentlevel), k)
            print_nested_table(v, indentlevel)
        else
            print(get_indent(indentlevel), k, v)
        end
    end
end

function lole_debug_test()
    local s = read_file(".\\Interface\\AddOns\\lole\\encounters\\encounter_template.json")
    local j = json.decode(s)
    --print(LOP:call(LOP.LDOP_TEST))
end

function lole_debug_pull_test()
    LOP:call(LOP.LDOP_PULL_TEST)
end

function cast_spell_packet(spellID)
    if not UnitGUID("target") then return end
    LOP:call(LOP.CAST_SPELL, spellID, UnitGUID("target"))
end

function get_combat_targets()
    return LOP:call(LOP.GetCombatTargets)
end

function execute_script(script)
    LOP:call(LOP.DoString, script)
end

function enable_wc3mode(enabled)
    LOP:call(LOP.WC3MODE, tonumber(enabled))
end

function get_aoe_feasibility(relative_to, range)
    return LOP:call(LOP.GetAoeFeasibility, relative_to, range)
end

function L_SendAddonMessage(...)
    LOP:call(LOP.SendAddonMessage, ...)
end

function L_ClearTarget()
    execute_script("ClearTarget()")
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

function L_PetDefensiveMode()
    execute_script("PetDefensiveMode()")
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
    LOP:call(LOP_EXT.SL_RESETCAMERA)
end

function avoid_npc_with_name(name, radius)
    local RADIUS = 0

    if not radius then
        RADIUS = 8
    else
        RADIUS = radius
    end

    return LOP:call(LOP.AVOID_NPC_WITH_NAME, name, RADIUS)
end

function get_last_spell_error()
    return LOP:call(LOP.GetLastSpellErrMsg)
end

function boss_action(name)
    LOP:call(LOP.BOSS_ACTION, name)
end

function hconfig(args)
    LOP:call(LOP.HCONFIG, args)
end

function capture_render_stages()
    LOP:call(LOP.LDOP_CAPTURE_FRAME_RENDER_STAGES)
end

function report_status_to_governor()
    local msg = UnitName("player") .. "," .. GetRealmName() .. "," .. GetMinimapZoneText()
    LOP:call(LOP.LDOP_REPORT_CONNECTED, msg)
end

function console_print(msg)
    LOP:call(LOP.LDOP_CONSOLE_PRINT, msg)
end

function eject_DLL()
    LOP:call(LOP.EjectDll)
end

local function get_available_taunt(taunt_spells)
    for _, spellname in pairs(taunt_spells) do
        if GetSpellCooldown(spellname) == 0 then
            return spellname
        end
    end

    return nil
end

function get_loose_tank_target(taunt_spells, ignore_mobs_tanked_by_GUID)
    local spell = get_available_taunt(taunt_spells)
    if spell then
        return LOP:call(LOP.TANK_TAUNT_LOOSE, spell, ignore_mobs_tanked_by_GUID), spell
    end
end

function read_file(filename)
    return LOP:call(LOP.READ_FILE, filename)
end

function loot_nearest_lootable_mob()
    return LOP:call(LOP.LootMob)
end

function face_mob()
    if not playermode() then
        return LOP:call(LOP.FaceMob)
    end
end

function L_MoveForwardStart()
    refresh_hwevent_timestamp()
    execute_script("MoveForwardStart()")
end

function L_MoveForwardStop()
    refresh_hwevent_timestamp()
    execute_script("MoveForwardStop()")
end

function L_MoveBackwardStart()
    refresh_hwevent_timestamp()
    execute_script("MoveBackwardStart()")
end

function L_MoveBackwardStop()
    refresh_hwevent_timestamp()
    execute_script("MoveBackwardStop()")
end

function L_MoveForwardStart()
    refresh_hwevent_timestamp()
    execute_script("MoveForwardStart()")
end

function L_MoveForwardStop()
    refresh_hwevent_timestamp()
    execute_script("MoveForwardStop()")
end

function L_JumpOrAscendStart()
    refresh_hwevent_timestamp()
    execute_script("JumpOrAscendStart()")
end

function L_AscendStop()
    refresh_hwevent_timestamp()
    execute_script("AscendStop()")
end

function L_StrafeLeftStart()
    refresh_hwevent_timestamp()
    execute_script("StrafeLeftStart()")
end

function L_StrafeLeftStop()
    refresh_hwevent_timestamp()
    execute_script("StrafeLeftStop()")
end

function L_StrafeRightStart()
    refresh_hwevent_timestamp()
    execute_script("StrafeRightStart()")
end

function L_StrafeRightStop()
    refresh_hwevent_timestamp()
    execute_script("StrafeRightStop()")
end

function L_TurnRightStart()
    refresh_hwevent_timestamp()
    execute_script("TurnRightStart()")
end

function L_TurnRightStop()
    refresh_hwevent_timestamp()
    execute_script("TurnRightStop()")
end

function L_TurnLeftStart()
    refresh_hwevent_timestamp()
    execute_script("TurnLeftStart()")
end

function L_TurnLeftStop()
    refresh_hwevent_timestamp()
    execute_script("TurnLeftStop()")
end
