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
	
	return true;
end


local function lole_getconfig(arg)
	local str = "nil";
	if (LOLE_CLASS_CONFIG.name ~= nil) then str = LOLE_CLASS_CONFIG.name; end
	
	echo("lole: current config: |cFFFFFF00" .. str);
	echo("mode attribs:");
	for k,v in pairs(LOLE_CLASS_CONFIG.MODE_ATTRIBS) do echo("|cFFFFFF00" .. k .. ": " .. v); end
	
	return false;
end

local function lole_cooldowns()
	if LOLE_CLASS_CONFIG.cooldowns ~= nil then
		LOLE_CLASS_CONFIG.cooldowns();
	end
	return true;
end


local function lole_followme() 
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, cipher_GUID(UnitGUID("player")))
	return true;
end

local function lole_stopfollow()
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, NOTARGET)
	return true;
end

local function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes for config " .. LOLE_CLASS_CONFIG.name .. " are: |cFFFFFF00" .. get_config_mode_attribs(LOLE_CLASS_CONFIG));
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
		echo("Valid modes for config " .. LOLE_CLASS_CONFIG.name .. " are: |cFFFFFF00" .. get_config_mode_attribs(LOLE_CLASS_CONFIG));
		return false;
	end

	return true;
end

local function lole_debug_dump_wowobjects()
	DelIgnore(LOLE_DEBUG_OPCODE_DUMP);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log! ;)")
	return true;
end


lole_subcommands = {
    lbuffcheck = lole_leaderbuffcheck;
	buffcheck = lole_buffcheck;
	cooldowns = lole_cooldowns;
	setconfig = lole_setconfig;
	getconfig = lole_getconfig;
	followme = lole_followme;
	stopfollow = lole_stopfollow;
	set = lole_set;
	
	dump = lole_debug_dump_wowobjects;
}


