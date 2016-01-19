LOLE_CLASS_CONFIG_NAME = "default";
LOLE_CLASS_CONFIG_ATTRIBS = nil; -- listed in SavedVariablesPerCharacter

local DEFAULT_CONFIG = { name = "default", MODE_ATTRIBS = nil, desired_buffs = function() return {}; end, combat = function() end, buffs = function() end, other = function() end };
LOLE_CLASS_CONFIG = DEFAULT_CONFIG;


local available_configs = {
	["default"] = DEFAULT_CONFIG,
	["paladin_prot"] = config_paladin_prot,
	["druid_resto"] = config_druid_resto,
	["paladin_holy"] = config_paladin_holy,
	["paladin_retri"] = config_paladin_retri,
	["druid_balance"] = config_druid_balance,
	["mage_fire"] = config_mage_fire,
	["mage_frost"] = config_mage_frost,
    ["priest_holy"] = config_priest_holy,
	["priest_shadow"] = config_priest_shadow,
	["shaman_elem"] = config_shaman_elem,
    ["shaman_resto"] = config_shaman_resto,
	["warlock_affli"] = config_warlock_affli,
	["warlock_sb"] = config_warlock_sb,
	["warrior_prot"] = config_warrior_prot,
	["warrior_arms"] = config_warrior_arms,
};


local function lole_setconfig(arg, modes) 
	if (arg == nil or arg == "") then 
		echo("lole_setconfig: erroneous argument!");
		return false;
	end
	
	conf = available_configs[arg];
	
	if conf == nil then
		echo("lole_setconfig: invalid config option \"" .. arg .. "\"! See lole.lua.");
	else
	
		LOLE_CLASS_CONFIG = conf;
		LOLE_CLASS_CONFIG_NAME = arg;
		
		if modes then
			LOLE_CLASS_CONFIG_ATTRIBS = shallowcopy(modes);
			LOLE_CLASS_CONFIG.MODE_ATTRIBS = shallowcopy(modes);
		else -- copy defaults
			LOLE_CLASS_CONFIG_ATTRIBS = shallowcopy(LOLE_CLASS_CONFIG.MODE_ATTRIBS);
		end
		
		--for key,value in pairs(LOLE_CLASS_CONFIG.MODE_ATTRIBS) do echo(key .. value) end
		echo("lole_setconfig: config set to " .. LOLE_CLASS_CONFIG.name .. ".");
	end
end

local function lole_getconfig(arg)
	local str = "nil";
	if (LOLE_CLASS_CONFIG.name ~= nil) then str = LOLE_CLASS_CONFIG.name; end
	
	echo("lole: current config: " .. str);
	echo("mode attribs:");
	for k,v in pairs(LOLE_CLASS_CONFIG.MODE_ATTRIBS) do echo(k .. ": " .. v); end
	echo("-----");
end

local function lole_cooldowns()
	if LOLE_CLASS_CONFIG.cooldowns ~= nil then
		LOLE_CLASS_CONFIG.cooldowns();
	end
end

local function get_int_from_strbool(strbool)
	local rval = -1;
	if strbool ~= nil then
		if strbool == "on" then
			rval = 1;
		elseif strbool == "off" then
			rval = 0;
		end
	end
	
	return rval;
end

local function lole_followme() 
	SendAddonMessage("lole_followme", cipher_GUID(UnitGUID("player")), "PARTY")
end

local function lole_stopfollow()
	SendAddonMessage("lole_stopfollow", nil, "PARTY")
end

local function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		-- TODO: maybe take these directly from the table
		echo("lole_set: no argument! valid modes are: buffmode selfbuffmode combatbuffmode aoemode shardmode scorchmode playermode");
		return false;
	end
	
	if (LOLE_CLASS_CONFIG.MODE_ATTRIBS and LOLE_CLASS_CONFIG.MODE_ATTRIBS[attrib_name]) then
		
		local on_off_bool = get_int_from_strbool(on_off_str);
		if on_off_bool < 0 then
			echo("lole_set: invalid argument \"" .. on_off_str .. "\" for attrib " .. attrib_name .. "! (use on/off)");
			return false;
		end
	
		LOLE_CLASS_CONFIG.MODE_ATTRIBS[attrib_name] = on_off_bool;
        if attrib_name == "buffmode" then
            BUFF_TABLE_READY = false;
            if on_off_bool == 1 then
                BUFF_TIME = GetTime();
            end
        else
		    LOLE_CLASS_CONFIG_ATTRIBS[attrib_name] = on_off_bool;
        end
		echo("lole_set: attrib \"" .. attrib_name .. "\" set to " .. on_off_bool);
		return true;
		
	else
		echo("lole_set: option \"" .. attrib_name .. "\" not available for config " .. LOLE_CLASS_CONFIG.name .. ".");
		return false;
	end

end


local lole_subcommands = {
    ["lbuffcheck"] = lole_leaderbuffcheck;
	["buffcheck"] = lole_buffcheck;
	["cooldowns"] = lole_cooldowns;
	["setconfig"] = lole_setconfig;
	["getconfig"] = lole_getconfig;
	["followme"] = lole_followme;
	["stopfollow"] = lole_stopfollow;
	["set"] = lole_set;
}
	
