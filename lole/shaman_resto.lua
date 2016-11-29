
local function refresh_ES(hottargets)

	if not hottargets or not hottargets[1] then return false; end

    local targetname = hottargets[1];
    if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then return false end

	if not has_buff(targetname, "Earth Shield") then
		TargetUnit(targetname)
		cast_heal("Earth Shield");
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
["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Frost Resistance Totem"
}

local function NS_heal_on_tank()

end

local function raid_heal()

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_heal("Lesser Healing Wave");
    else
        local target, bounce1, bounce2 = get_CH_target_trio(get_serialized_heals());
        if not target then
            return false;
        end
        TargetUnit(target);
        CH_BOUNCE_1 = bounce1
        CH_BOUNCE_2 = bounce2
        cast_heal("Chain Heal(Rank 4)");
    end

    return true;

end

combat_shaman_resto = function()

	if casting_legit_heal() then return end

	if not has_buff("player", "Water Shield") then
		CastSpellByName("Water Shield");
		return;
	end

	if refresh_totems(TOTEMS) then return; end
    if refresh_ES(get_assigned_hottargets(UnitName("player"))) then return end

    if UnitMana("player") < 5000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    TargetUnit(heal_targets[1]);

		local target_HPP = health_percentage("target")

    if target_HPP < 30 then
        cast_heal("Lesser Healing Wave");

		elseif health_percentage("player") < 30 then
        TargetUnit("player");
        cast_heal("Lesser Healing Wave");

	  elseif target_HPP < 70 then
        cast_heal("Healing Wave");

	  elseif health_percentage("player") < 50 then
        TargetUnit("player");
        cast_heal("Healing Wave");

	  elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
