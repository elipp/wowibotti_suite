function aoe_combat_warlock()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe("Rain of Fire(Rank 2)", get_unit_position("target"))
end
