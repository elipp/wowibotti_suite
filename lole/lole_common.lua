function echo(text) 
    DEFAULT_CHAT_FRAME:AddMessage(text)
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


local raid_target_indices = {};

raid_target_indices["star"] = 1;
raid_target_indices["circle"] = 2;
raid_target_indices["diamond"] = 3;
raid_target_indices["triangle"] = 4;
raid_target_indices["crescent"] = 5;
raid_target_indices["square"] = 6;
raid_target_indices["cross"] = 7;
raid_target_indices["skull"] = 8;

function get_desired_buffs(role)

    local dps_buffs = {
        "Blessing of Salvation",
        "Blessing of Kings",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Arcane Intellect",
        "Mark of the Wild"
    };

    local healer_buffs = {
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Arcane Intellect",
        "Mark of the Wild"
    };

    local desired_buffs;
    if role == "dps" then
        desired_buffs = dps_buffs;
    elseif role == "healer" then
        desired_buffs = healer_buffs;
    else
        return false;
    end

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;
end

BUFF_ALIASES = {
	["Arcane Intellect"] = "Arcane Brilliance",
--	["Arcane Brilliance"] = "Arcane Intellect",
	
	["Mark of the Wild"] = "Gift of the Wild",
--	["Gift of the Wild"] = "Mark of the Wild",
	
	["Power Word: Fortitude"] = "Prayer of Fortitude",
--	["Prayer of Fortitude"] = "Power Word: Fortitude",
	
	["Divine Spirit"] = "Prayer of Spirit",
--	["Prayer of Spirit"] = "Divine Spirit",
	
	["Blessing of Kings"] = "Greater Blessing of Kings",
--	["Greater Blessing of Kings"] = "Blessing of Kings",
	
	["Blessing of Wisdom"] = "Greater Blessing of Wisdom",
--	["Greater Blessing of Wisdom"] = "Blessing of Wisdom",
	
	["Blessing of Salvation"] = "Greater Blessing of Salvation",
--	["Greater Blessing of Salvation"] = "Blessing of Salvation"

    ["Blessing of Might"] = "Greater Blessing of Might",
};

CS_CASTING, CS_TIMESTAMP, CS_CASTTIME, CS_TARGET = 1, 2, 3, 4;
NOT_CASTING = { false, 0.0, 0.0, "none" };
cast_state = NOT_CASTING;

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
	for i=1,4,1 do local exists = GetPartyMember(i)
        local name = "party" .. i;
        if has_debuff(name, debuffname) then
            SendChatMessage(name .. "'s got " .. debuffname .. ". Dispelling!!!!XD", "YELL")
			TargetUnit(name);
          --  CastSpellByName("Cleanse");
			CastSpellByName("Dispel Magic")
            return true;
		end
	end
	return false;
end

function healer_move_into_range()
	DelIgnore(LOLE_OPCODE_HEALER_RANGE_CHECK);
end

function stopfollow()
-- OPCODE_FOLLOW with arg 0 is a special case in walk_to_unit_with_GUID(), just takes a click to move to the player's current loc
	DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. "0x0000000000000000");
end

function get_num_buff_requests(buffs)

	-- buffs is a "buffname" -> "table of character names with this particular buff request" map

	local requests = {}
	local request_amount = 0;

    for buff, chars in pairs(buffs) do
        for char in pairs(chars) do
            if not requests[char] then
                requests[char] = true;
                request_amount = request_amount + 1;
            end
        end
    end
	
	return request_amount;

end

function get_spam_table(buffs, group_buff_map)
    
    local groups = {[1] = {}, [2] = {}};
    if GetNumRaidMembers() == 0 then
        for buff, chars in pairs(buffs) do
            for char in pairs(chars) do
                groups[1][char] = true;
            end
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if raid_info[3] == 1 then
                groups[1][raid_info[1]] = true;
            elseif raid_info[3] == 2 then
                groups[2][raid_info[1]] = true;
            end
            i = i + 1;
        end
    end

    local grouped_requests = {};
    for buff, chars in pairs(buffs) do
        grouped_requests[buff] = {[1] = {}, [2] = {}};
        for char in pairs(chars) do
            if groups[1][char] then
                table.insert(grouped_requests[buff][1], char);
            elseif groups[2][char] then
                table.insert(grouped_requests[buff][2], char);
            end
        end
    end

    local spam_table = {};
    for buff, groups in pairs(grouped_requests) do
        for group, chars in pairs(groups) do
            if #chars > 2 and group_buff_map[buff] then
                table.insert(spam_table, {[chars[1]] = group_buff_map[buff]});
            else
                for key in pairs(chars) do
                    table.insert(spam_table, {[chars[key]] = buff});
                end
            end
        end
    end

    return spam_table;

