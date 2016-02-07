local available_configs = {
	default = create_class_config("default", {}, "FFFFFF", function() end, {}, 0),
	druid_resto = create_class_config("druid_resto", {}, CLASS_COLORS["druid"], druid_resto_combat, {}, ROLES.caster),
	druid_balance = create_class_config("druid_balance", {"Moonkin Form"}, CLASS_COLORS["druid"], druid_balance_combat, {"Barkskin"}, ROLES.caster),
	mage_fire = create_class_config("mage_fire", {"Molten Armor"}, CLASS_COLORS["mage"], mage_fire_combat, {"Icy Veins", "Combustion"}, ROLES.caster),
	mage_frost = create_class_config("mage_frost", {"Molten Armor"}, CLASS_COLORS["mage"], mage_frost_combat, {"Icy Veins"}, ROLES.caster),
	paladin_prot = create_class_config("paladin_prot", {"Devotion Aura", "Righteous Fury"}, CLASS_COLORS["paladin"], paladin_prot_combat, {"Avenging Wrath"}, ROLES.tank),
	paladin_holy = create_class_config("paladin_holy", {"Concentration Aura"}, CLASS_COLORS["paladin"], paladin_holy_combat, {"Divine Favor", "Divine Illumination"}, ROLES.healer),
	paladin_retri = create_class_config("paladin_retri", {"Sanctity Aura"}, CLASS_COLORS["paladin"], paladin_retri_combat, {"Avenging Wrath"}, ROLES.melee),
    priest_holy = create_class_config("priest_holy", {"Inner Fire"}, CLASS_COLORS["priest"], priest_holy_combat, {"Inner Focus"}, ROLES.healer),
	priest_shadow = create_class_config("priest_shadow", {"Shadow Form", "Inner Fire"}, CLASS_COLORS["priest"], priest_shadow_combat, {"Inner Focus"}, ROLES.caster),
	shaman_elem = create_class_config("shaman_elem", {"Water Shield"}, CLASS_COLORS["shaman"], shaman_elem_combat, {"Bloodlust", "Elemental Mastery"}, ROLES.caster),
    shaman_resto = create_class_config("shaman_resto", {"Water Shield"}, CLASS_COLORS["shaman"], shaman_resto_combat, {"Bloodlust"}, ROLES.healer),
	warlock_affli = create_class_config("warlock_affli", {"Fel Armor"}, CLASS_COLORS["warlock"], warlock_affli_combat, {}, ROLES.caster),
	warlock_sb = create_class_config("warlock_sb", {"Fel Armor"}, CLASS_COLORS["warlock"], warlock_sb_combat, {}, ROLES.caster),
	warrior_prot = create_class_config("warrior_prot", {"Commanding Shout"}, CLASS_COLORS["warrior"], warrior_prot_combat, {"Last Stand"}, ROLES.tank),
	warrior_arms = create_class_config("warrior_arms", {"Battle Shout"}, CLASS_COLORS["warrior"], warrior_arms_combat, {"Death Wish"}, ROLES.melee),
};


local mode_attribs = {
	playermode = 0,
	buffmode = 0,
	combatbuffmode = 0,
	aoemode = 0
}

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

		set_visible_dropdown_config(arg) -- gui stuff
		
		if modes then
			LOLE_CLASS_CONFIG_ATTRIBS_SAVED = shallowcopy(modes);
			LOLE_CLASS_CONFIG = shallowcopy(modes);
		else -- copy defaults
			LOLE_CLASS_CONFIG_ATTRIBS_SAVED = shallowcopy(mode_attribs);
		end
		
		echo("lole_setconfig: config set to " .. LOLE_CLASS_CONFIG.name .. ".");
	end
	
	return true;
end


local function lole_getconfig(arg)
	local str = "nil";
	if (LOLE_CLASS_CONFIG.name ~= nil) then str = LOLE_CLASS_CONFIG.name; end
	
	echo("lole: current config: |cFFFFFF00" .. str);
	echo("mode attribs:");
	for k,v in pairs(mode_attribs) do echo("|cFFFFFF00" .. k .. ": " .. v); end
	
	return false;
end

local function lole_followme() 
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, cipher_GUID(UnitGUID("player")))
	return true;
end

local function lole_stopfollow()
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, cipher_GUID(NOTARGET)) -- still needs the cipher.. :D
	return true;
end

local function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes for config " .. LOLE_CLASS_CONFIG.name .. " are: |cFFFFFF00" .. get_config_mode_attribs(LOLE_CLASS_CONFIG));
		return false;
	end
	
	if modes[attrib_name] then
		
		local on_off_bool = get_int_from_strbool(on_off_str);
		if on_off_bool < 0 then
			echo("lole_set: invalid argument \"" .. on_off_str .. "\" for attrib " .. attrib_name .. "! (use on/off)");
			return false;
		end
	
		modes[attrib_name] = on_off_bool;
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

local function lole_get(attrib_name)
	
	if not attrib_name or attrib_name == "" then
		return mode_attribs; -- return all attribs
		
	elseif (mode_attribs[attrib_name])
		return mode_attribs[attrib_name];
	end
	
	return nil;

end

local function lole_debug_dump_wowobjects()
	DelIgnore(LOLE_DEBUG_OPCODE_DUMP);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log (if you're injected!) ;)")
	return true;
end

local function lole_blast(arg)
	if (arg == "on" or arg == "1") then
		send_opcode_addonmsg(LOLE_OPCODE_BLAST, "1");
	elseif (arg == "off" or arg == "0") then
 		send_opcode_addonmsg(LOLE_OPCODE_BLAST, "0");
	else
		lole_error("lole_blast: need an argument (valid arguments: \"on\" / \"1\" or \"off\" / \"0\")");
		return false;
	end

	return true;
	
end

local function lole_ctm(arg)
	local mode = get_CTM_mode();
	
	echo("calling lole ctm with arg " .. arg)
	
	-- could consider a jump table here
	
	if mode == CTM_MODES.LOCAL then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. arg);	
				
	elseif mode == CTM_MODES.TARGET then
		local target_GUID = UnitGUID("target")
		if not target_GUID then return end
		
		echo("sending to target " .. target_GUID)
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
	
	return true;
end

local function lole_gui()
	main_frame_show()
end

local function lole_cooldowns()
	UseInventoryItem(13);
	UseInventoryItem(14);
	
	local _, race = UnitRace("player");
	
	if race == "Orc" then
		CastSpellByName("Blood Fury")
	elseif race == "Troll" then
		CastSpellByName("Berserking")
	end
	
	for spell in LOLE_CLASS_CONFIG.cooldowns do
		SpellStopCasting()
		CastSpellByName(spell);
	end
	
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
	get = lole_get;
	blast = lole_blast;
	ctm = lole_ctm;
	cooldowns = lole_cooldowns;
	
	gui = lole_gui;
	
	dump = lole_debug_dump_wowobjects;
}


