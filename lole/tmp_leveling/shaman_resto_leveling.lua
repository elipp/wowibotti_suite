local function raid_heal()

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        L_TargetUnit("player");
        cast_heal("Lesser Healing Wave");
        return true;
    else
        local target, urgencies = get_raid_heal_target(true)
        local heal_targets = get_raid_heal_targets(urgencies, 4);

        if not target then return false end

        L_TargetUnit(target);
        local target_HPP = health_percentage("target")

        if target_HPP < 20 then
            cast_heal("Lesser Healing Wave");
            return true
        end

        if heal_targets[2] ~= nil and health_percentage(heal_targets[2]) < 80 then
            cast_heal("Chain Heal");
        else
            if target_HPP < 30 then
                cast_heal("Lesser Healing Wave");
            elseif target_HPP < 70 then
                cast_heal("Healing Wave");
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

    if UnitMana("player") < 1000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    L_TargetUnit(heal_targets[1]);

    local target_HPP = health_percentage("target")

    if target_HPP < 30 then
        cast_heal("Lesser Healing Wave");
        return

    elseif health_percentage("player") < 30 then
        L_TargetUnit("player");
        cast_heal("Lesser Healing Wave");
        return

    elseif target_HPP < 50 then
        cast_heal("Healing Wave");
        return

    elseif health_percentage("player") < 50 then
        L_TargetUnit("player");
        cast_heal("Healing Wave");
        return

    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

end
