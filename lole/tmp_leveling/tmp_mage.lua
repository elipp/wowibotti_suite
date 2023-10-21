function tmp_mage_combat()
    if not UnitAffectingCombat("player") then
        if not has_buff("Lifegrief", "Arcane Intellect") then
            TargetUnit("Lifegrief")
            CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("Sunnmon", "Arcane Intellect") then
            TargetUnit("Sunnmon")
            CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("Pubissa", "Arcane Intellect") then
            TargetUnit("Pubissa")
            CastSpellByName("Arcane Intellect")
            return
        end
        if not has_buff("player", "Arcane Intellect") then
            TargetUnit("player")
            CastSpellByName("Arcane Intellect")
        end
    end

    if validate_target() then
        stopfollow()
        caster_range_check(1, 29)
        CastSpellByName("Frostbolt")
    end
end