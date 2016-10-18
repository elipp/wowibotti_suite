local function should_cast_tranquility()
    if GetSpellCooldown("Tranquility") > 0 then
        return false;
    end

    local r = false;
    local HP_deficits = get_HP_deficits(true);
    if next(HP_deficits) == nil then return false; end

    local num_deficients = 0;
    for unit, deficit in pairs(HP_deficits) do
        if deficit > 5000 then
            num_deficients = num_deficients + 1;
            if num_deficients > 2 then
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

    caster_range_check(35);

    local has_rg, timeleft_rg, stacks_rg = has_buff("target", "Regrowth");
    local has_rj, timeleft_rj, stacks_rj = has_buff("target", "Rejuvenation");
    local has_lb, timeleft_lb, stacks_lb = has_buff("target", "Lifebloom");
    if should_cast_tranquility() then
        CastSpellByName("Barkskin");
        cast_spell("Tranquility");
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        CastSpellByName("Barkskin");
        TargetUnit("player");
        CastSpellByName("Swiftmend")
        cast_spell("Regrowth");
    elseif (health_cur < health_max * 0.35) then
        CastSpellByName("Swiftmend");
        if not has_rg or timeleft_rg < 5 then
            cast_spell("Regrowth");
        end
    elseif (health_cur < health_max * 0.60) then
        if not has_rj then
            CastSpellByName("Rejuvenation");
        elseif not has_lb or stacks_lb < 3 then
            CastSpellByName("Lifebloom");
        end
    elseif (health_cur < health_max * 0.80) then
        if not has_lb or stacks_lb < 3 then
            CastSpellByName("Lifebloom");
        end
    else
        return false;
    end

    return true;

end

combat_druid_resto = function()

    if casting_legit_heal() then return end
    if UnitChannelInfo("player") then return; end -- tranquility

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

    caster_range_check(35);

    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    if (health_cur < health_max * 0.35) then
        CastSpellByName("Swiftmend");
        if not has_rg or timeleft_rg < 5 then
            cast_spell("Regrowth");
        end
        return;
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.30 then
        CastSpellByName("Barkskin");
        TargetUnit("player");
        CastSpellByName("Swiftmend")
        cast_spell("Regrowth");
        return;
    end

	if has_lb then
		if stacks_lb < 3 then
			CastSpellByName("Lifebloom")
			return
		elseif timeleft_lb < 1.2 then
			CastSpellByName("Lifebloom")
			return
		end
	else
		CastSpellByName("Lifebloom")
		return
	end

	if not has_rj then
		CastSpellByName("Rejuvenation")
		return
	end

    if should_cast_tranquility() then
        CastSpellByName("Barkskin");
        cast_spell("Tranquility");
        return;
    end

    if UnitHealth("player") < UnitHealthMax("player")*0.50 then
        TargetUnit("player");
        if (not has_buff("target", "Rejuvenation")) then
            CastSpellByName("Rejuvenation");
            return;
        end
    end

    if table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
