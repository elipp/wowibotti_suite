local mode_attribs = {
	blast = 0,
	heal_blast = 0,
	playermode = 0,
	buffmode = 0,
	combatbuffmode = 0,
	aoemode = 0,
	hold = 0,
	dispelmode = 0,
	strict_targeting = 0,
}

LOLE_MODE_ATTRIBS = mode_attribs

function get_available_mode_attribs()

	local r = "|cFFFFFF00"

	for k,v in pairs(mode_attribs) do
		 r = r .. " " .. k
	end

	return r

end

local function lole_setconfig(arg, modes)
	if (arg == nil or arg == "") then
		echo("lole_setconfig: erroneous argument!");
		return false;
	end

	conf = get_config(arg);

	if not conf then
		echo("lole_setconfig: invalid config option \"" .. arg .. "\"! See lole.lua.");
		return false;
	else

		set_config(arg)
		LOLE_CLASS_CONFIG_NAME_SAVED = arg;

		set_visible_dropdown_config(arg) -- gui stuff

		-- This patch is there so that adding mode attribs would be easier
		if modes then
			update_table(mode_attribs, modes)
		end

		local attribs = shallowcopy(mode_attribs)
		attribs.blast = 0
		attribs.heal_blast = 0

		LOLE_CLASS_CONFIG_ATTRIBS_SAVED = attribs

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
	if (not name) or (name == UnitName("player")) or playermode() then
		return
	else
			follow_unit(name)
	end
end

local function lole_stopfollow()
	if not playermode() then
		stopfollow()
	end
end

function lole_set(attrib_name, on_off_str)

	if (attrib_name == nil or attrib_name == "") then
		echo("lole_set: no argument! valid modes for config " .. get_current_config().name .. " are: |cFFFFFF00" .. get_available_mode_attribs());
		return false;
	end

	if mode_attribs[attrib_name] ~= nil then
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

function lole_get(attrib_name)

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

CtmPrio = {
	None = 0,
	Low = 1,
	Replace = 2,
	Exclusive = 3,
	Follow = 4,
	NoOverride = 5,
	HoldPosition = 6,
	ClearHold = 7,
}

local function lole_broadcast_ctm(x, y, z)

	local units = get_selected_units()

	for i,n in pairs(units) do
		lole_subcommands.sendmacro_to(n, "/lole ctm", x, y, z, CtmPrio.ClearHold);
	end
end

local function lole_ctm(x, y, z, prio)
	if playermode() then return end

	if lole_subcommands.get("hold") == 0 then
		walk_to(tonumber(x), tonumber(y), tonumber(z), prio)
	elseif tonumber(prio) >= CtmPrio.ClearHold then
		lole_subcommands.set("hold", 0)
		walk_to(tonumber(x), tonumber(y), tonumber(z), prio)
	end

end

local function lole_broadcast_hold()
	local units = get_selected_units()

	for i,n in pairs(units) do
		lole_subcommands.sendmacro_to(n, "/lole hold");
	end
end

local function lole_hold()
	lole_subcommands.set("hold", 1)
end

local function lole_show()
	main_frame_show()
end

local function lole_hide()
	main_frame_hide()
end

local function lole_sfshow()
	selection_frame_show()
end

local function lole_sfhide()
	selection_frame_hide()
end


