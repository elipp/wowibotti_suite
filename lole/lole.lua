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

local function on_spell_sent_event(self, event, caster, spell, rank, target)
    -- TODO: multi-target heals
    local heal_estimate = HEAL_ESTIMATES[spell.."("..rank..")"];
    local _, _, _, _, _, _, cast_time = GetSpellInfo(spell);
    local finish_time = tostring(GetTime()*1000 + cast_time);
    if heal_estimate ~= nil then
        msg = target .. "," .. UnitName(caster) .. "," .. heal_estimate .. "," .. finish_time;
        SendAddonMessage("lole_current_heals", msg, "RAID");
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
		local guildies = get_guild_members()
		if guildies[sender] then
			RunScript(message);
		else
			SendChatMessage("lole_runscript: " .. sender .. " doesn't appear to be a member of Uuslapio, not running script!", "GUILD");
		end

    elseif (prefix == "lole_healers") then
        -- message: healer1:target1,target2,...,targetN.healerN:target1,...
        echo(message);
        if message == nil or message == "" then
            HEALER_TARGETS = {}
            echo("lole_healers: Wiped out healer assignments.");
            return
        end
        local per_healer = {strsplit(".", message)};
        echo("lole_healers: Healer assignments change:")
        for key, healer_targets in pairs(per_healer) do
            local ht = {strsplit(":", healer_targets)};
            local healer = ht[1];
            local targets = {};
            if ht[2] == nil then
                ht[2] = "";
            else
                targets = {strsplit(",", ht[2])};
            end
            HEALER_TARGETS[healer] = targets;
            echo(healer .. ": " .. ht[2]);
        end

    elseif (prefix == "lole_current_heals") then
        local msg = {strsplit(",", message)};
        HEALS_IN_PROGRESS[msg[1]][msg[2]] = {msg[3],msg[4]};

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

local spell_sent_frame = nil;
local healers = {"Kasio", "Mam", "Igop", "Pehmware", "Kusip", "Ceucho"};
if table.contains(healers, UnitName("player")) then
    spell_sent_frame = CreateFrame("Frame");
    spell_sent_frame:RegisterEvent("UNIT_SPELLCAST_SENT");
    spell_sent_frame:SetScript("OnEvent", on_spell_sent_event);
end


function lole_OnLoad()
	SLASH_LOLEXDD1= "/lole";
	SlashCmdList["LOLEXDD"] = lole_SlashCommand;
end
