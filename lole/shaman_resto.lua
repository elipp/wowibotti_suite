config_shaman_resto = {}
config_shaman_resto.name = "shaman_resto";

config_shaman_resto.MODE_ATTRIBS = {
    ["combatbuffmode"] = 0,
    ["buffmode"] = 0,
    ["playermode"] = 0
};

config_shaman_resto.SELF_BUFFS = {"Water Shield"}; -- UNCOMMENT FOR PVP


config_shaman_resto.combat = function()
	--DEFAULT_CHAT_FRAME:AddMessage(tostring(cast_state[CS_CASTING]) .. ", " .. cast_state[CS_TIMESTAMP] .. ", " .. cast_state[CS_CASTTIME])
	
	
	if casting_legit_heal() then return end
	
	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	--if recast_totem_if_not_active_or_in_range("Strength of Earth Totem") then return; end
	if recast_totem_if_not_active_or_in_range("Tremor Totem") then return; end
	--if recast_totem_if_not_active_or_in_range("Stoneskin Totem") then return; end

	if recast_totem_if_not_active_or_in_range("Windfury Totem") then return; end
	if recast_totem_if_not_active_or_in_range("Mana Spring Totem") then return; end
	if recast_totem_if_not_active_or_in_range("Frost Resistance Totem") then return; end

	
	local group_member_hpdeficits = {};

	group_member_hpdeficits["player"] = UnitHealthMax("player") - UnitHealth("player");
	for i=1,4,1 do local exists = GetPartyMember(i)
		local name = "party" .. i;
		if exists and not UnitIsDead(name) then
			group_member_hpdeficits[name] = UnitHealthMax(name) - UnitHealth(name);
		end
	end
	
	local ESTARGET = "Adieux";
	
	if not has_buff(ESTARGET, "Earth Shield") then
		TargetUnit(ESTARGET);
		CastSpellByName("Earth Shield");
		return;
	end

	local lowest = nil;
	
	for name,hp_deficit in pairs(group_member_hpdeficits) do
		if not lowest then 
			lowest = name;
		else
			if hp_deficit > group_member_hpdeficits[lowest] then
				lowest = name;
			end
		end
	end
	
	
	if not lowest then return; end
	
	if (group_member_hpdeficits[lowest] < 1500) then 
		return;
	end

	TargetUnit(lowest);
	caster_range_check(35);

	if (group_member_hpdeficits[lowest] > 7000) then
		cast_if_nocd("Nature's Swiftness");
		UseInventoryItem(13);
		UseInventoryItem(14);
		cast_spell("Healing Wave");
		return;
	end
	
	if (group_member_hpdeficits[lowest] > 3750) then
		--cast_spell("Healing Wave");
		cast_spell("Chain Heal");
		return;
	end
	
	if (group_member_hpdeficits[lowest] > 2500) then	
		cast_spell("Chain Heal");
		return;
	else
		cast_spell("Chain Heal");
		return;
	end


end

config_shaman_resto.cooldowns = function() 

end

config_shaman_resto.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
        config_shaman_resto.MODE_ATTRIBS["buffmode"] = 0;
        SendAddonMessage("lole_bufferstatus", "0", "RAID", UnitName("player")); 
        echo("lole_set: attrib \"buffmode\" set to 0");
    else
        buff_self();
    end

end

config_shaman_resto.desired_buffs = function()

    local desired_buffs = get_desired_buffs("healer");
    return desired_buffs;

end

config_shaman_resto.other = function()

end;
