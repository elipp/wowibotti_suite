config_shaman_resto = {}
config_shaman_resto.name = "shaman_resto";

config_shaman_resto.MODE_ATTRIBS = {
    ["combatbuffmode"] = 0,
    ["buffmode"] = 0,
    ["playermode"] = 0
};

config_shaman_resto.SELF_BUFFS = {"Water Shield"}; -- UNCOMMENT FOR PVP

local function refresh_ES(targetname) 
	
	if not UnitExists(targetname) then return false end
	
	if not has_buff(targetname, "Earth Shield") then
		TargetUnit(targetname)
		CastSpellByName("Earth Shield");
		return true;
	end

	return false
	
end

local function get_group_member_HP_deficits() 

	local group_member_HP_deficits = {};

	group_member_HP_deficits["player"] = UnitHealthMax("player") - UnitHealth("player");
	for i=1,4,1 do local exists = GetPartyMember(i)
		local name = "party" .. i;
		if exists and not UnitIsDead(name) then
			group_member_HP_deficits[name] = UnitHealthMax(name) - UnitHealth(name);
		end
	end
	
	return group_member_HP_deficits;

end

local function get_lowest_hp(hp_deficits) 
	
	local lowest = nil;
	
	for name,hp_deficit in pairs(hp_deficits) do
		
		if not lowest then 
			lowest = name;
		else
			if hp_deficit > hp_deficits[lowest] then
				lowest = name;
			end
		end
	end
	
	return lowest;

end

local function get_total_deficit(hp_deficits) 
	local total = 0;
	
	for name, deficit in pairs(hp_deficits) do
		total = total + deficit
	end
	
	return total;

end

local TOTEMS = {
["air"] = "Windfury Totem", 
["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
["water"] = "Mana Spring Totem",
["fire"] = "Frost Resistance Totem"
}


local function refresh_totems()
	for slot,name in pairs(TOTEMS) do
		if recast_totem_if_noexists_or_OOR(name) then return true; end
	end
	return false;
end


config_shaman_resto.combat = function()
	
	local ES_TARGET = "Adieux";
	
	if casting_legit_heal() then return end
	
	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	if refresh_totems() then return; end
	
	if refresh_ES(ES_TARGET) then return end
	
	local HP_deficits = get_group_member_HP_deficits();
	if not HP_deficits then return; end
	
	local lowest = get_lowest_hp(HP_deficits);
	if not lowest then return; end
	
	if (HP_deficits[lowest] < 1500) then 
		return;
	end
	
	caster_range_check(35);
	TargetUnit(lowest);
	
	local total_deficit = get_total_deficit(HP_deficits)

	if (HP_deficits[lowest] > 7000) then
		cast_if_nocd("Nature's Swiftness");
		UseInventoryItem(13);
		UseInventoryItem(14);
		cast_spell("Healing Wave");

		return;
	end
	
	if HP_deficits[lowest] > 4000 then
		cast_spell("Healing Wave")

		return;
	end
	
	if total_deficit > 6000 then
		cast_spell("Chain Heal")

		return;
	end
	

end

config_shaman_resto.cooldowns = function() 

end

config_shaman_resto.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
        config_shaman_resto.MODE_ATTRIBS["buffmode"] = 0;
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
