function tmp_mage_combat()
    if not UnitAffectingCombat("player") then
        if not has_buff("Lifegrief", "Arcane Intellect") then
            L_TargetUnit("Lifegrief")
            L_CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("Sunnmon", "Arcane Intellect") then
            L_TargetUnit("Sunnmon")
            L_CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("Pubissa", "Arcane Intellect") then
            L_TargetUnit("Pubissa")
            CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("player", "Arcane Intellect") then
            L_TargetUnit("player")
            L_CastSpellByName("Arcane Intellect")
        end
    end

    if validate_target() then
        stopfollow()
        caster_range_check(1, 29)
        L_CastSpellByName("Frostbolt")
    end
end
