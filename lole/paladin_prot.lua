local taunt_spells = {
    "Righteous Defense",
    "Hand of Reckoning",
}


function combat_paladin_prot()

    --if cleanse_party("Arcane Shock") then return; end

    tank_face()
    local taunt_target, spell = get_loose_tank_target(taunt_spells, UnitGUID("Kuratorn"))

    if taunt_target then
        target_unit_with_GUID(taunt_target)
        caster_range_check(1, 36)
        L_CastSpellByName(spell)
    end

    if true then return end

    if cleanse_raid("Shared Suffering") then return end

    if not has_buff("player", "Holy Shield") then
        if cast_if_nocd("Holy Shield") then return; end
    end

    if cast_if_nocd("Shield of Righteousness") then return; end

    if cast_if_nocd("Hammer of the Righteous") then return; end

    if get_aoe_feasibility("player", 15) > 3 then
      if cast_if_nocd("Consecration") then return; end
    end

    if cast_if_nocd("Judgement of Light") then return; end

    if not has_buff("player", "Divine Plea") then
        if cast_if_nocd("Divine Plea") then return; end
    end

    if not has_buff("player", "Sacred Shield") then
        if cast_if_nocd("Sacred Shield") then return; end
    end

end
