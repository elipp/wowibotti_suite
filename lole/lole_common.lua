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

function healer_move_into_range()
	DelIgnore(LOLE_OPCODE_HEALER_RANGE_CHECK);
end

function stopfollow()
-- OPCODE_FOLLOW with arg 0 is a special case in walk_to_unit_with_GUID(), just takes a click to move to the player's current loc
	DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. "0x0000000000000000");
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

