function aoe_combat_druid()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe("Hurricane", get_unit_position("target"))
end