end

function get_paladin_spam_table(buffs, num_requests)

    local buff_order = {
        [1] = "Greater Blessing of Salvation",
        [2] = "Greater Blessing of Kings",
        [3] = "Greater Blessing of Wisdom",
        [4] = "Greater Blessing of Might",
    };

    local spam_table = {};
    local buffed_characters = {};
    local buffed_classes = {};
    if num_requests > 4 then
        for key, buff in ipairs(buff_order) do
            local buff_given = {};
            local chars = buffs[buff];
            if chars then
                for character in pairs(chars) do
                    local class = UnitClass(character);
                    if buff_given[class] then
                        buffed_characters[character] = true;
                    elseif not buffed_classes[class] then
                        table.insert(spam_table, {[character] = buff});
                        buffed_characters[character] = true;
                        buff_given[class] = true;
                        buffed_classes[class] = true;
                    end
                end
            end
        end
    end

    for key, buff in ipairs(buff_order) do
        local chars = buffs[buff];
        if chars then
            local temp = {strsplit(" ", buff)};
            table.remove(temp, 1);
            local short_buff = table.concat(temp, " ");
            for character in pairs(chars) do
                if not buffed_characters[character] then
                    table.insert(spam_table, {[character] = short_buff});
                    buffed_characters[character] = true;
                end
            end
        end
    end

    return spam_table;

end

function buffs()

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
        LOLE_CLASS_CONFIG.MODE_ATTRIBS["buffmode"] = 0;
        LBUFFCHECK_ISSUED = false;
        echo("lole_set: attrib \"buffmode\" set to 0");
    end

end

function buff_self()

    if (GetTime() - BUFF_TIME) < 1.8 then
        return false;
    else
        CastSpellByName(SELF_BUFF_SPAM_TABLE[1]);
        BUFF_TIME = GetTime();
    end
    table.remove(SELF_BUFF_SPAM_TABLE, 1);

end

function lole_selfbuffs()
    if SELF_BUFF_SPAM_TABLE[1] ~= nil then
        buff_self();
    else
        LOLE_CLASS_CONFIG.MODE_ATTRIBS["buffmode"] = 0;
        echo("lole_set: attrib \"buffmode\" set to 0");
    end
end

function get_num_paladins()

    local num_paladins = 0;

    if GetNumRaidMembers() == 0 then
        if UnitClass("player") == "Paladin" then
            num_paladins = num_paladins + 1;
        end
        local num_party_members = GetNumPartyMembers();
        for i = 1, num_party_members do
            if UnitClass("party" .. i) == "Paladin" then
                num_paladins = num_paladins + 1;
            end
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if raid_info[3] == 1 or raid_info[3] == 2 then
                if raid_info[5] == "Paladin" then
                    num_paladins = num_paladins + 1;
                end
            end
            i = i + 1;
        end
    end

    return num_paladins;

end

function target_mob_with_charm(index_str)

	local index = raid_target_indices[index_str];
	
	if (UnitExists("target") and index == GetRaidTargetIndex("target")) then 
		return 1; 
	end 
	
	AssistUnit("Adieux");
	AssistUnit("Noctur");
	
	if (UnitExists("target") and index == GetRaidTargetIndex("target")) then
		return 1;
	end
	
	ClearTarget();
	return 0;

end

function has_debuff(targetname, debuff_name)
	local fnd = false;
	local timeleft = 999;
	for i=1,16,1 do name, _, _, _, _, _, timeleft = UnitDebuff(targetname,i) 
		if(name ~= nil and string.find(name, debuff_name)) then 
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

	for i=1,16,1 do name, _, _, count = UnitDebuff(targetname, i) 
		if (name ~= nil and string.find(name, debuff_name)) then 
			return count; 
		end
	end
	-- not debuffed with debuff_name
	return 0;

end


function has_buff(targetname, buff_name)
	local fnd = false;
	for i=1,16,1 do name, _, icon, _, _, timeLeft = UnitBuff(targetname, i) 
		if(name ~= nil and string.find(name, buff_name)) then 
			fnd=true;
			break; 
		end
	end
	return fnd, timeleft;

end

function has_debuff_of_type(targetname, typename)
	local fnd = false;

	for i=1,16,1 do _, _, _, _, debuffType, timeLeft = UnitDebuff(targetname,i) 
		if(debuffType ~= nil and string.find(debuffType, typename)) then 
			fnd = true;
			break; 
		end
	end
	
	return fnd,timeLeft;

end

