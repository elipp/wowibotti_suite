
local function refresh_ES(targetname)

	if not targetname then return false; end

	if not UnitExists(targetname) then return false end

	if not has_buff(targetname, "Earth Shield") then
		TargetUnit(targetname)
		CastSpellByName("Earth Shield");
		return true;
	end

	return false

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

--["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Frost Resistance Totem"
}

local function NS_heal_on_tank()

end

combat_shaman_resto = function()
    --local mage_tank = "Dissona";
	local mage_tank = MAIN_TANK

	if casting_legit_heal() then return end

	if not has_buff("player", "Water Shield") then
		CastSpellByName("Water Shield");
		return;
	end

	if refresh_totems(TOTEMS) then return; end
	if UnitName("player") == "Igop" then
		if refresh_ES(OFF_TANK) then return end
	else
		if refresh_ES(MAIN_TANK) then return end
	end

    if UnitMana("player") < 5000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

	-- if UnitName("player") == "Pehmware" then
    --     TargetUnit(mage_tank);
    --     caster_range_check(35);
    --     if (UnitHealthMax(mage_tank) - UnitHealth(mage_tank)) > 5000 then
    --         cast_spell("Healing Wave")
    --     elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
    --         TargetUnit("player");
    --         cast_spell("Healing Wave")
	-- 	elseif (UnitHealthMax(mage_tank) - UnitHealth(mage_tank)) > 2000 then
	-- 		cast_spell("Lesser Healing Wave")
	-- 	end
    --     return;
    -- elseif (UnitHealthMax(OFF_TANK) - UnitHealth(OFF_TANK)) > 8000 then
    --     TargetUnit(OFF_TANK);
    --     caster_range_check(35);
    --     cast_spell("Lesser Healing Wave")
    --     return;
	-- end

	target_best_CH_target();
	if not UnitExists("target") then return end

	caster_range_check(35);
	cast_spell("Chain Heal")

	return;

--	local total_deficit = get_total_deficit(HP_deficits)
	--
	-- if (HP_deficits[lowest] > 7000) then
	-- 	if cast_if_nocd("Nature's Swiftness") then
	-- 		UseInventoryItem(13);
	-- 		UseInventoryItem(14);
	-- 		cast_spell("Healing Wave");
	-- 	end
	-- 	return;
	-- end

	-- if HP_deficits[lowest] > 3000 then -- this was actually really good for vexallus :O
	-- 	cast_spell("Lesser Healing Wave")
	-- 	return;
	-- end




end
