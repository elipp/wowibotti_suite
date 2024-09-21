function tmp_priest_combat()
    if not UnitAffectingCombat("player") then
        buff_if_eligible("Power Word: Fortitude", "Krahu")
    end

    if validate_target() then
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
end
