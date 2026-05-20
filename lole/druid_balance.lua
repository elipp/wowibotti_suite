function combat_druid_balance()
    if not validate_target() then return end

    caster_range_check(11, 35)

    local in_lunar, lunar_dur = has_buff("player", "Eclipse (Lunar)")
    local in_solar, solar_dur = has_buff("player", "Eclipse (Solar)")

    -- Faerie Fire
    if not has_debuff("target", "Faerie Fire") then
        L_CastSpellByName("Faerie Fire")
        return
    end

    -- Force of Nature on cooldown
    if GetSpellCooldown("Force of Nature") == 0 then
        local tx, ty, tz = get_unit_position("target")
        L_CastSpellByName("Force of Nature")
        L_Click(tx, ty, tz)
        return
    end

    -- Starfall on cooldown, hold for AOE if needed
    if GetSpellCooldown("Starfall") == 0 and lole_get("aoemode") ~= 1 then
        L_CastSpellByName("Starfall")
        return
    end

    -- Insect Swarm: allow to fall off, skip reapplication if lunar is ending soon
    local is_has, is_dur = has_debuff("target", "Insect Swarm")
    if not is_has and not (in_lunar and lunar_dur < 5) then
        L_CastSpellByName("Insect Swarm")
        return
    end

    -- Lunar Eclipse: Starfire
    if in_lunar then
        L_CastSpellByName("Starfire")
        return
    end

    -- Solar Eclipse: Wrath
    if in_solar then
        L_CastSpellByName("Wrath")
        return
    end

    -- Between eclipses: Wrath to proc Lunar, Starfire once Lunar has been seen
    -- Poll both eclipse buffs — if neither is up, cast based on which was last active.
    -- Starfire being on a recent CD suggests we were in Solar, seek Lunar with Wrath.
    -- Wrath being on a recent CD suggests we were in Lunar, seek Solar with Starfire.
    local wrath_cd = GetSpellCooldown("Wrath")
    local starfire_cd = GetSpellCooldown("Starfire")

    if starfire_cd > wrath_cd then
        L_CastSpellByName("Starfire")
    else
        L_CastSpellByName("Wrath")
    end

end
