local max_rank_blizzard = get_max_rank_AOE_spell("Blizzard")

function aoe_combat_mage()
    if player_casting() then return end
    if not validate_target() then return; end
    return cast_gtaoe(max_rank_blizzard, get_unit_position("target"))
end
