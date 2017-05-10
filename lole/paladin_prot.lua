
combat_paladin_prot = function()

    --if cleanse_party("Arcane Shock") then return; end

    --tank_face()

    if not has_buff("player", "Holy Shield") then
        if cast_if_nocd("Holy Shield") then return; end
    end

    if cast_if_nocd("Consecration") then return; end

    --if not has_debuff("target", "Judgement of the Crusader") then
    --    if not has_buff("player", "Seal of the Crusader") then
    --        L_CastSpellByName("Seal of the Crusader");
    --    end
    --    cast_if_nocd("Judgement");
    --    return;
    --end

    if not has_buff("player", "Seal of Righteousness") then
        L_CastSpellByName("Seal of Righteousness");
        return;
    end

    if cast_if_nocd("Judgement") then return; end

end
