local LOLE_CLASS_CONFIG = "default";
local LOLE_BLAST_STATE = nil;

local available_configs = {
	default =
	class_config_create("default", {}, {}, "FFFFFF", function() end, {}, 0),

	druid_feral =
	class_config_create("druid_feral", {"Mark of the Wild", "Thorns"}, {}, get_class_color("druid"), combat_druid_feral, {"Omen of Clarity"}, ROLES.melee),

	druid_resto =
	class_config_create("druid_resto", {"Mark of the Wild", "Thorns"}, {}, get_class_color("druid"), combat_druid_resto, {}, ROLES.healer),

	druid_balance =
	class_config_create("druid_balance", {"Mark of the Wild", "Thorns"}, {"Moonkin Form"}, get_class_color("druid"), combat_druid_balance, {"Barkskin"}, ROLES.caster),

	hunter =
	class_config_create("hunter", {}, {}, get_class_color("hunter"), combat_hunter, {"Bestial Wrath", "Rapid Fire"}, ROLES.melee),

	mage_fire =
	class_config_create("mage_fire", {"Arcane Intellect", "Amplify Magic"}, {"Molten Armor"}, get_class_color("mage"), combat_mage_fire, {"Icy Veins", "Combustion"}, ROLES.caster),

	mage_frost =
	class_config_create("mage_frost", {"Arcane Intellect", "Amplify Magic"}, {"Molten Armor"}, get_class_color("mage"), combat_mage_frost, {"Icy Veins"}, ROLES.caster),

	paladin_prot =
	class_config_create("paladin_prot", {}, {"Devotion Aura", "Righteous Fury"}, get_class_color("paladin"), combat_paladin_prot, {}, ROLES.paladin_tank),

	paladin_holy =
	class_config_create("paladin_holy", {}, {"Concentration Aura"}, get_class_color("paladin"), combat_paladin_holy, {"Divine Favor", "Divine Illumination"}, ROLES.healer),

	paladin_retri =
	class_config_create("paladin_retri", {}, {"Sanctity Aura"}, get_class_color("paladin"), combat_paladin_retri, {"Avenging Wrath"}, ROLES.melee),

	priest_holy =
	class_config_create("priest_holy", {"Power Word: Fortitude", "Shadow Protection"}, {"Inner Fire"}, get_class_color("priest"), combat_priest_holy, {"Inner Focus"}, ROLES.healer),

    priest_holy_ds =
    class_config_create("priest_holy_ds", {"Power Word: Fortitude", "Divine Spirit", "Shadow Protection"}, {"Inner Fire"}, get_class_color("priest"), combat_priest_holy, {"Inner Focus"}, ROLES.healer),

	priest_shadow =
	class_config_create("priest_shadow", {"Power Word: Fortitude", "Shadow Protection"}, {"Shadowform", "Inner Fire"}, get_class_color("priest"), combat_priest_shadow, {"Inner Focus"}, ROLES.caster),

	rogue_combat =
	class_config_create("rogue_combat", {}, {}, get_class_color("rogue"), combat_rogue_combat, {"Adrenaline Rush", "Blade Flurry"}, ROLES.melee),

	shaman_enh =
	class_config_create("shaman_enh", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_enh, {"Bloodlust", "Shamanistic Rage"}, ROLES.melee),

	shaman_elem =
	class_config_create("shaman_elem", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_elem, {"Bloodlust", "Elemental Mastery"}, ROLES.caster),

	shaman_resto =
	class_config_create("shaman_resto", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_resto, {"Bloodlust"}, ROLES.healer),

	warlock_affli =
	class_config_create("warlock_affli", {}, {"Fel Armor"}, get_class_color("warlock"), combat_warlock_affli, {}, ROLES.caster),

	warlock_sb =
	class_config_create("warlock_sb", {}, {"Fel Armor"}, get_class_color("warlock"), combat_warlock_sb, {}, ROLES.caster),

	warrior_prot =
	class_config_create("warrior_prot", {}, {"Commanding Shout"}, get_class_color("warrior"), combat_warrior_prot, {}, ROLES.warrior_tank),

	warrior_arms =
	class_config_create("warrior_arms", {}, {"Battle Shout"}, get_class_color("warrior"), combat_warrior_arms, {"Death Wish"}, ROLES.melee),

	--mage_aespam =
	--class_config_create("mage_aespam", {}, {}, get_class_color("mage"), function() CastSpellByName("Arcane Explosion") end, {}, ROLES.caster),
};


local mode_attribs = {
	playermode = 0,
	buffmode = 0,
	combatbuffmode = 0,
	aoemode = 0
}

