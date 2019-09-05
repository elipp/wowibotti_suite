
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
        L_TargetUnit(es_target)
        cast_heal("Earth Shield");
        return true;
    end

    return false

end


local function get_total_deficit(hp_deficits)
    local total = 0;

    for name, deficit in pairs(hp_deficits) do
        total = total + deficit
    end

    return total;

end


local function NS_heal_on_tank()

end

local function raid_heal()

    if (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        L_TargetUnit("player");
        cast_heal("Riptide");
        cast_heal("Lesser Healing Wave");
        return true;
    else
        local target, bounce1, bounce2 = get_CH_target_trio(get_serialized_heals());
        --local target, urgencies = get_raid_heal_target(true)
        --local heal_targets = get_raid_heal_targets(urgencies, 4);

        if not target then return end

        L_TargetUnit(target);
        local target_HPP = health_percentage("target")

        if target_HPP < 20 then
            cast_heal("Riptide");
            cast_heal("Lesser Healing Wave");
            return true
        end

        --[[local bounce1_is_deficient = false
        local bounce1 = heal_targets[2]
        if bounce1 then
            bounce1 = heal_targets[2]
            bounce1_deficit = UnitHealthMax(bounce1) - UnitHealth(bounce1)
            if bounce1_deficit > 2000 then
                bounce1_is_deficient = true
            end
        end--]]

        --if bounce1_is_deficient then
        if bounce1 then
            CH_BOUNCE_1 = bounce1
            CH_BOUNCE_2 = bounce2
            cast_heal("Chain Heal");
        else
            if target_HPP < 30 then
                cast_heal("Riptide");
                cast_heal("Lesser Healing Wave");
            elseif target_HPP < 70 then
                cast_heal("Healing Wave");
            end
        end
    end

    return true;

end

function check_EL()
    local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
    --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

    if not has_mh then
        L_CastSpellByName("Earthliving Weapon")
    end

end

local function use_sliver_trinket()
  if UnitManaMax("player") - UnitMana("player") >= 1625 then
    local a, c = GetItemCooldown(50339)
    if a == 0 then
      L_UseInventoryItem(14) -- use the mana trinket :DDD
    end
  end
end

local function cleanse_shaman()
  local debuffs = get_raid_debuffs_by_type("Poison,Disease,Curse")
  return cast_dispel(debuffs, "Cleanse Spirit")
end

combat_shaman_resto = function()


    if player_casting() then return end

    check_EL()

    if not has_buff("player", "Water Shield") and not has_buff("player", "Earth Shield") then
        L_CastSpellByName("Water Shield");
        return;
    end

    use_sliver_trinket()

    local totems = get_active_multicast_totems()
    local summonspell = get_active_multicast_summonspell()

    if refresh_totems(totems, summonspell) then return; end
    if refresh_ES(get_assigned_hottargets(UnitName("player"))) then return end


    if UnitMana("player") < 5000 then
        if cast_if_nocd("Mana Tide Totem") then return end
    end

    --if cleanse_raid("Curse of the Plaguebringer") then return end
    local debuffed_player = get_player_with_debuff("Incinerate Flesh")
    if debuffed_player then
        L_TargetUnit(debuffed_player)
        cast_heal("Tidal Force")
        cast_heal("Lesser Healing Wave")
        return
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();
        return;
    end

    L_TargetUnit(heal_targets[1]);

    local target_HPP = health_percentage("target")

    if target_HPP < 30 then
        cast_heal("Riptide");
        cast_heal("Lesser Healing Wave");
        return

    elseif health_percentage("player") < 30 then
        L_TargetUnit("player");
        cast_heal("Riptide");
        cast_heal("Lesser Healing Wave");
        return

    elseif target_HPP < 70 then
        cast_heal("Healing Wave");
        return

    elseif health_percentage("player") < 50 then
        L_TargetUnit("player");
        cast_heal("Healing Wave");
        return

    elseif table.contains(heal_targets, "raid") then
        raid_heal();
    end

    if lole_get("dispelmode") == 1 then
      if cleanse_shaman() then return end
    end

end
