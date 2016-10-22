local pom_time = 0;
local mt_healer = true;

local function should_cast_PoH(min_deficit, min_healable_chars)
    if min_deficit == nil then min_deficit = 3000; end
    if min_healable_chars == nil then min_healable_chars = 4; end

	local r = false;
	local HP_deficits = get_HP_deficits(true, true);

	local num_deficients = 0;
	for unit, deficit in pairs(HP_deficits) do
        local distance_to_unit = get_distance_between("player", UnitName(unit));
        if distance_to_unit <= 36 and deficit > min_deficit then
			num_deficients = num_deficients + 1;
			if num_deficients == min_healable_chars then
				r = true;
				break;
			end
		end
	end

	return r;
end

local function get_CoH_target(urgencies, min_deficit, max_ineligible_chars)
    if get_current_config().name == "priest_holy_ds" then
        return nil;
    end

    local highest_total_urgency = 0;
    local coh_target;

    local eligible_groups = get_CoH_eligible_groups(get_raid_groups(), min_deficit, max_ineligible_chars);
    if table.get_num_elements(eligible_groups) == 1 then
        coh_target = next(eligible_groups);
    else
        for tar, group in pairs(eligible_groups) do
            local total_urgency = 0;
            for i, name in pairs(group) do
                if urgencies[name] then
                    total_urgency = total_urgency + urgencies[name];
                end
            end
            if total_urgency > highest_total_urgency then
                highest_total_urgency = total_urgency;
                coh_target = tar;
            end
        end
    end

    return coh_target;
    
end


local function raid_heal()
    local target, urgencies = get_raid_heal_target(true);
    if target then
        TargetUnit(target);
    else
        TargetUnit("player");
    end
    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");
    local targeting_self = UnitName("target") == UnitName("player");

    caster_range_check(35);

    if should_cast_PoH(6000, 4) then
        cast_spell("Prayer of Healing");
        return true
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.30 then
        TargetUnit("player");
        cast_if_nocd("Power Word: Shield");
        cast_spell("Flash Heal");
        return true
    end

    if (health_cur < health_max * 0.20) then
        cast_if_nocd("Power Word: Shield");
        cast_spell("Flash Heal");
        return true
    end

    if should_cast_PoH(3000, 4) then
        cast_spell("Prayer of Healing");
        return true
    end

    local coh_target = get_CoH_target(urgencies);
    if coh_target then
        TargetUnit(coh_target);
        cast_if_nocd("Circle of Healing");
    elseif (health_cur < health_max * 0.50) then
        cast_spell("Greater Heal");
    elseif (health_cur < health_max * 0.75) then
        if not targeting_self and (UnitHealth("player") < UnitHealthMax("player")*0.75) then
            cast_spell("Binding Heal");
        else
            cast_spell("Greater Heal(Rank 1)");
        end
    elseif (health_cur < health_max * 0.85) then
        if time() - pom_time > 10 then
            if cast_if_nocd("Prayer of Mending") then
                pom_time = time();
            end
        elseif not has_buff("target", "Renew") then
            cast_if_nocd("Renew");
        end
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
	local targeting_self = UnitName("target") == UnitName("player");

    if (health_cur < health_max * 0.15) then
        cast_if_nocd("Power Word: Shield");
        cast_spell("Flash Heal");
	elseif (health_cur < health_max * 0.30) then
		cast_spell("Greater Heal");
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        TargetUnit("player");
        cast_if_nocd("Power Word: Shield");
        cast_spell("Flash Heal");
    elseif (health_cur < health_max * 0.50) then
        cast_spell("Greater Heal");
    elseif time() - pom_time > 10 then
        if cast_if_nocd("Prayer of Mending") then
            pom_time = time();
        end
    elseif (should_cast_PoH()) then
        cast_spell("Prayer of Healing");
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
		cast_if_nocd("Renew");
    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
