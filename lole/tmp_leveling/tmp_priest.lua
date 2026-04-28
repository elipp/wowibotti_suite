local function shadow()
    stopfollow()
    caster_range_check(3, 25)
    if (health_percentage("target") > 90 or UnitHealth("target") > 90) and
        not has_debuff("target", "Shadow Word: Pain") then
        L_CastSpellByName("Shadow Word: Pain")
    end
    if off_cd("Mind Blast") then
        L_CastSpellByName("Mind Blast")
    end
    L_CastSpellByName("Smite")
    --L_StartAttack();
end

function tmp_priest_combat()
    if not UnitAffectingCombat("player") then
        buff_if_eligible("Power Word: Fortitude", "Brah")
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));

    if heal_targets[1] == 'raid' then
        local target, urgencies = get_raid_heal_target(true);
        if not target then
            return
        else
            L_TargetUnit(target);
        end
    end

    local target_HPP = health_percentage("target")
    local has_renew, renew_timeleft = has_buff("target", "Renew");

    -- if group_dispel() then return end
    -- if UnitGUID("target") ~= UnitGUID("player") and health_percentage("player") < 75 then
    --     cast_heal("Binding Heal");
    if target_HPP < 30 then
        cast_heal("Heal")
        -- elseif target_HPP < 55 and GetSpellCooldown("Prayer of Mending") == 0 then
        --     cast_heal("Prayer of Mending")
    elseif target_HPP < 85 and not has_renew then
        cast_heal("Renew")
        -- elseif (GetSpellCooldown("Circle of Healing") == 0) and (coh_target.total_deficit > 3500 and coh_target.num_targets > 2 and coh_target.average_deficit > 500) then
        --     L_TargetUnit(coh_target.name);
        --     return cast_heal("Circle of Healing");
    elseif target_HPP < 60 then
        cast_heal("Lesser Heal")
    end
end
