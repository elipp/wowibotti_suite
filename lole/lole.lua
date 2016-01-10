LOLE_CLASS_CONFIG_NAME = "default";
LOLE_CLASS_CONFIG_ATTRIBS = nil; -- listed in SavedVariablesPerCharacter

local DEFAULT_CONFIG = { name = "default", MODE_ATTRIBS = nil, desired_buffs = function() return {}; end, combat = function() end, buffs = function() end, other = function() end };
LOLE_CLASS_CONFIG = DEFAULT_CONFIG;
BUFF_TIME = 0;
LAST_BUFF_CHECK = 0;
SPAM_TABLE = {};
SELF_BUFF_SPAM_TABLE = {};
BUFFS_CHECKED = false;
LAST_LBUFFCHECK = 0;
LBUFFCHECK_ISSUED = false;

-- opcodes for DelIgnore :P
LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID, 
LOLE_OPCODE_BLAST, 
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to the target
LOLE_OPCODE_CASTER_FACE,
LOLE_OPCODE_CTM_BROADCAST

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04", "LOP_05", "LOP_06";


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

local function cipher_GUID(GUID)
	local part1 = tonumber(string.sub(GUID, 3, 10), 16); -- the GUID string still has the 0x part in it
	local part2 = tonumber(string.sub(GUID, 11), 16);

	--DEFAULT_CHAT_FRAME:AddMessage("part 1: " .. string.format("%08X", part1) .. ", part 2: " .. string.format("%08X", part2));

	local XOR_mask1 = 0xAB0AB03F; -- just some arbitrary constants
	local XOR_mask2 = 0xEBAEBA55;

	local xor1 = string.format("%08X", bit.bxor(part1, XOR_mask1));
	local xor2 = string.format("%08X", bit.bxor(part2, XOR_mask2));

	--DEFAULT_CHAT_FRAME:AddMessage("XOR'd 1: " .. xor1 .. ", XOR'd 2: " .. xor2);

	return "0x" .. xor1 .. xor2;
end

local function decipher_GUID(ciphered)
-- this works because XOR is reversible ^^
	return cipher_GUID(ciphered);
end


local function lole_buffs()
    if LOLE_CLASS_CONFIG.MODE_ATTRIBS["selfbuffmode"] ~= nil and LOLE_CLASS_CONFIG.MODE_ATTRIBS["selfbuffmode"] == 1 then
        lole_selfbuffs();
    else
        if (LOLE_CLASS_CONFIG.buffs ~= nil) then
            local MISSING_BUFFS_COPY;
            if not BUFF_TABLE_READY then
                MISSING_BUFFS_COPY = shallowcopy(MISSING_BUFFS);
            end
            LOLE_CLASS_CONFIG.buffs(MISSING_BUFFS_COPY);
        end
    end
end

local function lole_leaderbuffcheck(arg)

    if arg ~= nil and arg ~= "clean" then 
		echo("lole_leaderbuffcheck: erroneous argument!");
		return false;
	end

    if not IsRaidLeader() then
        echo("lole_leaderbuffcheck: You're not the raid leader, asshole.");
        return false;
    end

    local msgstr = "buffcheck";
    if arg == "clean" then
        msgstr = msgstr .. " " .. arg;
    end
    SendAddonMessage("lole_buffcheck", msgstr, "RAID", UnitName("player"));

end

local function lole_buffcheck(arg)

    if arg ~= nil and arg ~= "clean" then 
		echo("lole_buffcheck: erroneous argument!");
		return false;
	end

    local buffname_timeleft_map = {}
    
    for i=1,32 do id,cancel = GetPlayerBuff(i,"HELPFUL") -- |HARMFUL|PASSIVE"); -- not needed really
        if (id > 0) then
            local name = GetPlayerBuffName(id);
            local timeleft = GetPlayerBuffTimeLeft(id);
            if cancel == 1 then
                timeleft = 1000;
            end
            buffname_timeleft_map[name] = timeleft;
        end
    end
   
    local missing_table = {};

    if arg == "clean" then
        echo("lole_buffcheck: requested full rebuffage");
        missing_table = LOLE_CLASS_CONFIG.desired_buffs();
    else
        local desired_buffs = LOLE_CLASS_CONFIG.desired_buffs();
        for i,bname in ipairs(desired_buffs) do
            local bname_alias = BUFF_ALIASES[bname];
            
            if buffname_timeleft_map[bname] ~= nil then
                if buffname_timeleft_map[bname] < 180 then
                    table.insert(missing_table, bname);
                end
            elseif bname_alias ~= nil and buffname_timeleft_map[bname_alias] ~= nil then
                if buffname_timeleft_map[bname_alias] < 180 then
                    table.insert(missing_table, bname); -- still use base name
                end
            else
                table.insert(missing_table, bname);
            end
        end
    end

    if LOLE_CLASS_CONFIG.SELF_BUFFS ~= nil then
        SELF_BUFF_SPAM_TABLE = {};
        for i,bname in ipairs(LOLE_CLASS_CONFIG.SELF_BUFFS) do
            if arg == "clean" then
                if buffname_timeleft_map[bname] == 1000 then
                else 
                    table.insert(SELF_BUFF_SPAM_TABLE, bname);
                end
            else
                if buffname_timeleft_map[bname] ~= nil then
                    if buffname_timeleft_map[bname] < 180 then
                        table.insert(SELF_BUFF_SPAM_TABLE, bname);
                    end
                else
                    table.insert(SELF_BUFF_SPAM_TABLE, bname);
                end
            end
        end
    end
	
	local msgstr = table.concat(missing_table, ",");
     
    SendAddonMessage("lole_buffs", msgstr, "RAID", UnitName("player"));
    LAST_BUFF_CHECK = time();
    BUFFS_CHECKED = true;