local function lole_cooldowns()
	L_UseInventoryItem(13);
	L_UseInventoryItem(14);

	local _, race = UnitRace("player");

	if race == "Orc" then
		L_CastSpellByName("Blood Fury")
	elseif race == "Troll" then
		L_CastSpellByName("Berserking")
	end

	for _, spell in pairs(get_current_config().cooldowns) do
		L_SpellStopCasting()
		L_CastSpellByName(spell);
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
    	            buffs["Greater Blessing of Sanctuary"] = missing_buffs["Blessing of Sanctuary"];
    	            buffs["Greater Blessing of Wisdom"] = missing_buffs["Blessing of Wisdom"];
    	            buffs["Greater Blessing of Might"] = missing_buffs["Blessing of Might"];
                elseif #paladins == 2 then
                    if config_name == "paladin_holy" or (config_name == "paladin_retri" and table.contains(paladins, "Adieux")) then
                        buffs["Greater Blessing of Sanctuary"] = missing_buffs["Blessing of Sanctuary"];
                        buffs["Greater Blessing of Wisdom"] = missing_buffs["Blessing of Wisdom"];
                        buffs["Greater Blessing of Might"] = missing_buffs["Blessing of Might"];
        	        else
        	        	buffs["Greater Blessing of Kings"] = missing_buffs["Blessing of Kings"];
        	        end
                elseif #paladins == 3 then
                    if config_name == "paladin_prot" then
                        buffs["Greater Blessing of Sanctuary"] = missing_buffs["Blessing of Sanctuary"];
                    elseif config_name == "paladin_retri" then
                        buffs["Greater Blessing of Kings"] = missing_buffs["Blessing of Kings"];
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
						echo('casting to ', char, buff)
            L_TargetUnit(char);
            L_CastSpellByName(buff);
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
		if mana_percentage("player") < 70 then
			stopfollow()
			UseItemByName("Conjured Water")
			local water_left = GetItemCount("Conjured Water")
			if (water_left < 20 and water_left % 5 == 0) or water_left < 5 then
				SendChatMessage("Got " .. water_left .. " mana drinks left.", "GUILD")
			end
		end

		--if 1 == 0 and UnitMana("player")/UnitManaMax("player") < 0.90 or UnitHealth("player")/UnitHealthMax("player") < 0.75 then
  --          if GetItemCount(43523) > 0 then
  --              L_UseItemByName("Conjured Mana Strudel");
		--	elseif GetItemCount(33445) > 0 then
		--		L_UseItemByName("Honeymint Tea");
		--	else
		--		SendChatMessage("I'm out of mana drinks! Giev plx.", "GUILD")
		--	end
		--end
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
		lole_frame_register("PARTY_MEMBERS_CHANGED");
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
									LOOT_OPENED_REASON = "DE_GREENIEZ"
									lole_frame_register("LOOT_OPENED")
									L_SpellStopCasting()
									L_CastSpellByName("Disenchant")
                  echo("disenchanting " .. n)
									UseContainerItem(b,s)
                  return
                end
        	end
    	end
    end

	echo("de_greeniez: no (more) equippable greeniez found!")

end

local function lole_clearcc()
	disable_all_cc_targets()
end

local function lole_pull(target_GUID)
	target_unit_with_GUID(target_GUID)
	caster_range_check(0,28)
	L_CastSpellByName("Avenger's Shield")
end

function set_target(target_GUID)
	local current_target_GUID = UnitGUID("target")

	BLAST_TARGET_GUID = target_GUID;
	target_unit_with_GUID(target_GUID); -- this does a C SelectUnit call :P
	L_FocusUnit("target")
	update_target_text(UnitName("target"), UnitGUID("target"));

	if playermode() and current_target_GUID then
		target_unit_with_GUID(current_target_GUID)
	end

end

function clear_target()
	BLAST_TARGET_GUID = NOTARGET;
	L_ClearFocus()
	if not playermode() then
		L_ClearTarget();
	end
	update_target_text("-none-", "");
end

local function lole_target_GUID(GUID)

	if not GUID then
		lole_error("lole_target_GUID: please specify a target GUID!")
		return
	end

	if GUID:len() ~= 18 then
		lole_error("lole_target_GUID: GUID str must be of length 18 (0x included)")
		return
	end

	if GUID == NOTARGET then
		clear_target()

	elseif BLAST_TARGET_GUID ~= GUID then
		set_target(GUID)
	end

end

local function lole_target()
    update_target();
end

local function lole_cc(state, marker, spellID)

		if (state == "enable") then
			set_CC_job(tonumber(spellID), marker)
			new_CC(UnitName("player"), marker, tonumber(spellID));
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

local function lole_interact_object(...)
	interact_with_object(...)
end

local function lole_interact_spellnpc(...)
	interact_with_spellnpc(...)
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

local function lole_cast_gtaoe(spellname, x, y, z)
	cast_gtaoe(spellname, x, y, z);
