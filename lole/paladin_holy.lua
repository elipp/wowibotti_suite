local function raid_heal()
    local target = get_raid_heal_target();
    if target then
        L_TargetUnit(target);
    else
        L_TargetUnit("player");
    end

    local target_HPP = health_percentage("target")
    local has, timeleft, stacks = has_buff("player", "Light's Grace");

    if health_percentage("player") < 30 then
        L_TargetUnit("player");
        cast_heal("Holy Light");

    elseif target_HPP < 50 then
        cast_heal("Holy Light");

    elseif target_HPP < 80 then
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
            L_CastSpellByName("Divine Shield");
        end
    end

    if casting_legit_heal() then return end

	if mana_percentage("player") < 40 then
		L_CastSpellByName("Divine Illumination");
	end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    L_TargetUnit(heal_targets[1]);

    local target_HPP = health_percentage("target")

    local has, timeleft, stacks = has_buff("player", "Light's Grace");

    if target_HPP < 30 then
        L_CastSpellByName("Divine Favor");
        L_CastSpellByName("Divine Illumination");
        cast_heal("Holy Light");

    elseif health_percentage("player") < 30 then
        L_TargetUnit("player");
        cast_heal("Holy Light");

    elseif target_HPP < 70 then
        cast_heal("Holy Light");

    elseif health_percentage("player") < 50 then
        L_TargetUnit("player");
        cast_heal("Holy Light");

    elseif target_HPP < 90 then
        if not has or timeleft < 3 then
            cast_heal("Holy Light(Rank 5)");
        else
            cast_heal("Flash of Light");
        end

    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    elseif not has or timeleft < 3 then
        cast_heal("Holy Light(Rank 1)");
    end

end