function get_available_configs()
	return available_configs;
end

function get_current_config()
	return available_configs[LOLE_CLASS_CONFIG]
end

function get_blast_state()
	return LOLE_BLAST_STATE
end

function set_blast_state(state)
	LOLE_BLAST_STATE = state
end

local function lole_setconfig(arg, modes)
	if (arg == nil or arg == "") then
		echo("lole_setconfig: erroneous argument!");
		return false;
	end

	conf = available_configs[arg];

	if not conf then
		echo("lole_setconfig: invalid config option \"" .. arg .. "\"! See lole.lua.");
		return false;
	else

		LOLE_CLASS_CONFIG = arg;
		LOLE_CLASS_CONFIG_NAME_SAVED = arg;

		set_visible_dropdown_config(arg) -- gui stuff

		if modes then
			mode_attribs = shallowcopy(modes);
			LOLE_CLASS_CONFIG_ATTRIBS_SAVED = shallowcopy(modes);

		else -- copy defaults
			LOLE_CLASS_CONFIG_ATTRIBS_SAVED = shallowcopy(mode_attribs);
		end

		echo("lole_setconfig: config set to " .. get_current_config().name .. ".");
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
	broadcast_follow_target(UnitGUID("player"))
	return true;
end

local function lole_stopfollow()
	broadcast_follow_target(NOTARGET) -- still needs the cipher.. :D
	return true;
end

local function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes for config " .. LOLE_CLASS_CONFIG.name .. " are: |cFFFFFF00" .. get_config_mode_attribs(LOLE_CLASS_CONFIG));
		return false;
	end

	if mode_attribs[attrib_name] then

		local on_off_bool = get_int_from_strbool(on_off_str);
		if on_off_bool < 0 then
			echo("lole_set: invalid argument \"" .. on_off_str .. "\" for attrib " .. attrib_name .. "! (use 1/on | 0/off)");
			return false;
		end

		mode_attribs[attrib_name] = on_off_bool;
        if attrib_name == "buffmode" then
            BUFF_TABLE_READY = false;
            if on_off_bool == 1 then
                BUFF_TIME = GetTime();
            end
        else
		    LOLE_CLASS_CONFIG_ATTRIBS_SAVED[attrib_name] = on_off_bool;
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

	elseif (mode_attribs[attrib_name]) then
		return mode_attribs[attrib_name];
	end

	return nil;

end

local function lole_blast(arg)
	if (arg == "on" or arg == "1") then
		broadcast_blast_state(1);
	elseif (arg == "off" or arg == "0") then
 		broadcast_blast_state(0);
	else
		lole_error("lole_blast: need an argument (valid arguments: \"on\" / \"1\" or \"off\" / \"0\")");
		return false;
	end

	return true;

end

local function lole_ctm(arg)
	local mode = get_CTM_mode();

	broadcast_CTM(mode, arg)

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

	for _, spell in pairs(get_current_config().cooldowns) do
		SpellStopCasting()
		CastSpellByName(spell);
	end

end

