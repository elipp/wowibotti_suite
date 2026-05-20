function combat_priest_discipline()
    if casting_legit_heal() then return end

    -- Shadowfiend on low mana
    if UnitAffectingCombat("player") and total_combat_mob_health() > 30000 and mana_percentage("player") < 30 and GetSpellCooldown("Shadowfiend") == 0 then
        if validate_target() then
            L_CastSpellByName("Shadowfiend")
            L_PetAttack()
            return
        end
    end

    if GetSpellCooldown("Shadowfiend") > (5*60 - 15) then
        L_RunMacroText("/petaggressive")
        if validate_target() then
            L_PetAttack()
        end
    end

    -- Dispel first
    if group_dispel() then return end

    -- Self-heal emergency
    if health_percentage("player") < 30 and cast_if_nocd("Desperate Prayer") then return end

    -- Get heal target
    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")))
    if heal_targets[1] == "raid" then
        local target, urgencies = get_raid_heal_target(true)
        if not target then return end
        L_TargetUnit(target)
    end

    local target_HPP = health_percentage("target")
    local has_shield = has_buff("target", "Weakened Soul")

    -- ================================================
    -- EMERGENCY
    -- ================================================

    -- Binding Heal if we ourselves are also taking damage
    if UnitGUID("target") ~= UnitGUID("player") and health_percentage("player") < 75 then
        cast_heal("Binding Heal")
        return
    end

    -- Penance: very cheap and effective, use at low HP or to apply Inspiration on tank
    if GetSpellCooldown("Penance") == 0 then
        if target_HPP < 60 then
            cast_heal("Penance")
            return
        end
    end

    -- Shield critically low targets immediately
    if target_HPP < 30 and not has_shield then
        cast_heal("Power Word: Shield")
        return
    end

    if target_HPP < 30 then
        cast_heal("Flash Heal")
        return
    end

    -- ================================================
    -- CORE PRIORITY
    -- ================================================

    -- 1. Prayer of Mending on cooldown (highest HPS potential per cast)
    if GetSpellCooldown("Prayer of Mending") == 0 then
        cast_heal("Prayer of Mending")
        return
    end

    -- 2. Penance on cooldown into tank for Inspiration uptime even if not low
    if GetSpellCooldown("Penance") == 0 then
        cast_heal("Penance")
        return
    end

    -- 3. Power Word: Shield - majority of casts, shield raid members likely to take damage
    -- Avoid tanks unless Rapture just came off ICD (tracked via Weakened Soul absence)
    local is_tank = UnitGroupRolesAssigned and UnitGroupRolesAssigned("target") == "TANK"
    local rapture_ready = GetSpellCooldown("Rapture") == 0
    if not has_shield then
        if not is_tank or rapture_ready then
            cast_heal("Power Word: Shield")
            return
        end
    end

    -- 4. Prayer of Healing for grouped raid damage (10-man friendly)
    if should_cast_PoH(3000, 3) then
        cast_heal("Prayer of Healing")
        return
    end

    -- 5. Flash Heal for moderate damage
    if target_HPP < 70 then
        cast_heal("Flash Heal")
        return
    end

end
