local LOLE_CLASS_CONFIG = "default";

local available_configs = {
	default =
	class_config_create("default", {}, {}, "FFFFFF", function() end, {}, 0, "NONE"),

	druid_feral =
	class_config_create("druid_feral", {"Mark of the Wild", "Thorns"}, {"Omen of Clarity", "Cat Form"}, get_class_color("druid"), combat_druid_feral, {}, ROLES.mana_melee, "MELEE"),

	druid_resto =
	class_config_create("druid_resto", {"Mark of the Wild", "Thorns"}, {"Tree of Life"}, get_class_color("druid"), combat_druid_resto, {}, ROLES.healer, "HEALER"),

	druid_balance =
	class_config_create("druid_balance", {"Mark of the Wild", "Thorns"}, {"Moonkin Form"}, get_class_color("druid"), combat_druid_balance, {"Barkskin"}, ROLES.caster, "RANGED"),

	hunter =
	class_config_create("hunter", {}, {}, get_class_color("hunter"), combat_hunter, {"Bestial Wrath", "Rapid Fire"}, ROLES.mana_melee, "RANGED"),

	mage_fire =
	class_config_create("mage_fire", {"Arcane Intellect", "Amplify Magic"}, {"Molten Armor"}, get_class_color("mage"), combat_mage_fire, {"Icy Veins", "Combustion"}, ROLES.caster, "RANGED"),

	mage_frost =
	class_config_create("mage_frost", {"Arcane Intellect", "Amplify Magic"}, {"Molten Armor"}, get_class_color("mage"), combat_mage_frost, {"Icy Veins"}, ROLES.caster, "RANGED"),

	paladin_prot =
	class_config_create("paladin_prot", {}, {"Devotion Aura", "Righteous Fury"}, get_class_color("paladin"), combat_paladin_prot, {}, ROLES.paladin_tank, "TANK"),

	paladin_holy =
	class_config_create("paladin_holy", {}, {"Concentration Aura"}, get_class_color("paladin"), combat_paladin_holy, {"Divine Favor", "Divine Illumination"}, ROLES.healer, "HEALER"),

	paladin_retri =
	class_config_create("paladin_retri", {}, {"Sanctity Aura"}, get_class_color("paladin"), combat_paladin_retri, {"Avenging Wrath"}, ROLES.mana_melee, "MELEE"),

	priest_holy =
	class_config_create("priest_holy", {"Power Word: Fortitude", "Shadow Protection"}, {"Inner Fire"}, get_class_color("priest"), combat_priest_holy, {"Inner Focus"}, ROLES.healer, "HEALER"),

	priest_holy_ds =
	class_config_create("priest_holy_ds", {"Power Word: Fortitude", "Divine Spirit", "Shadow Protection"}, {"Inner Fire"}, get_class_color("priest"), combat_priest_holy, {"Inner Focus"}, ROLES.healer, "HEALER"),

	priest_shadow =
	class_config_create("priest_shadow", {"Power Word: Fortitude", "Shadow Protection"}, {"Shadowform", "Inner Fire"}, get_class_color("priest"), combat_priest_shadow, {"Inner Focus"}, ROLES.caster, "RANGED"),

	rogue_combat =
	class_config_create("rogue_combat", {}, {}, get_class_color("rogue"), combat_rogue_combat, {"Adrenaline Rush", "Blade Flurry"}, ROLES.melee, "MELEE"),

	shaman_enh =
	class_config_create("shaman_enh", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_enh, {"Bloodlust", "Shamanistic Rage"}, ROLES.mana_melee, "MELEE"),

	shaman_elem =
	class_config_create("shaman_elem", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_elem, {"Bloodlust", "Elemental Mastery"}, ROLES.caster, "RANGED"),

	shaman_resto =
	class_config_create("shaman_resto", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_resto, {"Bloodlust"}, ROLES.healer, "HEALER"),

	warlock_affli =
	class_config_create("warlock_affli", {}, {"Fel Armor"}, get_class_color("warlock"), combat_warlock_affli, {}, ROLES.caster, "RANGED"),

	warlock_sb =
	class_config_create("warlock_sb", {}, {"Fel Armor"}, get_class_color("warlock"), combat_warlock_sb, {}, ROLES.caster, "RANGED"),

	warrior_fury =
	class_config_create("warrior_fury", {}, {"Battle Shout"}, get_class_color("warrior"), combat_warrior_fury, {"Recklessness"}, ROLES.melee, "MELEE"),

	warrior_arms =
	class_config_create("warrior_arms", {}, {"Battle Shout"}, get_class_color("warrior"), combat_warrior_arms, {"Death Wish"}, ROLES.melee, "MELEE"),

	warrior_prot =
	class_config_create("warrior_prot", {}, {"Commanding Shout"}, get_class_color("warrior"), combat_warrior_prot, {}, ROLES.warrior_tank, "TANK"),
	--mage_aespam =
	--class_config_create("mage_aespam", {}, {}, get_class_color("mage"), function() CastSpellByName("Arcane Explosion") end, {}, ROLES.caster),
};

