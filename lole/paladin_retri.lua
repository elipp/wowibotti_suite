function combat_paladin_retribution()
    if not validate_target() then return end
    melee_attack_behind(5)

    local ppos = vec3:create(get_unit_position("player"))
    local tpos = vec3:create(get_unit_position("target"))
    local dist = ppos:distance(tpos)

    if lole_get("aoemode") == 1 and get_aoe_feasibility("target", 8) > 2.50 then
        -- Multi-target: use Seal of Command
        if not has_buff("player", "Seal of Command") then
            L_CastSpellByName("Seal of Command")
            return
        end
    else
        -- Single-target: use Seal of Vengeance (Horde: Seal of Corruption)
        if not has_buff("player", "Seal of Vengeance") then
            L_CastSpellByName("Seal of Vengeance")
            return
        end
    end

    -- ================================================
    -- SINGLE TARGET PRIORITY
    -- ================================================

    -- 1. Crusader Strike
    if GetSpellCooldown("Crusader Strike") == 0 then
        L_CastSpellByName("Crusader Strike")
        return
    end

    -- 2. Judgement of Wisdom
    if GetSpellCooldown("Judgement of Wisdom") == 0 then
        L_CastSpellByName("Judgement of Wisdom")
        return
    end

    -- 3. Divine Storm
    if GetSpellCooldown("Divine Storm") == 0 then
        L_CastSpellByName("Divine Storm")
        return
    end

    -- 4. Consecration
    if GetSpellCooldown("Consecration") == 0 then
        L_CastSpellByName("Consecration")
        return
    end

    -- 5. Exorcism (Art of War makes it instant, always prefer that proc)
    if GetSpellCooldown("Exorcism") == 0 then
        if has_buff("player", "The Art of War") then
            L_CastSpellByName("Exorcism")
            return
        end
    end

    -- 6. Holy Wrath (only if target is undead or demon)
    if GetSpellCooldown("Holy Wrath") == 0 then
        if UnitCreatureType("target") == "Undead" or UnitCreatureType("target") == "Demon" then
            L_CastSpellByName("Holy Wrath")
            return
        end
    end

end
