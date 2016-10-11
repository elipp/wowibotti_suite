-- globals

NOTARGET = "0x0000000000000000";

BLAST_TARGET_GUID = "0x0000000000000000";
MISSING_BUFFS = {};

MAIN_TANK = nil
OFF_TANK = nil
HEALERS = {"Ceucho", "Kusip", "Kasio", "Mam", "Igop", "Puhveln"};
HEALER_TARGETS = { -- defaults
    Ceucho = {"Adieux", "Noctur", "raid"};
    Kusip = {"Adieux", "Noctur", "raid"};
    Kasio = {"raid"};
    Mam = {"raid"};
    Igop = {"raid"};
    Puhveln = {"raid"};
}
HEALS_IN_PROGRESS = {};

HEAL_ESTIMATES = {
    ["Flash of Light(Rank 7)"] = 1400,
    ["Holy Light(Rank 11)"] = 4300,
    ["Holy Light(Rank 5)"] = 1300,
    ["Lesser Healing Wave(Rank 7)"] = 2200,
    ["Healing Wave(Rank 12)"] = 4700,
    ["Chain Heal(Rank 5)"] = 3200,
    ["Regrowth(Rank 10)"] = 3200, -- a few ticks included
    ["Healing Touch(Rank 13)"] = 5300,
    ["Flash Heal(Rank 9)"] = 2300,
    ["Greater Heal(Rank 7)"] = 5000,
    ["Greater Heal(Rank 1)"] = 3000,
    ["Prayer of Healing(Rank 6)"] = 2300,
    ["Binding Heal(Rank 1)"] = 2000,
}

ROLES = { healer = 1, caster = 2, warrior_tank = 3, paladin_tank = 4, melee = 5, mana_melee = 6 }

-- http://wowwiki.wikia.com/wiki/Class_colors

local CLASS_COLORS = {
	druid = "FF7D0A",
	DRUID = "FF7D0A",
	Druid = "FF7D0A",

	hunter = "ABD473",
	HUNTER = "ABD473",
	Hunter = "ABD473",

	mage = "69CCF0",
	MAGE = "69CCF0",
	Mage = "69CCF0",

	paladin = "F58CBA",
	PALADIN = "F58CBA",
	Paladin = "F58CBA",

	priest = "FFFFFF",
	PRIEST = "FFFFFF",
	Priest = "FFFFFF",

	rogue = "FFF569",
	ROGUE = "FFF569",
	Rogue = "FFF569",

	shaman = "0070DE",
	SHAMAN = "0070DE",
	Shaman = "0070DE",

	warlock = "9482C9",
	WARLOCK = "9482C9",
	Warlock = "9482C9",

	warrior = "C79C6E",
	WARRIOR = "C79C6E",
	Warrior = "C79C6E",
}

function get_class_color(class)
	local r = CLASS_COLORS[class]
	if not r then
		return "(ERR)"
	else
		return r
	end
end

local raid_target_indices = {

["star"] = 1,
["Star"] = 1,

["circle"] = 2,
["Circle"] = 2,

["diamond"] = 3,
["Diamond"] = 3,

["triangle"] = 4,
["Triangle"] = 4,

["crescent"] = 5,
["Crescent"] = 5,

["moon"] = 5,
["Moon"] = 5,

["square"] = 6,
["Square"] = 6,

["cross"] = 7,
["Cross"] = 7,

["skull"] = 8,
["Skull"] = 8

}


local CC_spells = {
	Polymorph = 118,
	Sheep = 118,
	sheep = 118,

	Cyclone = 33786,
	cyclone = 33786,

	["Entangling Roots"] = 26989,
	Roots = 26989,
	roots = 26989,
	root = 26989,

	Banish = 18647,
	banish = 18647,
	ban = 18647,

	Fear = 6215,
	fear = 6215,

	["Shackle Undead"] = 10955,
	Shackle = 10955,
	shackle = 10955,

	["Turn Evil"] = 10326,
	Turn = 10326,
	turn = 10326,
}


local CC_spellnames = { -- in a CastSpellByName-able format
	[118] = "Polymorph",
	[33786] = "Cyclone",
	[26989] = "Entangling Roots",
	[18647] = "Banish",
	[6215] = "Fear",
	[10955] = "Shackle Undead",
	[10326] = "Turn Evil",
}

function get_CC_spellID(name)
	return CC_spells[name];
end

function get_CC_spellname(spellID)
	return CC_spellnames[spellID];
end

function get_marker_index(name)
	return raid_target_indices[name]