local function do_buffs(missing_buffs)

	if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP, buffs = {}, {}
		local config_name = get_current_config().name;

        if need_to_buff() then
            if UnitClass("player") == "Paladin" then
    		    local paladins = get_chars_of_class("Paladin");
    		    if #paladins == 1 then
    		        buffs["Greater Blessing of Kings"] = missing_buffs["Blessing of Kings"];
    	            buffs["Greater Blessing of Salvation"] = missing_buffs["Blessing of Salvation"];
    	            buffs["Greater Blessing of Wisdom"] = missing_buffs["Blessing of Wisdom"];
    	            buffs["Greater Blessing of Might"] = missing_buffs["Blessing of Might"];
                elseif #paladins == 2 then
                    if config_name == "paladin_holy" or (config_name == "paladin_retri" and table.contains(paladins, "Adieux")) then
                        buffs["Greater Blessing of Salvation"] = missing_buffs["Blessing of Salvation"];
                        buffs["Greater Blessing of Wisdom"] = missing_buffs["Blessing of Wisdom"];
                        buffs["Greater Blessing of Might"] = missing_buffs["Blessing of Might"];
        	        else
        	        	buffs["Greater Blessing of Kings"] = missing_buffs["Blessing of Kings"];
        	        end
                elseif #paladins == 3 then
                    if config_name == "paladin_prot" then
                        buffs["Greater Blessing of Kings"] = missing_buffs["Blessing of Kings"];
                    elseif config_name == "paladin_retri" then
                        buffs["Greater Blessing of Salvation"] = missing_buffs["Blessing of Salvation"];
                        buffs["Greater Blessing of Light"] = missing_buffs["Blessing of Light"];
                    else
                        buffs["Greater Blessing of Wisdom"] = missing_buffs["Blessing of Wisdom"];
                        buffs["Greater Blessing of Might"] = missing_buffs["Blessing of Might"];
                    end
                end
           	else
                local config_buffs = get_current_config().buffs;
                if config_name == "priest_holy_ds" then
                    local priests = get_chars_of_class("Priest");
                    if #priests > 1 then
                        config_buffs = {"Divine Spirit"};
                    end
                end
    			for k, buff in pairs(config_buffs) do
    				if BUFF_ALIASES[buff] then
    					GROUP_BUFF_MAP[buff] = BUFF_ALIASES[buff];
    				end
    				if missing_buffs[buff] then
    					buffs[buff] = missing_buffs[buff]
    				end
    			end

    			if config_name == "druid_balance" and buffs["Mark of the Wild"] ~= nil then
    	            SELF_BUFF_SPAM_TABLE[1] = "Moonkin Form";
                elseif config_name == "druid_resto" and (buffs["Mark of the Wild"] ~= nil or buffs["Thorns"] ~= nil) then
                    SELF_BUFF_SPAM_TABLE[1] = "Tree of Life";
                elseif config_name == "druid_feral" and (buffs["Mark of the Wild"] ~= nil or buffs["Thorns"] ~= nil) then
                    SELF_BUFF_SPAM_TABLE[1] = "Cat Form";
    	        end
    	    end

            local num_requests = get_num_buff_requests(buffs);
            if num_requests > 0 then
            	if UnitClass("player") == "Paladin" then
            		SPAM_TABLE = get_paladin_spam_table(buffs, num_requests);
            	else
                	SPAM_TABLE = get_spam_table(buffs, GROUP_BUFF_MAP);
                end
            end
        end

		BUFF_TABLE_READY = true;
    end

    if SPAM_TABLE[1] ~= nil then
        if (GetTime() - BUFF_TIME) < 1.8 then
            return false;
        else
            local char, buff = next(SPAM_TABLE[1]);
            CastSpellByName(buff, char);
            BUFF_TIME = GetTime();
            table.remove(SPAM_TABLE, 1);
        end
    elseif SELF_BUFF_SPAM_TABLE[1] ~= nil then
        buff_self();
    else
        MISSING_BUFFS = {};
        lole_set("buffmode", 0);
        LBUFFCHECK_ISSUED = false;
    end

end

local function lole_drink()

	if UnitPowerType("player") == 0 then -- 0 for mana

		if UnitMana("player")/UnitManaMax("player") < 0.90 then
			if GetItemCount(34062) > 0 then
				UseItemByName("Conjured Manna Biscuit");

			elseif GetItemCount(27860) > 0 then
				UseItemByName("Purified Draenic Water");

			elseif GetItemCount(32453) > 0 then
				UseItemByName("Star's Tears");
			else
				SendChatMessage("I'm out of mana drinks! Giev plx.", "GUILD")
			end
		end
	end
end

local raid_first_pass = true

local function lole_raid()


	if GetNumPartyMembers() == 0 then
		raid_first_pass = true
	else
		-- this always makes a raid, since GuildRoster is fucked up beyond belief..
		ConvertToRaid()
		raid_first_pass = false
	end

	local guildies = get_guild_members()

	if raid_first_pass then -- inv first guy, because more ppl can only be invited when a group exists
		lole_frame:RegisterEvent("PARTY_MEMBERS_CHANGED");
		for name, _ in pairs(guildies) do
			InviteUnit(name);
			break;
		end
		raid_first_pass = false

	else
		for name, _ in pairs(guildies) do
			InviteUnit(name)
		end
		raid_first_pass = true
	end

end

local function lole_release()
	release_spirit_all()
end


local function lole_leaveparty()
	leave_party_all()
end

local function lole_maintank(arg)
	broadcast_main_tank(arg)
end

local function lole_clearcc()
	disable_all_cc_targets()
end

local function lole_pull(arg)
	--send_opcode_addonmsg_to(LOLE_OPCODE_PULL_MOB, arg, MAIN_TANK);
--	lole_debug_pull_test()
	target_best_CH_target()
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
	buffs = do_buffs;
	gui = lole_gui;
	drink = lole_drink;
	raid = lole_raid;
	party = lole_party;
	leaveparty = lole_leaveparty;
	release = lole_release;
	mt = lole_maintank;
	clearcc = lole_clearcc;
	pull = lole_pull;

	dump = lole_debug_dump_wowobjects;
	loot = lole_debug_loot_all;
}
