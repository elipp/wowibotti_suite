local heal_targets = {
    "Krahu", "Lifegrief", "Sunnmon", "Pubissa", "Fartrippah"
}

local cd_heals = {
    "Holy Light"
}

local function get_highest_prio_heal_target()
    for key, unitname in pairs(heal_targets) do
        local target_hp = health_percentage(unitname)
        local target_alive = not UnitIsDeadOrGhost(unitname)
        if unitname == "Krahu" and target_hp < 60 and target_alive then
            return unitname
        end
        if target_hp < 25 and target_alive then
            return unitname
        end
    end
    return nil
end

function tmp_paladin_combat()
    local target = get_highest_prio_heal_target()
    if target == nil and not UnitAffectingCombat("player") then
        if not has_buff("player", "Devotion Aura") then
            L_CastSpellByName("Devotion Aura")
        end
        if not has_buff("player", "Blessing of Wisdom") then
            TargetUnit("player")
            L_CastSpellByName("Blessing of Wisdom")
        end
        if not has_buff("Krahu", "Blessing of Might") then
            TargetUnit("Krahu")
            L_CastSpellByName("Blessing of Might")
        end
        follow_unit("Krahu")
        return
    end

    stopfollow()
    -- Now we have target, check if we should actually heal or wait
    if table.contains(cd_heals, LAST_SUCCESSFUL_SPELL.name) and
            GetTime() - LAST_SUCCESSFUL_SPELL.cast_time < 0.9 then
        -- Avoid casting overheals if we just healed
        L_SpellStopCasting()
        return
    elseif target == nil then
        L_SpellStopCasting()
        return
    else
        TargetUnit(target)
        L_CastSpellByName("Holy Light")
    end

    --if health_percentage("Krahu") < 50 then
    --    stopfollow()
    --    TargetUnit("Krahu")
    --    CastSpellByName("Holy Light")
    --end
end