local mode_attribs = {
	blast = 0,
	heal_blast = 0,
	playermode = 0,
	buffmode = 0,
	combatbuffmode = 0,
	aoemode = 0,
}

function get_available_configs()
	return available_configs;
end

function get_current_config()
	return available_configs[LOLE_CLASS_CONFIG]
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
	if (get_current_config().name ~= nil) then str = get_current_config().name; end

	echo("current config: |cFFFFFF00" .. str);
	echo("mode attribs:");
	for k,v in pairs(mode_attribs) do echo("|cFFFFFF00" .. k .. ": " .. v); end

	return false;
end

local function lole_followme()
	lole_subcommands.broadcast("follow", UnitName("player"))
	return true;
end

local function lole_follow(name)
	follow_unit(name)
end

local function lole_stopfollow()
	stopfollow()
end

local function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes for config " .. get_current_config().name .. " are: |cFFFFFF00" .. get_available_mode_attribs());
		return false;
	end

	if mode_attribs[attrib_name] then
		local state = get_int_from_strbool(on_off_str);
		if state < 0 then
			echo("lole_set: invalid argument \"" .. state .. "\" for attrib " .. attrib_name .. "! (use 1/on | 0/off)");
			return false;
		end

		mode_attribs[attrib_name] = state;
	if attrib_name == "buffmode" then
	  BUFF_TABLE_READY = false;
	  if state == 1 then
	      BUFF_TIME = GetTime();
	  end
	elseif attrib_name == "blast" then
		gui_set_blast(state)
	elseif attrib_name == "heal_blast" then
		gui_set_heal_blast(state)
	else
		LOLE_CLASS_CONFIG_ATTRIBS_SAVED[attrib_name] = state;
	end

		echo("lole_set: attrib \"" .. attrib_name .. "\" set to " .. state);
		return true;

	else
		echo("lole_set: attrib \"" .. attrib_name .. "\" not available!");
		echo("Valid mode attribs are: |cFFFFFF00" .. get_available_mode_attribs());
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

local CTM_PRIO_NONE = 0
local CTM_PRIO_LOW = 1
local CTM_PRIO_FOLLOW = 2
local CTM_PRIO_REPLACE = 3
local CTM_PRIO_EXCLUSIVE = 4
local CTM_PRIO_HOLD_POSITION = 5
local CTM_PRIO_CLEAR_HOLD = 6

local function lole_broadcast_ctm(x, y, z)

	local mode = get_CTM_mode();
	if mode == CTM_MODES.LOCAL then
		lole_subcommands.ctm(x, y, z);

	elseif mode == CTM_MODES.TARGET then
		local tname = UnitName("target")
		if not tname then return end

		echo("sending CTM to target " .. tname)
		lole_subcommands.sendmacro_to(tname, "/lole ctm", x, y, z, CTM_PRIO_LOW); -- last arg == priority level

		-- kinda redundant.
	elseif mode == CTM_MODES.EVERYONE then
		lole_subcommands.sendmacro("RAID", "/lole ctm", x, y, z, CTM_PRIO_LOW);

	else
		lole_error("lole_ctm: invalid mode: " .. tostring(mode));
		return false;
	end

	return true;