local function usage()
	echo("|cFFFFFF00/lole usage: /lole subcmd subcmd_arg");
	
	local subcmds_concatd = "";
	
	for subcommand, _ in pairs(lole_subcommands) do
		subcmds_concatd = subcmds_concatd .. subcommand .. ", "
	end
	
	echo(" - Available subcommands are: |cFFFFFF00\n" .. string.sub(subcmds_concatd, 1, -3));
	
	local configs_concatd = "";
	
	for config,_ in pairs(available_configs) do
		configs_concatd = configs_concatd .. config .. ", "
	end
	
	echo(" - Available class configs are: |cFFFFFF00\n" .. string.sub(configs_concatd, 1, -3));
	
	local modes_concatd = "";
	
	for mode, _ in pairs(LOLE_CLASS_CONFIG.MODE_ATTRIBS) do
		modes_concatd = modes_concatd .. mode .. ", "
	end
	
	echo(" - Available mode attributes for this config (" .. LOLE_CLASS_CONFIG.name .. ") are: |cFFFFFF00\n" .. string.sub(modes_concatd, 1, -3))
	
end


function lole_SlashCommand(args) 

	if (IsRaidLeader()) then
	  	if UnitExists("focus") and UnitIsDead("focus") then 
			lole_clear_target()
		end
	
		if BLAST_TARGET_GUID == NOTARGET then
			if not UnitExists("focus") then
				if UnitExists("target") and not UnitIsDead("target") and UnitReaction("target", "player") < 5 then
					--lole_set_target(UnitGUID("target"))
					broadcast_target_GUID(UnitGUID("target"));
				end
			else 
				-- not sure if this is reachable or not
				lole_clear_target()
			end
		end
	end
	
	if (not args or args == "") then
	
        if LOLE_CLASS_CONFIG.MODE_ATTRIBS and LOLE_CLASS_CONFIG.MODE_ATTRIBS["buffmode"] == 1 then
            lole_buffs();
        else
            if (time() - LAST_BUFF_CHECK) > 30 then
                lole_buffcheck(nil, false);
            elseif ((LOLE_CLASS_CONFIG.MODE_ATTRIBS and LOLE_CLASS_CONFIG.MODE_ATTRIBS["combatbuffmode"] == 1) or LBUFFCHECK_ISSUED) and BUFFS_CHECKED and (time() - LAST_BUFF_CHECK) > 1 then
                lole_set("buffmode", "on");
                BUFFS_CHECKED = false;
                return;
            end
            if not LOLE_CLASS_CONFIG.MODE_ATTRIBS or LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] ~= 1 then
               	if UnitExists("focus") and UnitIsDead("focus") then 
					ClearFocus()
				end
				LOLE_CLASS_CONFIG.combat();
            end
        end
        return;
	end

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
		echo("lole: error: unknown subcommand \"" .. a1 .. "\".");
		usage();
		return false;
	end
		
end

local function on_buff_check_event(self, event, ...)
    lole_buffcheck(nil, false);
end


local function OnMsgEvent(self, event, prefix, message, channel, sender)

	-- ok. so DelIgnore is hooked to do all sorts of cool stuff depending on the opcode.
	-- e.g. LOLE_OPCODE_TARGET_GUID changes the players target to the provided GUID ;) see DLL src.

    if (prefix == "lole_blast") then
		set_blast(message)
  
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

    elseif not LOLE_CLASS_CONFIG.MODE_ATTRIBS or LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] ~= 1 then
        if (prefix == "lole_target") then
			target_unit_with_GUID(message);
    		
    	elseif (prefix == "lole_follow") then
			follow_unit_with_GUID(message);
			
    	elseif (prefix == "lole_stopfollow") then
    		if not IsRaidLeader() then
    			stopfollow();
    		end
    		
    	elseif (prefix == "lole_followme") then
    		follow_unit_with_GUID(message);
    	
    	elseif (prefix == "lole_CTM_broadcast") then
			act_on_CTM_broadcast(message);
		end
    end

end

function LOLE_EventHandler(self, event, prefix, message, channel, sender) 
	--DEFAULT_CHAT_FRAME:AddMessage("LOLE_EventHandler: event:" .. event)
	if event == "PLAYER_REGEN_DISABLED" then
		SendAddonMessage("lole_opcode", LOLE_OPCODE_FOLLOW, "PARTY");
	--elseif event == "PLAYER_REGEN_ENABLED" then -- this is kinda crap, remove
		--if IsRaidLeader() then
		--	SendAddonMessage("lole_follow", tostring(UnitGUID("player")), "PARTY");
		--end
	end

end

local buff_check_frame = CreateFrame("Frame");
buff_check_frame:RegisterEvent("PLAYER_ALIVE");
buff_check_frame:RegisterEvent("PLAYER_ENTERING_WORLD");
buff_check_frame:SetScript("OnEvent", on_buff_check_event);

local msg_frame = CreateFrame("Frame");
msg_frame:RegisterEvent("CHAT_MSG_ADDON");
msg_frame:SetScript("OnEvent", OnMsgEvent);

local lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:SetScript("OnEvent", LOLE_EventHandler);

local frame = CreateFrame("frame");
	frame:SetScript("OnEvent", function(self, event, arg1)
	if (event == "ADDON_LOADED" and arg1 == "lole") then
		if LOLE_CLASS_CONFIG_NAME ~= nil then
			lole_setconfig(LOLE_CLASS_CONFIG_NAME, LOLE_CLASS_CONFIG_ATTRIBS);
		else
			lole_setconfig("default");
		end
	end
end)

frame:RegisterEvent("ADDON_LOADED");

function lole_OnLoad()
	SLASH_LOLEXDD1= "/lole";
	SlashCmdList["LOLEXDD"] = lole_SlashCommand;
end
