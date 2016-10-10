local pom_time = 0;
local mt_healer = true;

local function should_cast_PoH()
	local r = false;
	local HP_deficits = get_HP_deficits(true);
	if next(HP_deficits) == nil then return false; end

	local num_deficients = 0;
	for unit, deficit in pairs(HP_deficits) do
		if deficit > 3000 then
			num_deficients = num_deficients + 1;
			if num_deficients > 2 then
				r = true;
				break;
			end
		end
	end

	return r;
end

local function get_CoH_target()
    if get_current_config().name == "priest_holy_ds" then
        return nil;
    end

    local eligible_groups = get_CoH_eligible_groups(get_raid_groups(), 2000, 1);
    local coh_target = nil;
    local highest_total_deficit = 0;
    for grp, tbl in pairs(eligible_groups) do
        local total_deficit = 0;
        local highest_deficit = 0;
        local char_with_highest_def = "";
        for name, deficit in pairs(tbl) do
            total_deficit = total_deficit + deficit;
            if deficit > highest_deficit then
                highest_deficit = deficit;
                char_with_highest_def = name;
            end
        end
        if total_deficit > highest_total_deficit then
            highest_total_deficit = total_deficit;
            coh_target = char_with_highest_def;
        end
    end

    return coh_target;
    
end


local function raid_heal()
    local target = get_raid_heal_target();
    TargetUnit(target);
    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");
    local targeting_self = UnitName("target") == UnitName("player");

    caster_range_check(35);

    if should_cast_PoH() then
        cast_spell("Prayer of Healing");
        return true
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.30 then
        TargetUnit("player");
        cast_spell("Power Word: Shield");
        cast_spell("Flash Heal");
        return true
    end

    local coh_target = get_CoH_target();
    if coh_target then
        TargetUnit(coh_target);
        cast_spell("Circle of Healing");
    elseif (health_cur < health_max * 0.50) then
        cast_spell("Greater Heal");
    elseif (health_cur < health_max * 0.75) then
        if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.75) then
            cast_spell("Binding Heal");
        else
            cast_spell("Greater Heal(Rank 1)");
        end
    elseif time() - pom_time > 10 then
        if cast_if_nocd("Prayer of Mending") then
            pom_time = time();
            return;
        end
    elseif not has_buff("target", "Renew") then
        cast_spell("Renew");
    else
        return false;
    end

    return true;
end

combat_priest_holy = function()

	if casting_legit_heal() then return end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Shadowfiend") == 0 and validate_target() then
		CastSpellByName("Shadowfiend");
		return;
	end

    local heal_targets = get_heal_targets();
    if heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    local heal_target = get_lowest_hp_char(heal_targets);
    if not heal_target then
        raid_heal();
        return;
    end

    TargetUnit(heal_target);
    caster_range_check(35);

	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	local targeting_self = UnitName("target") == UnitName("player");

    if (health_cur < health_max * 0.15) then
        cast_spell("Power Word: Shield");
        cast_spell("Flash Heal");
	elseif (health_cur < health_max * 0.30) then
		cast_spell("Greater Heal");
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        TargetUnit("player");
        cast_spell("Power Word: Shield");
        cast_spell("Flash Heal");
    elseif (should_cast_PoH()) then
        cast_spell("Prayer of Healing");
    elseif time() - pom_time > 10 then
        if cast_if_nocd("Prayer of Mending") then
            pom_time = time();
        end
	elseif (health_cur < health_max * 0.60) then
		if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.50) then
			cast_spell("Binding Heal");
		else
			cast_spell("Greater Heal");
		end
	elseif (health_cur < health_max * 0.80) then
		if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.70) then
			cast_spell("Binding Heal");
		else
			cast_spell("Greater Heal(Rank 1)");
		end
	elseif not has_buff("target", "Renew") then
		cast_spell("Renew");
    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
