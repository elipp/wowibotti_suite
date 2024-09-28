local max_rank_hurricane = get_max_rank_AOE_spell("Hurricane")

function aoe_combat_druid()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe(max_rank_hurricane, get_unit_position("target"))
end
