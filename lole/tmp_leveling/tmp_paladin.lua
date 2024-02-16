function tmp_paladin_combat()
    if validate_target() then
        melee_attack_behind(1.5);
        L_StartAttack();
        if not has_buff("player", "Seal of Command") then
            return L_CastSpellByName("Seal of Command")
        elseif not has_debuff("target", "Judgement of Wisdom") then
            if cast_if_nocd("Judgement of Wisdom") then return end
        elseif #{get_combat_mobs()} > 2 then
            if cast_if_nocd("Consecration") then return end
        end
    end
end
