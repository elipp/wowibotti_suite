function tmp_warrior_combat()
    if validate_target() then
        melee_attack_behind(1.5);
        L_StartAttack();
        if IsUsableSpell("Charge") and cast_if_nocd("Charge") then return end

        local aoe_feas = get_aoe_feasibility("player", 5)
        local overpower, _ = IsUsableSpell("Overpower")
        cast_if_nocd("Bloodrage")
        if overpower then
            L_CastSpellByName("Overpower")
        elseif not has_buff("player", "Battle Shout") then
            L_CastSpellByName("Battle Shout")
        elseif not has_debuff("target", "Rend") then
            L_CastSpellByName("Rend")
        elseif UnitMana("player") > 50 then
            L_CastSpellByName("Heroic Strike")
        elseif aoe_feas > 1.5 then
            L_CastSpellByName("Thunder Clap")
        elseif IsUsableSpell("Execute") then
            L_CastSpellByName("Execute")
        end
    end
end
