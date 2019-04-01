function combat_paladin_prot()

    --if cleanse_party("Arcane Shock") then return; end

    --tank_face()

    if not has_buff("player", "Holy Shield") then
        if cast_if_nocd("Holy Shield") then return; end
    end

    if cast_if_nocd("Shield of Righteousness") then return; end

    if cast_if_nocd("Hammer of the Righteous") then return; end

    if get_aoe_feasibility(15) > 3 then
      if cast_if_nocd("Consecration") then return; end
    end

    if cast_if_nocd("Judgement of Light") then return; end

    -- if cast_if_nocd("Consecration") then return; end

    if not has_buff("player", "Divine Plea") then
        if cast_if_nocd("Divine Plea") then return; end
    end

    if not has_buff("player", "Sacred Shield") then
        if cast_if_nocd("Sacred Shield") then return; end
    end

end