end

local function lole_ctm(x, y, z, prio)
	if lole_subcommands.get("playermode") == 0 then
		walk_to(tonumber(x), tonumber(y), tonumber(z), prio)
	end
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

		if UnitMana("player")/UnitManaMax("player") < 0.90 or UnitHealth("player")/UnitHealthMax("player") < 0.75 then
			if GetItemCount(34062) > 0 then
				UseItemByName("Conjured Manna Biscuit");

			elseif GetItemCount(27860) > 0 then
				UseItemByName("Purified Draenic Water");

			elseif GetItemCount(32453) > 0 then
				UseItemByName("Star's Tears");
			elseif GetItemCount(22018) > 0 then
				UseItemByName("Conjured Glacier Water");
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
	RepopMe()
end


local function lole_leavegroup()
	LeaveParty();
end

local function lole_disenchant_greeniez()

	if UnitCastingInfo("player") ~= nil then return end

	for b = 0, NUM_BAG_SLOTS do
        for s = 1,GetContainerNumSlots(b) do
            local n = GetContainerItemLink(b,s)
            if n and strfind(n,"ff1eff00") then
        		itemName, itemLink, itemRarity, itemLevel, itemMinLevel, itemType, itemSubType, itemStackCount,
                itemEquipLoc, itemTexture = GetItemInfo(n)

                if itemEquipLoc ~= "" then
					lole_frame:RegisterEvent("LOOT_OPENED")
					SpellStopCasting()
					CastSpellByName("Disenchant")
                    echo("disenchanting " .. n)
					UseContainerItem(b,s)
                    return
                end
        	end
    	end
    end

	echo("de_greeniez: no (more) equippable greeniez found!")

end

local function lole_setmt(name)
		MAIN_TANK = name
		update_main_tank(MAIN_TANK)
		echo("Main tank set to " .. MAIN_TANK .. ". (change with /lole mt <mtname>)")
end

local function lole_setot(name)
		OFF_TANK = name
		update_off_tank(OFF_TANK)
		echo("Off tank set to " .. OFF_TANK .. ". (change with /lole ot <mtname>)")
end

local function lole_clearcc()
	disable_all_cc_targets()
end

local function lole_pull(target_GUID)
	target_unit_with_GUID(target_GUID)
	caster_range_check(28)
	CastSpellByName("Avenger's Shield")
end

function set_target(target_GUID)
	local previous_target_GUID = UnitGUID("target")

	BLAST_TARGET_GUID = target_GUID;
	target_unit_with_GUID(target_GUID); -- this does a C SelectUnit call :P
	FocusUnit("target")
	update_target_text(UnitName("target"), UnitGUID("target"));

	if lole_subcommands.get("playermode") == 1 then
		-- set to previous target :D
		target_unit_with_GUID(previous_target_GUID);
	end

end

function clear_target()
	BLAST_TARGET_GUID = NOTARGET;
	ClearFocus();
	ClearTarget();
	update_target_text("-none-", "");
end

local function lole_target_GUID(GUID)
	if GUID == NOTARGET then
		clear_target()

	elseif BLAST_TARGET_GUID ~= GUID then
		set_target(GUID)
	end

end

local function lole_cc(state, marker, spell)

		echo("set_cc_target: got args: " .. state .. ", " .. marker .. ", " .. spell);

		if (state == "enable") then
			set_CC_job(spell, marker)
			new_CC(UnitName("player"), marker, get_CC_spellID(spell));
		elseif (state == "disable") then
			unset_CC_job(marker)
			delete_CC_entry(marker)
		elseif (state == "clear") then
			unset_all_CC_jobs()
			delete_all_CC_entries()
		else
			lole_error("set_cc_target: invalid argument! (enabled must be \"enable\" || \"disable\" || \"clear\")");
			return false
		end

		return false;

end

local function lole_object_interact(...)
	interact_with_object(...)
end

