local function raid_heal()
    local target = get_raid_heal_target();
    if target then
        TargetUnit(target);
    else
        TargetUnit("player");
    end
    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    local has, timeleft, stacks = has_buff("player", "Light's Grace");

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_heal("Holy Light");
    elseif (health_cur < health_max * 0.50) then
        cast_heal("Holy Light");
    elseif (health_cur < health_max * 0.80) then
        if not has or timeleft < 3 then
            cast_heal("Holy Light(Rank 5)");
        else
            cast_heal("Flash of Light");
        end
    elseif not has or timeleft < 3 then
        cast_heal("Holy Light(Rank 1)");
    else
        return false;
    end

    return true;

end

combat_paladin_holy = function()

    if UnitHealth("player") < 2000 then
        if (GetSpellCooldown("Divine Shield") == 0) then
            SpellStopCasting();
            CastSpellByName("Divine Shield");
        end
    end

    if casting_legit_heal() then return end
    
	local mana_left = UnitMana("player");
	if mana_left < 4000 then
		CastSpellByName("Divine Illumination");
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

    local health_max = UnitHealthMax("target");
    local health_cur = UnitHealth("target");

    local has, timeleft, stacks = has_buff("player", "Light's Grace");

    if (health_cur < health_max * 0.30) then
        CastSpellByName("Divine Favor");
        CastSpellByName("Divine Illumination");
        cast_heal("Holy Light");
    elseif (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_heal("Holy Light");
    elseif (health_cur < health_max * 0.70) then
        cast_heal("Holy Light");
    elseif (UnitHealth("player") < UnitHealthMax("player")*0.50) then
        TargetUnit("player");
        cast_heal("Holy Light");
    elseif (health_cur < health_max * 0.90) then
        if not has or timeleft < 3 then
            cast_heal("Holy Light(Rank 5)");
        else
            cast_heal("Flash of Light");
        end
    else
        raid_heal();
    end

end
