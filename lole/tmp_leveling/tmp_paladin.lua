local framenum = 0
local reset_framenum = 10

function throw_insta_flash_of_lights()
    local target, urgencies = get_raid_heal_target(true);
    if target ~= nil and has_buff("player", "The Art of War") and health_percentage(target) < 50 then
        L_TargetUnit(target)
        L_CastSpellByName("Flash of Light")
        return true
    end
end

function tmp_paladin_combat()
    if throw_insta_flash_of_lights() then
        return
    end

    if validate_target() then
        melee_attack_behind(1.5);
        L_StartAttack();
        local aoe_feas = get_aoe_feasibility("player", 5)
        if health_percentage("target") < 20 and cast_if_nocd("Hammer of Wrath") then
            return
        elseif UnitMana("player") > 800 and aoe_feas > 3 and cast_if_nocd("Consecration") then
            return
        elseif not has_buff("player", "Seal of Command") then
            return L_CastSpellByName("Seal of Command")
        elseif aoe_feas > 1.8 and cast_if_nocd("Divine Storm") then
            return
        elseif cast_if_nocd("Crusader Strike") then
            return
        elseif cast_if_nocd("Judgement of Wisdom") then
            return
        elseif cast_if_nocd("Exorcism") then
            return
        end
    end
end