end

function injected()
	local s = GetCVar("screenshotQuality");
	if s ~= "iok" then
		return nil;
	else
		return 1
	end
end

function echo(text)
    DEFAULT_CHAT_FRAME:AddMessage("lole: " .. tostring(text))
end

function lole_error(text)
	DEFAULT_CHAT_FRAME:AddMessage("|cFFFF3300lole: error: " .. tostring(text))
end

function shallowcopy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in pairs(orig) do
            copy[orig_key] = orig_value
        end
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function first_to_upper(str)
    return (str:gsub("^%l", string.upper))
end

function table.val_to_str ( v )
  if "string" == type( v ) then
    v = string.gsub( v, "\n", "\\n" )
    if string.match( string.gsub(v,"[^'\"]",""), '^"+$' ) then
      return "'" .. v .. "'"
    end
    return '"' .. string.gsub(v,'"', '\\"' ) .. '"'
  else
    return "table" == type( v ) and table.tostring( v ) or
      tostring( v )
  end
end

function table.key_to_str ( k )
  if "string" == type( k ) and string.match( k, "^[_%a][_%a%d]*$" ) then
    return k
  else
    return "[" .. table.val_to_str( k ) .. "]"
  end
end

function table.tostring( tbl )
  local result, done = {}, {}
  for k, v in ipairs( tbl ) do
    table.insert( result, table.val_to_str( v ) )
    done[ k ] = true
  end
  for k, v in pairs( tbl ) do
    if not done[ k ] then
      table.insert( result,
        table.key_to_str( k ) .. "=" .. table.val_to_str( v ) )
    end
  end
  return "{" .. table.concat( result, "," ) .. "}"
end

function table.contains(table, element)
  for _, value in pairs(table) do
    if value == element then
      return true
    end
  end
  return false
end

function get_available_class_configs()
	return get_list_of_keys(available_configs)
end

function get_available_class_configs_pretty()
	local key_tab, n = {}, 1;

	for name, _ in pairs_by_key(get_available_configs()) do
		key_tab[n] = get_config_name_with_color(name);
		n = n + 1;
	end

	return table.concat(key_tab, ", ");

end


function get_mode_attribs()
	return get_list_of_keys(lole_subcommands.get())
end


function get_available_subcommands()
	return get_list_of_keys(lole_subcommands);
end

function get_config_name_with_color(arg_config)
	if arg_config == "default" then
		return "|r|rdefault";
	else
		local avconf = get_available_configs()
		return "|cFF" .. avconf[arg_config].color .. avconf[arg_config].name .. "|r";
	end

end

function cast_if_nocd(spellname)
	if GetSpellCooldown(spellname) == 0 then
		CastSpellByName(spellname);
		return true;
	end

	return false;
end

function cast_spell(spellname)
	local name, rank, icon, cost, isFunnel, powerType, castTime, minRange, maxRange = GetSpellInfo(spellname);
	cast_state = { true, GetTime(), castTime, UnitName("target") };
	cast_if_nocd(spellname);
end

