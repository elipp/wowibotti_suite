local max_rank_rain_of_fire = get_max_rank_AOE_spell("Rain of Fire")

function aoe_combat_warlock()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe(max_rank_rain_of_fire, get_unit_position("target"))
end