end

local function lole_setselection(targets)
	local target_table = tokenize_string(targets, ",")
	update_selection(target_table)
end

local function lole_clearselection()
	clear_selection()
end

local function lole_resetcamera()
	reset_camera()
end

local function lole_wc3mode(enabled)
	enable_wc3mode(enabled)
end

local function lole_capturerender()
	print("Capturing all render stages of a frame, this could take a while...")
	capture_render_stages()
end

local function lole_getbiscuit()
	L_CastSpellByName("Arcane Torrent")
	--get_biscuits()
end

local function lole_script(scripttext)
	L_RunScript(scripttext)
end

local function lole_sendscript(to, ...)
    local usage = "lole_sendscript: Usage: sendscript/ss [(RAID)/PARTY/GUILD]/[WHISPER/w [t1,t2,...,tn]] scripttext";
    if to == nil then
        echo(usage);
        return false;
    end

    local atab = get_arg_table(...)
		local script_text = "";

    if to == "WHISPER" or to == "w" then
    	local recipients = {strsplit(",", atab[1])};
    	table.remove(atab, 1);
    	script_text = table.concat(atab, " ");
    	for _, recipient in pairs(recipients) do
    		L_SendAddonMessage("lole_runscript", script_text, "WHISPER", recipient);
    	end
    elseif to == "PARTY" or to == "GUILD" then
        script_text = table.concat(atab, " ");
        L_SendAddonMessage("lole_runscript", script_text, to);
    else
        if to ~= "RAID" then
            table.insert(atab, 1, to);
        end
        script_text = table.concat(atab, " ");
        L_SendAddonMessage("lole_runscript", script_text, "RAID");
    end
end

local function lole_sendmacro(to, ...)
    local usage = "lole_sendmacro: Usage: sendmacro/run [(RAID)/PARTY/GUILD]/[WHISPER/w [t1,t2,...,tn]] macrotext";
    if to == nil then
        echo(usage);
        return false;
    end

    local atab = get_arg_table(...)
    local script_text = "";

    if to == "WHISPER" or to == "w" then
        local recipients = {strsplit(",", atab[1])};
        table.remove(atab, 1);
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        for _, recipient in pairs(recipients) do
            L_SendAddonMessage("lole_runscript", script_text, "WHISPER", recipient);
        end
    elseif to == "PARTY" or to == "GUILD" then
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        L_SendAddonMessage("lole_runscript", script_text, to);
    else
        if to ~= "RAID" then
            table.insert(atab, 1, to);
        end
        script_text = "RunMacroText(\"" .. table.concat(atab, " ") .. "\")";
        L_SendAddonMessage("lole_runscript", script_text, "RAID");
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
    local atab = get_arg_table(...)

    script_text = table.concat(atab, " ");
    if name == "RAID" then
        L_SendAddonMessage("lole_override", script_text, "RAID");
    else
        L_SendAddonMessage("lole_override", script_text, "WHISPER", name);
    end
end

local function lole_loot_badge(corpse_GUID)

	if not corpse_GUID then
		lole_error("lole_loot_badge: no target GUID given!")
		return false
	end

	target_unit_with_GUID(corpse_GUID)
	if not UnitExists("target") or not UnitIsDead("target") then
		lole_error("loot_badge: invalid loot target with GUID " .. corpse_GUID .. " (does not exist or is still alive)!")
	else
		LOOT_OPENED_REASON = "LOOT_BADGE"
		lole_frame_register("LOOT_OPENED")
		loot_badge(corpse_GUID)
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

local function lole_invite()
	for name, i in pairs(get_guild_members()) do
		echo(name)
		InviteUnit(name)
		if GetNumPartyMembers() >= 5 then
			ConvertToRaid()
		end
	end
end

local function lole_raid_aoe(on_off_str)
    local script_text = "RunMacroText(\"/lole set aoemode " .. on_off_str .. "\")";
    L_SendAddonMessage("lole_runscript", script_text, "RAID");
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

    L_SendAddonMessage("lole_healers", msg, "RAID");
end