function get_HP_deficits(party_only)
	local HP_deficits = {};
    local num_raid_members = 0;

    if party_only then
        num_raid_members = 0;
    else
        num_raid_members = GetNumRaidMembers();
    end

	if num_raid_members == 0 then
	    HP_deficits["player"] = UnitHealthMax("player") - UnitHealth("player");
	    for i=1,4,1 do local exists = GetPartyMember(i)
            local name = "party" .. i;
            if exists and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
            	HP_deficits[name] = UnitHealthMax(name) - UnitHealth(name);
            end
	    end
	else
		--HP_deficits["player"] = UnitHealthMax("player") - UnitHealth("player");
		for i=1,num_raid_members,1 do
			local name = "raid" .. tonumber(i);
			if UnitExists(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
				HP_deficits[name] = UnitHealthMax(name) - UnitHealth(name);
			end
		end
	end

	return HP_deficits;

end

function get_lowest_hp(hp_deficits)

	local lowest = nil;
	local lowest_deficit = 0;

	for name,hp_deficit in pairs(hp_deficits) do

		if not lowest then
			lowest = name
			lowest_deficit = hp_deficit
		else
			if hp_deficit > hp_deficits[lowest] then
				lowest = name
				lowest_deficit = hp_deficit
			end
		end
	end

	return lowest, lowest_deficit;

end

function get_HPs(party_only)
    local HPs = {};
    local num_raid_members = 0;

    if party_only then
        num_raid_members = 0;
    else
        num_raid_members = GetNumRaidMembers();
    end

    if num_raid_members == 0 then
        HPs["player"] = UnitHealth("player");
        for i=1,4,1 do local exists = GetPartyMember(i)
            local name = "party" .. i;
            if exists and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
                HPs[name] = UnitHealth(name);
            end
        end
    else
        for i=1,num_raid_members,1 do
            local name = "raid" .. tonumber(i);
            if UnitExists(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
                HPs[name] = UnitHealth(name);
            end
        end
    end

    return HPs;

end

function get_lowest_hp_char(chars)

    local lowest = nil;
    local lowest_hp = 0;

    for i, name in pairs(chars) do
        if name == "raid" or not UnitExists(name) or UnitIsDead(name) or has_buff(name, "Spirit of Redemption") then
        elseif not lowest then
            lowest_hp = UnitHealth(name);
            lowest = name;
        else
            local tmp_hp = UnitHealth(name);
            if tmp_hp < lowest_hp then
                lowest_hp = tmp_hp;
                lowest = name;
            end
        end
    end

    return lowest;

end

local CS_CASTING, CS_TIMESTAMP, CS_CASTTIME, CS_TARGET = 1, 2, 3, 4;
local NOT_CASTING = { false, 0.0, 0.0, "none" };

local cast_state = NOT_CASTING;

function casting_legit_heal()

	if UnitCastingInfo("player") then return true; end

	if cast_state[CS_CASTING] then
		if (GetTime() - cast_state[CS_TIMESTAMP])*1000 > (cast_state[CS_CASTTIME]+100) then
			cast_state = NOT_CASTING;
			return false;

		elseif not UnitCastingInfo("player") then
			cast_state = NOT_CASTING;
			return false;

		elseif (UnitHealthMax(cast_state[CS_TARGET]) - UnitHealth(cast_state[CS_TARGET])) < 1000 then
			stopfollow();
			cast_state = NOT_CASTING; -- useful when the UnitHealth info lag causes the char to overheal (or any cause)
			return false;

		else return true; end
	end

	return false;

end

function cleanse_party(debuffname)
	for i=1,5,1 do
        local exists = true;
        local name = "party" .. i;
        if i == 5 then
            name = "player"
        else
            exists = GetPartyMember(i)
        end
        if exists and has_debuff(name, debuffname) then
			TargetUnit(name);
            CastSpellByName("Cleanse");
			CastSpellByName("Dispel Magic")
            return true;
		end
	end
	return false;
end

function decurse_party(debuffname)
    for i=1,5,1 do
        local exists = true;
        local name = "party" .. i;
        if i == 5 then
            name = "player"
        else
            exists = GetPartyMember(i)
        end
        if exists and has_debuff(name, debuffname) then
            TargetUnit(name);
            CastSpellByName("Remove Curse");
            CastSpellByName("Remove Lesser Curse")
            return true;
        end
    end
    return false;
end

function has_debuff(targetname, debuff_name)
	local fnd = false;
	local timeleft = 999;
	for i=1,16,1 do name, _, _, _, _, _, timeleft = UnitDebuff(targetname,i)
		if (name ~= nil and string.find(name, debuff_name)) then
			fnd=true;
			break;
		end
	end
	return fnd, timeleft;
end

function has_debuff_by_self(targetname, debuff_name)

	for i=1,16,1 do name, _, _, _, _, duration = UnitDebuff(targetname,i)
		if (name ~= nil and string.find(name, debuff_name)) then
			if duration then	-- in this version of the wow lua api, duration and timeleft == nil for debuffs cast by others
				return true;
			end
		end
	end

	return false;


end

function get_num_debuff_stacks(targetname, debuff_name)

	for i=1,16,1 do name, _, _, count, _, _, timeleft = UnitDebuff(targetname, i)
		if (name ~= nil and string.find(name, debuff_name)) then
			return count, timeleft;
		end
	end
	-- not debuffed with debuff_name
	return 0, 999;

end


function has_buff(targetname, buff_name)
	local fnd = false;
	local timeleft = 999;
	local stacks = 0;
	for i=1,16,1 do name, _, icon, stacks, _, timeleft = UnitBuff(targetname, i)
		if(name ~= nil and string.find(name, buff_name)) then
			fnd=true;
			break;
		end
	end
	return fnd, timeleft, stacks;

end

function has_debuff_of_type(targetname, typename)
	local fnd = false;

	for i=1,16,1 do _, _, _, _, debuffType, timeleft = UnitDebuff(targetname,i)
		if(debuffType ~= nil and string.find(debuffType, typename)) then
			fnd = true;
			break;
		end
	end

	return fnd,timeleft;

end


local CC_jobs = {}
local num_CC_jobs = 0

function set_CC_job(spell, marker)
	num_CC_jobs = num_CC_jobs + 1
	CC_jobs[marker] = spell
end

function unset_CC_job(marker)
	num_CC_jobs = num_CC_jobs - 1
	CC_jobs[marker] = nil;
end

function unset_all_CC_jobs()
	CC_jobs = nil
end

function do_CC_jobs()

	for marker, spell in pairs(CC_jobs) do
		target_unit_with_marker(marker);

		if UnitExists("target") and not UnitIsDead("target") then
			a, d = has_debuff("target", spell)
			if not a then
				CastSpellByName(spell)
				return true;
			elseif d < 3 then
				CastSpellByName(spell)
				return true;
			end
		end
	end

	return false
end

function keep_CCd(targetname, spellname)
	TargetUnit(targetname);
	if UnitExists("target") then
		a, d = has_debuff("target", spellname);
		if not a then
			CastSpellByName(spellname);
			return true;
		elseif d < 6 then
			CastSpellByName(spellname);
			return true;
		end
	end

	return false;
end

function validate_target()

	if BLAST_TARGET_GUID ~= NOTARGET and UnitExists("focus") and BLAST_TARGET_GUID == UnitGUID("focus") then
		if not UnitIsDead("focus") then
			if get_current_config().general_role == "MELEE" then
				if UnitName("focus") == "Krosh Firehand" then
					AssistUnit("Gawk")
					return true;
				end
			end

			if has_debuff("focus", "Polymorph") or has_debuff("focus", "Shackle") then
			 	return false;
			end

			TargetUnit("focus");
			return true;
		else
			clear_target()
			return false;
		end
	else
		return false;
	end

end

function cipher_GUID(GUID)
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

function decipher_GUID(ciphered)
-- this works because XOR is reversible ^^
	return cipher_GUID(ciphered);
end


function get_int_from_strbool(strbool)
	local rval = -1;
	if strbool ~= nil then
		if strbool == "on" or strbool == "1" or strbool == 1 then
			rval = 1;
		elseif strbool == "off" or strbool == "0" or strbool == 0 then
			rval = 0;
		end
	end

	return rval;
end

function pairs_by_key(t, f)
	local a = {}
		for n in pairs(t) do table.insert(a, n) end
		table.sort(a, f)
		local i = 0      -- iterator variable
		local iter = function ()   -- iterator function
			i = i + 1
			if a[i] == nil then return nil
			else return a[i], t[a[i]]
			end
		end
	return iter
end

function get_list_of_keys(dict)
	if not dict or next(dict) == nil then
		return "-none-"
	end

	local key_tab, n = {}, 1;

	for k, _ in pairs_by_key(dict) do
		key_tab[n] = k;
		n = n + 1;
	end

	return table.concat(key_tab, ", "); -- alphabetical sort.

end

function tokenize_string(str, sep)
        local sep, fields = sep or ":", {}
        local pattern = string.format("([^%s]+)", sep)
        str:gsub(pattern, function(c) fields[#fields+1] = c end)
        return fields
end

function trim_string(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local guild_members = {
	["Adieux"] = 1,
	["Bogomips"] = 2,
	["Ceucho"] = 3,
	["Consona"] = 4,
	["Crq"] = 5,
	["Dissona"] = 6,
	["Gawk"] = 7,
	["Gyorgy"] = 8,
	["Igop"] = 9,
	["Jobim"] = 10,
	["Josp"] = 11,
	["Kasio"] = 12,
	["Kusip"] = 13,
	["Mam"] = 14,
	["Meisseln"] = 15,
	["Mulck"] = 16,
	["Nilck"] = 17,
	["Noctur"] = 18,
	["Pehmware"] = 19,
	["Pogi"] = 20,
	["Puhveln"] = 21,
	["Pussu"] = 22,
	["Ribb"] = 23,
	["Teline"] = 24,
	["Viginti"] = 25,
}

for name, num in pairs(guild_members) do
    HEALS_IN_PROGRESS[name] = {};
end

function get_guild_members()
	return guild_members
end

function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

function get_durability_status()
	for i=1,10,1 do
		local dur, max = GetInventoryItemDurability(i)
		if dur and dur == 0 then
			return false
		end
	end

	for i=16,18,1 do
		local dur, max = GetInventoryItemDurability(i)
		if dur and dur == 0 then
			return false
		end
	end

	return true
end

function get_item_bag_position(itemLink)

    for bag = 0, NUM_BAG_SLOTS do
        for slot = 1, GetContainerNumSlots(bag) do
            if (GetContainerItemLink(bag, slot) == itemLink) then
                return bag, slot
            end
        end
    end

	return nil,nil

end

function in_raid_group()
	if GetNumPartyMembers() > 0 then
		if GetNumRaidMembers() > 0 then
			return true;
		end
	end
	return false;
end


function get_raid_groups()

    local groups = {[1] = {}};

    if GetNumRaidMembers() == 0 then
        local name = UnitName("player");
        table.insert(groups[1], name);
        local num_party_members = GetNumPartyMembers();
        for i = 1, num_party_members do
            local name = UnitName("party" .. i);
            table.insert(groups[1], name);
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if not groups[raid_info[3]] then
                groups[raid_info[3]] = {};
            end
            table.insert(groups[raid_info[3]], raid_info[1]);
            i = i + 1;
        end
    end

    return groups;

end

function get_raid_HP_deficits_grouped(groups)

    local grouped_deficits = {}

    for grp, tbl in pairs(groups) do
        grouped_deficits[grp] = {};
        for i, name in pairs(tbl) do
            if UnitExists(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
                local deficit = UnitHealthMax(name) - UnitHealth(name);
                table.insert(grouped_deficits[grp], name, deficit)
            end
        end
    end

    return grouped_deficits;

end

function get_CoH_eligible_groups(groups, min_deficit, max_inelibigle_chars)

    if min_deficit == nil then min_deficit = 2000; end
    if max_inelibigle_chars == nil then max_inelibigle_chars = 1; end

    local grouped_deficits = {}

    for grp, tbl in pairs(groups) do
        local group = {};
        local group_eligible = false;
        for i, name in pairs(tbl) do
            group_eligible = true;
            local num_inelibigle_chars = 0;
            if (not UnitExists(name) or UnitIsDead(name) or has_buff(name, "Spirit of Redemption") or UnitHealthMax(name) - UnitHealth(name) < min_deficit) then
                num_inelibigle_chars = num_inelibigle_chars + 1;
                if num_inelibigle_chars > max_inelibigle_chars then
                    group_eligible = false;
                    break;
                end
            else
                local deficit = UnitHealthMax(name) - UnitHealth(name);
                table.insert(group, name, deficit);
            end
        end
        if group_eligible then
            table.insert(grouped_deficits, grp, group);
        end
    end

    return grouped_deficits;

end

function get_heal_targets(healer)
    return HEALER_TARGETS[healer];
end

function get_raid_heal_target()

    local health_points = get_HPs();
    local heals_in_progress = shallowcopy(HEALS_IN_PROGRESS);
    local lowest = nil;
    local lowest_hp = 0;
    for target, hp in pairs(health_points) do
        if not lowest then
            lowest = target;
            lowest_hp = hp;
        end
        local tmp_hp = hp;
        for healer, info in pairs(heals_in_progress[target]) do
            if healer ~= UnitName("player") and info[3] > GetTime()*1000 then
                tmp_hp = tmp_hp + info[1];
            end
        end
        if tmp_hp < lowest_hp then
            lowest_hp = tmp_hp;
            lowest = target;
        end
    end

    if lowest then
        return lowest;
    else
        return "player";
    end

end

function handle_healer_assignment(message)
    -- message: healer1:target1,target2,...,targetN.healerN:target1,...

    if message == "reset" then
        HEALER_TARGETS = {}
        echo("Wiped out healer assignments.");
        return
    end
    local per_healer = {strsplit(".", message)};
    echo("Healer assignments change:")
    for key, healer_targets in pairs(per_healer) do
        local ht = {strsplit(":", healer_targets)};
        local healer = ht[1];
        if not table.contains(HEALERS, healer) then
            echo("No such healer: " .. "'" .. healer .. "'");
            return;
        end
        local targets = {};
        if ht[2] == nil or ht[2] == "" then
            ht[2] = "(none)";
        else
            targets = {strsplit(",", ht[2])};
            local guildies = get_guild_members();
            for i, target in pairs(targets) do
                if not guildies[target] and target ~= "raid" then
                    echo("Not a valid target: " .. "'" .. target .. "'");
                    return;
                end
            end
        end
        HEALER_TARGETS[healer] = targets;
        echo(healer .. ": " .. ht[2]);
    end

end