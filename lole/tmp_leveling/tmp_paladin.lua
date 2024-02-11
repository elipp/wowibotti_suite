function tmp_paladin_combat()
    if validate_target() then
        melee_attack_behind(1.5);
        L_StartAttack();
        if not has_buff("player", "Seal of Righteousness") then
            return L_CastSpellByName("Seal of Righteousness")
        elseif not has_debuff("target", "Judgement of Light") then
            if cast_if_nocd("Judgement of Light") then return end
        end
    end
end
