local function should_cast_tranquility(min_deficit, min_healable_chars)
    if min_deficit == nil then min_deficit = 5000; end
    if min_healable_chars == nil then min_healable_chars = 4; end

    if GetSpellCooldown("Tranquility") > 0 then
        return false;
    end

    local r = false;
    local HP_deficits = get_HP_deficits(true, false);

    local num_deficients = 0;
    for unit, deficit in pairs(HP_deficits) do
        local distance_to_unit = get_distance_between("player", UnitName(unit));
        if distance_to_unit <= 30 and deficit > min_deficit then
            num_deficients = num_deficients + 1;
            if num_deficients == min_healable_chars then
                r = true;
                break;
            end
        end
    end

    return r;
end

local function raid_heal()
    local target = get_raid_heal_target();
    if target then
        TargetUnit(target);
    else
        TargetUnit("player");
    end
    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    local has_rg, timeleft_rg, stacks_rg = has_buff("target", "Regrowth");
    local has_rj, timeleft_rj, stacks_rj = has_buff("target", "Rejuvenation");
    local has_lb, timeleft_lb, stacks_lb = has_buff("target", "Lifebloom");
    if should_cast_tranquility() then
        CastSpellByName("Barkskin");
        CastSpellByName("Tranquility");
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        CastSpellByName("Barkskin");
        TargetUnit("player");
        cast_heal("Swiftmend")
        cast_heal("Regrowth");
    elseif (health_cur < health_max * 0.35) then
        cast_heal("Swiftmend");
        if not has_rg or timeleft_rg < 5 then
            cast_heal("Regrowth");
        end
    elseif (health_cur < health_max * 0.60) then
        if not has_rj then
            cast_heal("Rejuvenation");
        elseif not has_lb or stacks_lb < 3 then
            cast_heal("Lifebloom");
        end
    elseif (health_cur < health_max * 0.80) then
        if not has_lb or stacks_lb < 3 then
            cast_heal("Lifebloom");
        end
    else
        return false;
    end

    return true;

end

combat_druid_resto = function()

    if casting_legit_heal() then return end
    if UnitChannelInfo("player") then return; end -- tranquility

    if (not has_buff("player", "Tree of Life")) then
        CastSpellByName("Tree of Life")
        return
    end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Innervate") == 0 then
		CastSpellByName("Innervate", "player");
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
	local has_rg, timeleft_rg, stacks_rg = has_buff("target", "Regrowth");
    local has_rj, timeleft_rj, stacks_rj = has_buff("target", "Rejuvenation");
    local has_lb, timeleft_lb, stacks_lb = has_buff("target", "Lifebloom");

    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    if (health_cur < health_max * 0.35) then
        cast_heal("Swiftmend");
        if not has_rg or timeleft_rg < 5 then
            cast_heal("Regrowth");
        end
        return;
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.30 then
        CastSpellByName("Barkskin");
        TargetUnit("player");
        cast_heal("Swiftmend")
        cast_heal("Regrowth");
        return;
    end

	if has_lb then
		if stacks_lb < 3 then
			cast_heal("Lifebloom")
			return
		elseif timeleft_lb < 1.2 then
			cast_heal("Lifebloom")
			return
		end
	else
		cast_heal("Lifebloom")
		return
	end

	if not has_rj then
		cast_heal("Rejuvenation")
		return
	end

    if should_cast_tranquility() then
        CastSpellByName("Barkskin");
        CastSpellByName("Tranquility");
        return;
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.50 then
        TargetUnit("player");
        if (not has_buff("target", "Rejuvenation")) then
            cast_heal("Rejuvenation");
            return;
        end
    end

    if table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
