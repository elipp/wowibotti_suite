local framenum = 0
local reset_framenum = 10

function tmp_paladin_combat()
    framenum = framenum+1
    if GetZoneText() == 'Gnomeregan' and framenum % reset_framenum == 0 then
        framenum = 1
        for name, _ in pairs(get_guild_members()) do
            target_unit_with_GUID(UnitGUID(name))
            if has_debuff("target", "Irradiated") then
                return L_CastSpellByName("Purify")
            end
        end
    end

    if validate_target() then
        melee_attack_behind(1.5);
        L_StartAttack();
        if health_percentage("target") < 20 and cast_if_nocd("Hammer of Wrath") then
            return
        elseif not has_buff("player", "Seal of Command") then
            return L_CastSpellByName("Seal of Command")
        elseif cast_if_nocd("Judgement of Wisdom") then return
        elseif cast_if_nocd("Exorcism") then return
        elseif #{get_combat_mobs()} > 2 then
            if cast_if_nocd("Consecration") then return end
        end
    end
end
