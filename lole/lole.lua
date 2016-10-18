LOLE_CLASS_CONFIG_NAME_SAVED = "default";
LOLE_CLASS_CONFIG_ATTRIBS_SAVED = nil; -- listed in SavedVariablesPerCharacter

local function usage()
	echo("|cFFFFFF00/lole usage: /lole subcmd subcmd_arg");

	echo(" - Available subcommands are: |cFFFFFF00\n" .. get_available_subcommands());
	echo(" - Available class configs are: |cFFFFFF00\n" .. get_available_class_configs_pretty());

	echo(" - Available mode attributes (for /lole set) are: |cFFFFFF00\n" .. get_mode_attribs())

end

local function handle_subcommand(args)

	local atab = {strsplit(" ", args)};
	local numargs = table.getn(atab);

	if (numargs < 1) then
		usage();
		return false;
	end

	local a1 = atab[1];
	table.remove(atab, 1);
	local cmdfunc = lole_subcommands[a1];

    if cmdfunc then
		cmdfunc(unpack(atab));
		return true;
	else
		lole_error("unknown subcommand \"" .. a1 .. "\".");
		usage();
		return false;
	end


end

function lole_main(args)

	if args and args ~= "" then
		handle_subcommand(args)
		return;
	end

    if lole_subcommands.get("buffmode") == 1 then
        lole_buffs();
    else
        if (time() - LAST_BUFF_CHECK) > 30 then
            lole_buffcheck(nil, false);
        elseif (lole_subcommands.get("combatbuffmode") == 1 or LBUFFCHECK_ISSUED) and BUFFS_CHECKED and (time() - LAST_BUFF_CHECK) > 1 then
            lole_subcommands.set("buffmode", "on");
            BUFFS_CHECKED = false;
			return;
        end

		if lole_subcommands.get("playermode") ~= 1 then
            if UnitExists("focus") and UnitIsDead("focus") then
				ClearFocus()
			end

			get_current_config().combat();

        end
    end


	if (IsRaidLeader()) then
		--if (BLAST_TARGET_GUID ~= NOTARGET or (not UnitExists("focus")))
		if (UnitExists("focus") and UnitIsDead("focus")) then
			clear_target()
			broadcast_target_GUID(NOTARGET)
		end

		if BLAST_TARGET_GUID == NOTARGET then
			if not UnitExists("focus") then
				if UnitExists("target") and not UnitIsDead("target") and UnitReaction("target", "player") < 5 then
					set_target(UnitGUID("target"))
					broadcast_target_GUID(UnitGUID("target"));
				end
			else
					-- not sure if this is reachable or not
				clear_target()
			end
		end
	end

    return;

end

local function lole_SlashCommand(args)
	lole_main(args)
end

local function on_buff_check_event(self, event, ...)
    lole_buffcheck(nil, false);
end

local function handle_opcode(arg)

	--lole_error(arg); -- debug

	local opcode, message = strsplit(":", arg);

	if not lole_opcode_funcs[opcode] then
		lole_error("unknown opcode " .. tostring(opcode))
		return false;
	end

	lole_opcode_funcs[opcode](message);

	return true;

end

PREFIX_LOCKS = {
    lole_runscript = false,
    lole_healers = false,
    lole_echo = false,
    UNIT_SPELLCAST_SENT = false,
    UNIT_SPELLCAST_START = false,
}
local function prevent_double_call(prefix)
    local r = PREFIX_LOCKS[prefix];
    PREFIX_LOCKS[prefix] = not PREFIX_LOCKS[prefix];
    return r;
end