local function lole_dscript(...)

		local usage = "dscript: usage: /lole dscript {run SCRIPTNAME | stop}";

  	local atab = {};
		for i = 1, select('#', ...) do
        local arg = select(i, ...);
        table.insert(atab, arg);
  	end

		local numargs = table.getn(atab);

		if numargs < 1 then
			lole_error("dscript: usage: /lole dscript {load SCRIPTNAME | run | next | stop}")
			return false;
		end

		local command = atab[1];

	  if command == "load" then
			if numargs < 2 then
				lole_error("dscript load: missing SCRIPTNAME argument!")
				return false;
			else
				local scriptname = atab[2];
				dscript(command, scriptname)
				return true;
			end
		elseif command == "run" then
				dscript(command) -- TODO: if return value is nil, then report erreur
		elseif command == "next" then
				dscript(command)
		elseif command == "stop" then
				dscript(command)
		else
			lole_error("dscript: unknown COMMAND " .. command);
			return false
		end

end

local function lole_cast_gtaoe(spellID, x, y, z)
	cast_GTAOE(spellID, x, y, z);
end

local function lole_sendscript(to, ...)
    local usage = "lole_sendscript: Usage: sendscript/ss [(RAID)/PARTY/GUILD]/[WHISPER/w [t1,t2,...,tn]] scripttext";
    if to == nil then
        echo(usage);
        return false;
    end

    local atab = {};
	for i = 1, select('#', ...) do
        local arg = select(i, ...);
        table.insert(atab, arg);
	end

	local script_text = "";

    if to == "WHISPER" or to == "w" then
    	local recipients = {strsplit(",", atab[1])};
    	table.remove(atab, 1);
    	script_text = table.concat(atab, " ");
    	for _, recipient in pairs(recipients) do
    		SendAddonMessage("lole_runscript", script_text, "WHISPER", recipient);
    	end
    elseif to == "PARTY" or to == "GUILD" then
        script_text = table.concat(atab, " ");
        SendAddonMessage("lole_runscript", script_text, to);
    else
        if to ~= "RAID" then
            table.insert(atab, 1, to);
        end
        script_text = table.concat(atab, " ");
        SendAddonMessage("lole_runscript", script_text, "RAID");
    end
end

local function lole_sendmacro(to, ...)
    local usage = "lole_sendmacro: Usage: sendmacro/run [(RAID)/PARTY/GUILD]/[WHISPER/w [t1,t2,...,tn]] macrotext";
    if to == nil then
        echo(usage);
        return false;
    end

    local atab = {};
    for i = 1, select('#', ...) do
        local arg = select(i, ...);
        table.insert(atab, arg);
    end

    local script_text = "";

    if to == "WHISPER" or to == "w" then
        local recipients = {strsplit(",", atab[1])};
        table.remove(atab, 1);
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        for _, recipient in pairs(recipients) do
            SendAddonMessage("lole_runscript", script_text, "WHISPER", recipient);
        end
    elseif to == "PARTY" or to == "GUILD" then
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        SendAddonMessage("lole_runscript", script_text, to);
    else
        if to ~= "RAID" then
            table.insert(atab, 1, to);
        end
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        SendAddonMessage("lole_runscript", script_text, "RAID");
    end
end

local function lole_sendmacro_to(to, ...)
	lole_sendmacro("WHISPER", to, ...)
end

local function lole_override(name, ...)
    local usage = "lole_override: Usage: override [NAME/RAID] scripttext";
    if name == nil then
        echo(usage);
        return false;
    end
    local atab = {};
    for i = 1, select('#', ...) do
        local arg = select(i, ...);
        table.insert(atab, arg);
    end
    script_text = table.concat(atab, " ");
    if name == "RAID" then
        SendAddonMessage("lole_override", script_text, "RAID");
    else
        SendAddonMessage("lole_override", script_text, "WHISPER", name);
    end
end

local invite_order = {
	"Adieux", "Igop", "Kusip", "Gyorgy",
	"Ribb", "Meisseln", "Crq", "Josp", "Teline",
	"Jobim", "Nilck", "Mulck", "Pogi", "Gawk",
	"Consona", "Dissona", "Kasio", "Bogomips", "Puhveln",
	"Viginti", "Ceucho", "Mam", "Pussu", "Pehmware"
}

