function aoe_combat_mage()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe("Blizzard(Rank 3)", get_unit_position("target"))
end