end

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
	
	-- rval = -1; -- should already be -1
	
	return rval;
end

local function lole_followme() 
	SendAddonMessage("lole_followme", UnitGUID("player"), "PARTY")
end

local function lole_stopfollow()
	SendAddonMessage("lole_stopfollow", nil, "PARTY")
end

local function lole_set(attrib_name, on_off_str) 

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes are: buffmode selfbuffmode combatbuffmode aoemode shardmode scorchmode combatmode");
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
	echo("/lole usage: /lole subcmd subcmd_arg");
	echo("available subcmds are:");
    echo(" lbuffcheck");
	echo(" buffcheck");
	echo(" cooldowns");
	echo(" setconfig");
	echo(" getconfig");
	echo(" set <mode> on|off");
	echo("   available modes are: buffmode combatbuffmode selfbuffmode aoemode shardmode scorchmode combatmode");
end


function lole_SlashCommand(args) 

	if (IsRaidLeader()) then
		local target_GUID = UnitGUID("target");
		if target_GUID and not UnitIsDead("target") and UnitReaction("target", "player") < 5 then
				local ciphered = cipher_GUID(target_GUID);
				SendAddonMessage("lole_target", tostring(ciphered), "PARTY");
		end
	end
	if (not args or args == "") then
        if LOLE_CLASS_CONFIG.MODE_ATTRIBS and LOLE_CLASS_CONFIG.MODE_ATTRIBS["buffmode"] == 1 then
            lole_buffs();
        else
            if (time() - LAST_BUFF_CHECK) > 30 then
                lole_buffcheck();
            elseif ((LOLE_CLASS_CONFIG.MODE_ATTRIBS and LOLE_CLASS_CONFIG.MODE_ATTRIBS["combatbuffmode"] == 1) or LBUFFCHECK_ISSUED) and BUFFS_CHECKED and (time() - LAST_BUFF_CHECK) > 1 then
                lole_set("buffmode", "on");
                BUFFS_CHECKED = false;
                return;
            end
            LOLE_CLASS_CONFIG.combat();
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
		echo("lole: unknown subcommand \"" .. a1 .. "\".");
		usage();
		return false;
	end
		
end

local function on_buff_check_event(self, event, ...)
    lole_buffcheck();
end


MISSING_BUFFS = {};


local lole_target = "0x0000000000000000";

local function OnMsgEvent(self, event, prefix, message, channel, sender)

	-- ok. so DelIgnore is hooked to do all sorts of cool stuff depending on the opcode.
	-- e.g. LOLE_OPCODE_TARGET_GUID changes the players target to the provided GUID ;) see DLL src.

    if (prefix == "lole_target") then
		local GUID_deciphered = decipher_GUID(message);
		if (lole_target ~= GUID_deciphered) then
			DelIgnore(LOLE_OPCODE_TARGET_GUID .. ":" .. GUID_deciphered); 
			lole_target = GUID_deciphered;
		end
		
	elseif (prefix == "lole_blast") then
		DelIgnore(LOLE_OPCODE_BLAST .. ":" .. message);	
		
	elseif (prefix == "lole_follow") then
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. message);
		
	elseif (prefix == "lole_stopfollow") then
		if not IsRaidLeader() then
			stopfollow();
		end
		
	elseif (prefix == "lole_followme") then
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. message);
	
	elseif (prefix == "lole_CTM_broadcast") then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. message);
		
    elseif (prefix == "lole_buffs") then
        --echo(message .. " -" .. sender .. "-");
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
                lole_buffcheck();
            elseif message == "buffcheck clean" then
                echo("lole: The raid leader issued a clean buffcheck.");
                lole_buffcheck("clean");
            end
            LAST_LBUFFCHECK = time();
            LBUFFCHECK_ISSUED = true;
        end
    end
end

function LOLE_EventHandler(self, event, prefix, message, channel, sender) 
	--DEFAULT_CHAT_FRAME:AddMessage("LOLE_EventHandler: event:" .. event)
	if event == "PLAYER_REGEN_DISABLED" then
		SendAddonMessage("lole_stopfollow", nil, "PARTY");
	elseif event == "PLAYER_REGEN_ENABLED" then
		if IsRaidLeader() then
			SendAddonMessage("lole_follow", tostring(UnitGUID("player")), "PARTY");
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
