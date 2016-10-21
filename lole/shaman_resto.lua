
local function refresh_ES(targetname)

	if not targetname then return false; end

    if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then return false end

	if not has_buff(targetname, "Earth Shield") then
		TargetUnit(targetname)
		caster_range_check(35)
		cast_if_nocd("Earth Shield");
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

local function raid_heal()

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_spell("Lesser Healing Wave");
    else
        local target, bounce1, bounce2 = get_CH_target_trio(get_serialized_heals());
        if not target then
            return false;
        end
        TargetUnit(target);
        CH_BOUNCE_1 = bounce1
        CH_BOUNCE_2 = bounce2
        caster_range_check(35);
        cast_spell("Chain Heal");
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
	if UnitName("player") == "Igop" then
		if refresh_ES(OFF_TANK) then return end
	else
		if refresh_ES(MAIN_TANK) then return end
	end

    if UnitMana("player") < 5000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

    local heal_targets = get_heal_targets(UnitName("player"));
    if heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    local heal_target = get_single_heal_target(heal_targets);
    if not heal_target then
        raid_heal();
        return;
    end

    TargetUnit(heal_target);
    caster_range_check(35);

    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    if (health_cur < health_max * 0.30) then
        cast_spell("Lesser Healing Wave");
    elseif (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_spell("Lesser Healing Wave");
    elseif (health_cur < health_max * 0.70) then
        cast_spell("Healing Wave");
    elseif (UnitHealth("player") < UnitHealthMax("player")*0.50) then
        TargetUnit("player");
        cast_spell("Healing Wave");
    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