local function sync_healer_targets(with)
    local arg_str = "sync";
    if with then
        arg_str = arg_str .. ";" .. with;
    end
    L_SendAddonMessage("lole_healers", arg_str, "RAID");
end

local function restore_healer_targets(with)
    local arg_str = "restore";
    if with then
        arg_str = arg_str .. ";" .. with;
    end
    L_SendAddonMessage("lole_healers", arg_str, "RAID");
end

local function reset_healer_targets()
    L_SendAddonMessage("lole_healers", "reset", "RAID");
end

local function wipe_healer_targets()
    L_SendAddonMessage("lole_healers", "wipe", "RAID");
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

	local atab = get_arg_table(...)

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
    L_SendAddonMessage("lole_echo", msg, "RAID");
end


local function lole_invite_guild()
    GuildRoster()
	for i = 1, GetNumGuildMembers() do
		local name = GetGuildRosterInfo(i)
		if UnitName("player") ~= name then
			InviteUnit(name)
		end
	end
end

local function lole_distance_to_target()
    echo(get_distance_between("player", "target"));
end

local function lole_noclip()
	noclip()
end

local function lole_execute(...)
	local arg_concatd = concatenate_args(" ", ...)
	if arg_concatd then
		execute_script(arg_concatd)
	end
end


----------------------------------------
				----- BROADCASTS ------
----------------------------------------


local function lole_broadcast_target(GUID_str)
	lole_subcommands.sendmacro("RAID", "/lole target", GUID_str);
end

local function lole_broadcast_attack(GUID_str)
		local units = get_selected_units()
		for i,n in pairs(units) do
			lole_subcommands.sendmacro_to(n, "/lole target", GUID_str); -- last arg == priority level
			lole_subcommands.sendmacro_to(n, "/lole set blast 1")
			lole_subcommands.sendmacro_to(n, "/lole set hold 0")
		end

end

local function lole_broadcast_follow(name)
	lole_subcommands.sendmacro("RAID", "/lole follow", name);
end

local function lole_broadcast_stopfollow()
	lole_subcommands.sendmacro("RAID", "/lole stopfollow");
end

local function lole_broadcast_set(attrib, state)
    if not in_party_or_raid() then
        echo("Not in a party or raid, setting state locally")
        lole_set(attrib, state)
    else
	   lole_subcommands.sendmacro("RAID", "/lole set", tostring(attrib), tostring(state));
    end
end

local function lole_broadcast_cooldowns()
	lole_subcommands.override("RAID", "cooldowns");
end

local function lole_broadcast_drink()
	lole_subcommands.sendmacro("RAID", "/lole drink")
end

local function lole_broadcast_drink()
	lole_subcommands.sendmacro("RAID", "/lole drink")
end

local function lole_broadcast_release()
	lole_subcommands.sendmacro("GUILD", "/lole release")
end

local function lole_broadcast_leavegroup()
	lole_subcommands.sendmacro("GUILD", "/lole leavegroup")
end

local function lole_broadcast_getbiscuits()
	lole_subcommands.sendmacro("GUILD", "/lole getbiscuit")
end

local function lole_broadcast_reloadui()
	lole_subcommands.sendscript("RAID", "ConsoleExec(\"reloadui\")")
end

local function lole_broadcast_eject()
	lole_subcommands.sendmacro("GUILD", "/lole eject")
end