SPELL_TARGET = UnitName("player");
local function on_spell_event(self, event, caster, spell, rank, target)
    if prevent_double_call(event) then return end
    if event == "UNIT_SPELLCAST_SENT" then
        SPELL_TARGET = target;
        return
    end
    local heal_estimate = HEAL_ESTIMATES[spell.."("..rank..")"];
    if heal_estimate then
        if UnitName(caster) == UnitName("player") then
            local targets = {SPELL_TARGET};
            if spell == "Binding Heal" then
                targets = {SPELL_TARGET, UnitName("player")};
            elseif spell == "Prayer of Healing" then
                targets = get_group_members(get_group_number(UnitName("player")));
            end
            local targets_str = "";
            for i, name in ipairs(targets) do
                if i == 1 then
                    targets_str = targets_str .. name;
                else
                    targets_str = targets_str .. "," .. name;
                end
            end
            SendAddonMessage("lole_heal_target", targets_str, "RAID");
        else
            local _, _, _, _, _, finish_time = UnitCastingInfo(caster);
            HEAL_FINISH_INFO[UnitName(caster)] = {heal_estimate, finish_time};
        end
    end
end

local function OnMsgEvent(self, event, prefix, message, channel, sender)

	if (prefix == "lole_opcode") then
		handle_opcode(message)

	elseif (prefix == "lole_buffs") then
        local buffs = {strsplit(",", message)};
        for key, buff in pairs(buffs) do
            if not MISSING_BUFFS[buff] then
                MISSING_BUFFS[buff] = {[sender] = true};
            else
                MISSING_BUFFS[buff][sender] = true;
            end
        end

    elseif (prefix == "lole_buffcheck") then
        if (time() - LAST_LBUFFCHECK) > 1 then
            if message == "buffcheck" then
                echo("lole: The raid leader issued a buffcheck.");
                lole_buffcheck(nil, false);
            elseif message == "buffcheck clean" then
                echo("lole: The raid leader issued a clean buffcheck.");
                lole_buffcheck("clean", false);
            end
            LAST_LBUFFCHECK = time();
            LBUFFCHECK_ISSUED = true;
		end

	elseif (prefix == "lole_runscript") then
        if prevent_double_call(prefix) then return end
		local guildies = get_guild_members()
		if guildies[sender] then
			RunScript(message);
		else
			SendChatMessage("lole_runscript: " .. sender .. " doesn't appear to be a member of Uuslapio, not running script!", "GUILD");
		end

    elseif (prefix == "lole_healers") then
        if prevent_double_call(prefix) then return end
        handle_healer_assignment(message);

    elseif (prefix == "lole_heal_target") then
        if HEAL_FINISH_INFO[sender] then
            local targets = {strsplit(",", message)};
            for i, target in pairs(targets) do
                HEALS_IN_PROGRESS[target][sender] = HEAL_FINISH_INFO[sender];
            end
        end

    elseif (prefix == "lole_echo") then 
        -- Feenix addon messaging cannot handle "\n", so we use "§" instead
        -- and substitute "§" symbols with "\n" here.
        if prevent_double_call(prefix) then return end
        message = string.gsub(message, "§", "\n");
        echo(message);

    elseif (prefix == "lole_mount") then
        RunMacro("mount");

	end
end

local buff_check_frame = CreateFrame("Frame");
buff_check_frame:RegisterEvent("PLAYER_ALIVE");
buff_check_frame:RegisterEvent("PLAYER_ENTERING_WORLD");
buff_check_frame:SetScript("OnEvent", on_buff_check_event);

local msg_frame = CreateFrame("Frame");
msg_frame:RegisterEvent("CHAT_MSG_ADDON");
msg_frame:SetScript("OnEvent", OnMsgEvent);

local spell_event_frame = nil;
if HEALER_TARGETS[UnitName("player")] then
    spell_event_frame = CreateFrame("Frame");
    spell_event_frame:RegisterEvent("UNIT_SPELLCAST_SENT");
    spell_event_frame:RegisterEvent("UNIT_SPELLCAST_START");
    spell_event_frame:SetScript("OnEvent", on_spell_event);
end


function lole_OnLoad()
	SLASH_LOLEXDD1= "/lole";
	SlashCmdList["LOLEXDD"] = lole_SlashCommand;
end
