combat_druid_feral = function()

    if not validate_target() then return end

	melee_attack_behind()

    if (not has_buff("player", "Omen of Clarity")) then
        CastSpellByName("Omen of Clarity")
        return
    end

    if (not has_buff("player", "Cat Form")) then
        CastSpellByName("Cat Form")
        return
    end

    if (not has_debuff("target", "Faerie Fire")) then
        CastSpellByName("Faerie Fire (Feral)(Rank 5)")
        return
    end

    if (not has_debuff("target", "Mangle")) then
        CastSpellByName("Mangle (Cat)(Rank 3)")
        return
    end

    if (GetComboPoints("player", "target") < 5) then
        CastSpellByName("Shred")
        return;
    end

    if (UnitMana("player") < 70) then
        return
    end

    CastSpellByName("Rip")



end