local function lole_inv_ordered()
	if UnitName("player") ~= "Noctur" then
		echo("Please run this function as Noctur.")
		return
	end

	for i, name in ipairs(invite_order) do
		if not UnitInRaid(name) then
			InviteUnit(name)
			ConvertToRaid()
			return
		end
	end
end

local function lole_raid_aoe(on_off_str)
    local script_text = "RunMacroText(\"/lole set aoemode " .. on_off_str .. "\")";
    SendAddonMessage("lole_runscript", script_text, "RAID");
end

local function change_healer_targets(op, ...)
    local usage = "change_healer_targets: Usage: healer [set/add/del] healername [heals/hots/ignores] target1,target2,...,targetN [heals/hots/ignores] *targets ... where target = unitname (or 'raid' for heals option)";

    local function get_arg_class(arg)
    	local guildies = get_guild_members()
    	local name = strsplit(",", arg);
    	if guildies[name] or name == "raid" then
    		return "unit";
    	elseif table.contains(ASSIGNMENT_DOMAINS, arg) then
    		return "domain";
    	else
    		return nil
    	end
    end

    local msg = "";
    local previous_class = "domain";
    local class_delimiters = {["unit"] = ">", ["domain"] = "<"};
    if select('#', ...) == 1 then echo(usage); return false; end
    for i = 1, select('#', ...) do
        local arg = select(i, ...);
        local arg_class = get_arg_class(arg);
	    if not arg_class or (i == 1 and arg_class ~= "unit") or (arg_class == "unit" and previous_class ~= "domain") then
        	echo(usage);
	        return false;
	    end
	    if i == 1 then
        	msg = arg .. ":";
        else
	    	msg = msg .. class_delimiters[arg_class] .. arg;
	    end
        previous_class = arg_class;
    end

    msg = op .. ";" .. msg;

    SendAddonMessage("lole_healers", msg, "RAID");
end

local function sync_healer_targets(with)
    local arg_str = "sync";
    if with then
        arg_str = arg_str .. ";" .. with;
    end
    SendAddonMessage("lole_healers", arg_str, "RAID");
end

local function restore_healer_targets(with)
    local arg_str = "restore";
    if with then
        arg_str = arg_str .. ";" .. with;
    end
    SendAddonMessage("lole_healers", arg_str, "RAID");
end

local function reset_healer_targets()
    SendAddonMessage("lole_healers", "reset", "RAID");
end

local function wipe_healer_targets()
    SendAddonMessage("lole_healers", "wipe", "RAID");
end

local function echo_healer_target_info()
    local info = "Current healer assignments (on local client):\n" .. get_healer_target_info();
    echo(info);
end

local function lole_manage_healers(...)
	local funcs = {
		set = change_healer_targets,
		add = change_healer_targets,
		del = change_healer_targets,
		reset = reset_healer_targets,
        wipe = wipe_healer_targets,
		sync = sync_healer_targets,
        restore = restore_healer_targets,
		info = echo_healer_target_info,
	};
	local atab = {};
    for i = 1, select('#', ...) do
        local arg = select(i, ...);
        table.insert(atab, arg);
    end
	local _, func = next(atab);
    table.remove(atab, 1);
    if not funcs[func] then
    	local available_ops = {};
    	for f, _ in pairs(funcs) do
    		table.insert(available_ops, f);
    	end
        if not func then
            echo("Please enter a healer management operation.");
        else
            echo("Not a valid healer management operation: " .. func);
        end
    	echo("Available operations: " .. table.concat(available_ops, ", "));
    	return false;
    end
    if func == "set" or func == "add" or func == "del" then
		funcs[func](...);
	else
		funcs[func](unpack(atab));
	end
end

local function lole_echo(msg)
    SendAddonMessage("lole_echo", msg, "RAID");
end

local function lole_debug_test_blast_target()
		lole_subcommands.setall("blast", 1);
		update_target()
end

local function lole_distance_to_target()
    echo(get_distance_between("player", "target"));
end


----------------------------------------
				----- BROADCASTS ------
----------------------------------------


local function lole_broadcast_target(GUID_str)
	lole_subcommands.sendmacro("RAID", "/lole target", GUID_str);
