local function refresh_ES(hottargets)
    if not hottargets or not hottargets[1] then return false; end

    local es_target = nil
    for i, targetname in ipairs(hottargets) do
        if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then
            -- pass
        else
            es_target = targetname
            break
        end
    end

    if not es_target then return false end

    if not has_buff(es_target, "Earth Shield") then
        cast_heal("Earth Shield", es_target);
        return true;
    end

    return false
end

local function raid_heal()

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        cast_heal("Lesser Healing Wave", "player");
        return true;
    else
        local target, urgencies = get_raid_heal_target(true)
        
        if not target then return false end

        local heal_targets = get_raid_heal_targets(urgencies, 4);

        local target_HPP = health_percentage(target)

        if target_HPP < 20 then
            cast_heal("Lesser Healing Wave", target);
            return true
        end

        if heal_targets[2] ~= nil and health_percentage(heal_targets[2]) < 80 then
            cast_heal("Chain Heal", target);
        else
            if target_HPP < 30 then
                cast_heal("Lesser Healing Wave", target);
            elseif target_HPP < 70 then
                cast_heal("Healing Wave", target);
            end
        end
    end

    return true;

end

local function check_EL()
    local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
    --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

    if not has_mh then
        L_CastSpellByName("Earthliving Weapon")
    end

end

combat_shaman_resto_leveling = function()

    local spell_being_cast = player_casting();

    if spell_being_cast and spell_being_cast ~= "Lightning Bolt" then
        if health_percentage("target") > 90 then
            L_SpellStopCasting();
        end
        return;
    end

    check_EL()

    if not has_buff("player", "Water Shield") and not has_buff("player", "Earth Shield") then
        L_CastSpellByName("Water Shield");
        return;
    end

    --local totems = get_active_multicast_totems()
    --local summonspell = get_active_multicast_summonspell()
    --if refresh_totems(totems, summonspell) then return; end

    if refresh_ES(get_assigned_hottargets(UnitName("player"))) then return end

    if UnitMana("player") < 1000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    local target = heal_targets[1]

    local target_HPP = health_percentage(target)

    if target_HPP < 30 then
        cast_heal("Lesser Healing Wave", target);
        return

    elseif health_percentage("player") < 30 then
        cast_heal("Lesser Healing Wave", "player");
        return

    elseif target_HPP < 50 then
        cast_heal("Healing Wave", target);
        return

    elseif health_percentage("player") < 50 then
        cast_heal("Healing Wave", "player");
        return

    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
