function tmp_priest_combat()
    if not UnitAffectingCombat("player") then
        buff_if_eligible("Power Word: Fortitude", "Krahu")
    end

    if validate_target() then
        stopfollow()
        caster_range_check(3, 25)
        if (health_percentage("target") > 90 or UnitHealth("target") > 90) and
                not has_debuff("target", "Shadow Word: Pain") then
            CastSpellByName("Shadow Word: Pain")
        end
        if off_cd("Mind Blast") then
            CastSpellByName("Mind Blast")
        end
        CastSpellByName("Smite")
        --StartAttack();
    end
end