end

local function lole_broadcast_follow(target_GUID)
	lole_subcommands.sendmacro("RAID", "/lole follow", target_GUID);
end

local function lole_broadcast_stopfollow()
	lole_subcommands.sendmacro("RAID", "/lole stopfollow");
end

local function lole_broadcast_set(attrib, state)
	lole_subcommands.sendmacro("RAID", "/lole set", tostring(attrib), tostring(state));
end

local function lole_broadcast_cooldowns()
	lole_subcommands.override("RAID", "cooldowns");
end

local function lole_broadcast_drink()
	lole_subcommands.sendmacro("RAID", "/lole drink")
end

local function lole_broadcast_mt(name)
	lole_subcommands.sendmacro("RAID", "/lole setmt", name)
end

local function lole_broadcast_ot(arg)
	lole_subcommands.sendmacro("RAID", "/lole setot", name)
end

local function lole_broadcast_drink()
	lole_subcommands.sendmacro("RAID", "/lole drink")
end

local function lole_broadcast_release()
	lole_subcommands.sendmacro("GUILD", "/lole release")
end

local function lole_broadcast_leavegroup()
	lole_subcommands.sendscript("GUILD", "/lole leavegroup")
end


local lole_broadcast_commands = {
	ctm = lole_broadcast_ctm;
	drink = lole_broadcast_drink;
	release = lole_broadcast_release;
	mt = lole_broadcast_mt;
	ot = lole_broadcast_ot;
	cooldowns = lole_broadcast_cooldowns;
	set = lole_broadcast_set;
	follow = lole_broadcast_follow;
	stopfollow = lole_broadcast_stopfollow;
	target = lole_broadcast_target;
	leavegroup = lole_broadcast_leavegroup;
}

local function lole_broadcast(funcname, ...)
	local usage = "lole_broadcast: usage: /lole broadcast funcname [args]";
	if not funcname or funcname == "" then
		echo(usage)
		return 0
	end

	local func = lole_broadcast_commands[funcname];

	if not func then
		lole_error("lole_broadcast: unknown broadcast function \"" .. funcname .. "\"!")
		return 0
	end

	-- call broadcast function =)

	func(...);

end

lole_subcommands = {
    lbuffcheck = lole_leaderbuffcheck,
	buffcheck = lole_buffcheck,
	cooldowns = lole_cooldowns,
	setconfig = lole_setconfig,
	getconfig = lole_getconfig,
	followme = lole_followme,
	stopfollow = lole_stopfollow,
	set = lole_set,
	setall = lole_setall,
	get = lole_get,

	broadcast = lole_broadcast,
	ctm = lole_ctm,

	cooldowns = lole_cooldowns,
	buffs = do_buffs,
	gui = lole_gui,
	drink = lole_drink,
	raid = lole_raid,
	party = lole_party,
	leavegroup = lole_leavegroup,
	release = lole_release,

	setmt = lole_setmt,
	setot = lole_setot,

	clearcc = lole_clearcc,
	pull = lole_pull,
	durability = lole_durability,
	inv_ordered = lole_inv_ordered,
    raid_aoe = lole_raid_aoe,
  	healer = lole_manage_healers,
	de_greeniez = lole_disenchant_greeniez,

	follow = lole_follow,
	stopfollow = lole_stopfollow,
	target = lole_target_GUID,
	sendscript = lole_sendscript,
	ss = lole_sendscript,
	sendmacro = lole_sendmacro,
	run = lole_sendmacro,
    override = lole_override,
    echo = lole_echo,

	sendmacro_to = lole_sendmacro_to,

	cast_gtaoe = lole_cast_gtaoe,
	object_interact = lole_object_interact,

	cc = lole_cc,

	dump = lole_debug_dump_wowobjects,
	loot = lole_debug_loot_all,
	de_greeniez = lole_disenchant_greeniez,
	test_blast_target = lole_debug_test_blast_target,
	dscript = lole_dscript,

	register = lole_debug_lua_register,
    distance = lole_distance_to_target,

	encrypt = lole_encrypt_test;
}
