
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
--["air"] = "Wrath of Air Totem",

["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Frost Resistance Totem"
}


combat_shaman_resto = function()
	
	local ES_TARGET = "Adieux";
	
	if casting_legit_heal() then return end
	
	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	if refresh_totems(TOTEMS) then return; end
	
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