local lole_broadcast_commands = {
	ctm = lole_broadcast_ctm;
	drink = lole_broadcast_drink;
	release = lole_broadcast_release;
	cooldowns = lole_broadcast_cooldowns;
	set = lole_broadcast_set;
	follow = lole_broadcast_follow;
	stopfollow = lole_broadcast_stopfollow;
	target = lole_broadcast_target;
	attack = lole_broadcast_attack;
	leavegroup = lole_broadcast_leavegroup;
	getbiscuits = lole_broadcast_getbiscuits;
	hold = lole_broadcast_hold;
	reloadui = lole_broadcast_reloadui;
	eject = lole_broadcast_eject;
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

local function lole_console_print(msg)

	console_print(msg)

	-- for k,v in pairs(_G) do
	-- 	--if string.find(k, "LFD") then
	-- 		console_print(k)
	-- 	--end
	-- end

end

-- just a "macro"
local function send_spread_ctm(to, v3)
	lole_sendmacro_to(to, "/lole ctm", tostring(v3.x), tostring(v3.y), tostring(v3.z))
end

local function SPREAD_ONYXIA()

	local opos = vec3:create(-10.6, -219.4, -87.7) -- THIS IS FOR ONYXIA
	local RADIUS = 14
	local v = vec3:create(RADIUS, 0, 0)

-- local V = opos:add(v:rotated2d(-2*(3.14)/8))
-- lole_sendmacro_to("Briit", "/lole ctm", tostring(V.x), tostring(V.y), tostring(V.z))
--
-- V = opos:add(v:rotated2d(-1*(3.14)/8))
-- lole_sendmacro_to("Eino", "/lole ctm", tostring(V.x), tostring(V.y), tostring(V.z))

	V = opos:add(v:rotated2d(-1*(3.14)/8))
	send_spread_ctm("Printf", V)

	V = opos:add(v:rotated2d(1*(3.14)/8))
	send_spread_ctm("Ghospodi", V)

	-- V = opos:add(v:rotated2d(2*(3.14)/8))
	-- lole_sendmacro_to("Robins", "/lole ctm", tostring(V.x), tostring(V.y), tostring(V.z))
end

local function SPREAD_MADOT()
	--local opos = vec3:create(563, 152, 394)
	local opos = vec3:create(get_unit_position("focus"))
	local RADIUS = 30

	local v = TOC_middle:subtract(opos):unit_scaled(RADIUS) -- 563, 140, 393 is middle of TOC arena

	local V = opos:add(v:rotated2d(-2*(3.14)/8))
	send_spread_ctm("Briit", V)

	V = opos:add(v:rotated2d(-1*(3.14)/8))
	send_spread_ctm("Eino", V)

	V = opos:add(v:rotated2d(0*(3.14)/8))
	send_spread_ctm("Printf", V)

	V = opos:add(v:rotated2d(1*(3.14)/8))
	send_spread_ctm("Ghospodi", V)

	V = opos:add(v:rotated2d(2*(3.14)/8))
	send_spread_ctm("Robins", V)
end

local function SPREAD_SAVIANA(dist)
	local opos = vec3:create(get_unit_position("player")) -- these are the real coords
--	local opos = vec3:create(3220, 607, 79)
	local incr_y = vec3:create(0, dist, 0)
	local incr_x = vec3:create(dist, 0, 0)

	local V = opos:add(incr_y)
	send_spread_ctm("Briit", V)

	V = opos:add(incr_y:scale2d(2))
	send_spread_ctm("Eino", V)

	opos = opos:add(incr_x)
	V = opos
	send_spread_ctm("Printf", V)

	V = opos:add(incr_y)
	send_spread_ctm("Ghospodi", V)

	V = opos:add(incr_y:scale2d(2))
	send_spread_ctm("Robins", V)

	opos = opos:add(incr_x)
	V = opos
	send_spread_ctm("Teilor", V)

	V = opos:add(incr_y)
	send_spread_ctm("Iijj", V)

	V = opos:add(incr_y:scale2d(2))
	send_spread_ctm("Spobodi", V)


end

local function SPREAD_TORAVON()

	local opos = vec3:create(get_unit_position("focus"))
	local RADIUS = 27
	local v = vec3:create(0, -1, 0):unit_scaled(RADIUS)

	local FACTOR = 6.28/6.0

	local V = opos:add(v:rotated2d(-2*FACTOR))
	send_spread_ctm("Briit", V)

	V = opos:add(v:rotated2d(-1*FACTOR))
	send_spread_ctm("Eino", V)

	V = opos:add(v:rotated2d(0*FACTOR))
	send_spread_ctm("Printf", V)

	V = opos:add(v:rotated2d(1*FACTOR))
	send_spread_ctm("Ghospodi", V)

	V = opos:add(v:rotated2d(2*FACTOR))
	send_spread_ctm("Robins", V)

	V = opos:add(v:rotated2d(3*FACTOR))
	send_spread_ctm("Teilor", V)
end

local function SPREAD_FESTERGUT()
	local mid = vec3:create(4267.9, 3137.3, 360.4)
	local RADIUS = 28
	local v = vec3:create(0, -RADIUS, 0)

	local FACTOR = 6.28/8.0

	local V = mid:add(v:rotated2d(-2*FACTOR))
	send_spread_ctm("Briit", V)

	V = mid:add(v:rotated2d(-1*FACTOR))
	send_spread_ctm("Eino", V)

	V = mid:add(v:rotated2d(0*FACTOR))
	send_spread_ctm("Printf", V)

	V = mid:add(v:rotated2d(1*FACTOR))
	send_spread_ctm("Ghospodi", V)

	V = mid:add(v:rotated2d(2*FACTOR))
	send_spread_ctm("Robins", V)

	V = mid:add(v:rotated2d(3*FACTOR))
	send_spread_ctm("Teilor", V)

end


local function lole_spread_those_cheeks(dist)
	-- REMEMBER TO CHANGE THIS ACCORDINGLY :DDDDDDDDD
--	SPREAD_ONYXIA()

	if not dist then dist = 15 end
--	SPREAD_SAVIANA(dist)
	--SPREAD_MADOT()
	SPREAD_FESTERGUT()
end

local function lole_iccrocket(mirror_data)
	if not playermode() then
		iccrocket(mirror_data)
	end
end

local function lole_click_essence_portal(type)
	-- TYPE == either "Dark" or "Light"
	run_to_essenceportal_and_click(type .. " Essence")

end

local function lole_boss_action(...)
	local catd = concatenate_args(" ", ...)
	if not catd then return end
	boss_action(catd)
end

local function lole_hconfig(...)
	local catd = concatenate_args(" ", ...)
	if not catd then return end
	hconfig(catd)
end

local function lole_loot_mob()
	loot_nearest_lootable_mob()
end

local function lole_eject()
	eject_DLL()
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

	execute = lole_execute,

	broadcast = lole_broadcast,
	bc = lole_broadcast,

	ctm = lole_ctm,

	cooldowns = lole_cooldowns,
	buffs = do_buffs,
	show = lole_show,
	hide = lole_hide,

	sfshow = lole_sfshow;
	sfhide = lole_sfhide;

	drink = lole_drink,
	raid = lole_raid,
	party = lole_party,
	leavegroup = lole_leavegroup,
	release = lole_release,

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
	tar = lole_target,
	sendscript = lole_sendscript,
	ss = lole_sendscript,
	sendmacro = lole_sendmacro,
	run = lole_sendmacro,
	script = lole_script,
	override = lole_override,
	echo = lole_echo,

	sendmacro_to = lole_sendmacro_to,

	cast_gtaoe = lole_cast_gtaoe,
	interact_object = lole_interact_object,
	interact_spellnpc = lole_interact_spellnpc,

	cc = lole_cc,

	dump = lole_debug_dump_wowobjects,
	loot = lole_debug_loot_all,
	de_greeniez = lole_disenchant_greeniez,
	dscript = lole_dscript,

	face = lole_debug_face_angle,

	register = lole_debug_lua_register,
	distance = lole_distance_to_target,

  debug_test = lole_debug_test,
	parse_encounter = lole_parse_encounter,
	noclip = lole_noclip,
	getbiscuit = lole_getbiscuit,
	loot_badge = lole_loot_badge,
	invite_guild = lole_invite_guild,
	invite = lole_invite,
	boss_action = lole_boss_action,
	ba = lole_boss_action,

	hconfig = lole_hconfig,

	spread = lole_spread_those_cheeks,
	click_essence_portal = lole_click_essence_portal,

	setselection = lole_setselection,
	clearselection = lole_clearselection,

	resetcamera = lole_resetcamera,
	capturerender = lole_capturerender,

	iccrocket = lole_iccrocket,

	wc3mode = lole_wc3mode,
	hold = lole_hold,

	eject = lole_eject,

	console_print = lole_console_print,
	loot = lole_loot_mob,

	dispel = group_dispel,
}
