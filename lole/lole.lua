LOLE_CLASS_CONFIG_NAME = "default";
LOLE_CLASS_CONFIG_ATTRIBS = nil; -- listed in SavedVariablesPerCharacter

local DEFAULT_CONFIG = { name = "default", COLOR = "FFFFFF", MODE_ATTRIBS = {}, desired_buffs = function() return {}; end, combat = function() end, buffs = function() end, other = function() end };
LOLE_CLASS_CONFIG = DEFAULT_CONFIG;

LOLE_BLAST_STATE = nil;

available_configs = {
	default = DEFAULT_CONFIG,
	druid_resto = config_druid_resto,
	druid_balance = config_druid_balance,
	mage_fire = config_mage_fire,
	mage_frost = config_mage_frost,
	paladin_prot = config_paladin_prot,
	paladin_holy = config_paladin_holy,
	paladin_retri = config_paladin_retri,
    priest_holy = config_priest_holy,
	priest_shadow = config_priest_shadow,
	shaman_elem = config_shaman_elem,
    shaman_resto = config_shaman_resto,
	warlock_affli = config_warlock_affli,
	warlock_sb = config_warlock_sb,
	warrior_prot = config_warrior_prot,
	warrior_arms = config_warrior_arms
};



local function usage()
	echo("|cFFFFFF00/lole usage: /lole subcmd subcmd_arg");
	
	echo(" - Available subcommands are: |cFFFFFF00\n" .. get_available_subcommands());
	echo(" - Available class configs are: |cFFFFFF00\n" .. get_available_class_configs_pretty());	
	
	echo(" - Available mode attributes for this class config (" 
	.. LOLE_CLASS_CONFIG.name .. ") are: |cFFFFFF00\n" .. get_config_mode_attribs(LOLE_CLASS_CONFIG))
	
end

local function handle_subcommand(args)

	local atab = {strsplit(" ", args)};
	local numargs = table.getn(atab);

	if (numargs < 1) then 
		usage();
		return false;
	end
	
	local a1 = atab[1];
	local a2 = atab[2];
	local a3 = atab[3]; 

	local cmdfunc = lole_subcommands[a1];
    if cmdfunc then
		cmdfunc(a2, a3);
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
	
    if LOLE_CLASS_CONFIG.MODE_ATTRIBS["buffmode"] == 1 then
        lole_buffs();
    else
        if (time() - LAST_BUFF_CHECK) > 30 then
            lole_buffcheck(nil, false);
        elseif (LOLE_CLASS_CONFIG.MODE_ATTRIBS["combatbuffmode"] == 1 or LBUFFCHECK_ISSUED) and BUFFS_CHECKED and (time() - LAST_BUFF_CHECK) > 1 then
            lole_subcommands.set("buffmode", "on");
            BUFFS_CHECKED = false;
			return;
        end
        if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] ~= 1 then
            if UnitExists("focus") and UnitIsDead("focus") then 
				ClearFocus()
			end
			LOLE_CLASS_CONFIG.combat();
        end
    end
		
		
	if (IsRaidLeader()) then
		if UnitExists("focus") and UnitIsDead("focus") then 
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

	end
end

local buff_check_frame = CreateFrame("Frame");
buff_check_frame:RegisterEvent("PLAYER_ALIVE");
buff_check_frame:RegisterEvent("PLAYER_ENTERING_WORLD");
buff_check_frame:SetScript("OnEvent", on_buff_check_event);

local msg_frame = CreateFrame("Frame");
msg_frame:RegisterEvent("CHAT_MSG_ADDON");
msg_frame:SetScript("OnEvent", OnMsgEvent);


function lole_OnLoad()
	SLASH_LOLEXDD1= "/lole";
	SlashCmdList["LOLEXDD"] = lole_SlashCommand;
end
