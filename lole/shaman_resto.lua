
local function refresh_ES(targetname)

	if not targetname then return false; end

	if not UnitExists(targetname) or UnitIsDead(targetname) then return false end

	if not has_buff(targetname, "Earth Shield") then
		TargetUnit(targetname)
		caster_range_check(35)
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
	-- 	TargetUnit("Krosh Firehand")
	-- 	if UnitExists("target") and not UnitIsDead("target") then
	-- 		TargetUnit(mage_tank);
	--         caster_range_check(35);
	--         if (UnitHealthMax(mage_tank) - UnitHealth(mage_tank)) > 5000 then
	--             cast_spell("Healing Wave")
	--         elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
	--             TargetUnit("player");
	--             cast_spell("Healing Wave")
	-- 		elseif (UnitHealthMax(mage_tank) - UnitHealth(mage_tank)) > 2000 then
	-- 			cast_spell("Lesser Healing Wave")
	-- 		end
	--         return;
	-- 	end
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



end
