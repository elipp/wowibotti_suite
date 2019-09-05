local do_tranquility = false;

local function refresh_rejuvenation(hottargets)

    if not hottargets or not hottargets[1] then return false; end

    for i, targetname in ipairs(hottargets) do
        if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then
        elseif not has_buff(targetname, "Rejuvenation") then
            cast_heal("Rejuvenation", targetname);
            return true;
        end
    end

    return false

end

local function should_cast_tranquility(min_deficit, min_healable_chars)
    if min_deficit == nil then min_deficit = 8000; end
    if min_healable_chars == nil then min_healable_chars = 4; end

    if GetSpellCooldown("Tranquility") > 0 then
        return false;
    end

    local r = false;
    local HP_deficits = get_HP_deficits(true, false);

    local num_deficients = 0;
    for unit, deficit in pairs(HP_deficits) do
        local distance_to_unit = get_distance_between("player", UnitName(unit));
        if distance_to_unit and distance_to_unit <= 30 and deficit > min_deficit then
            num_deficients = num_deficients + 1;
            if num_deficients == min_healable_chars then
                r = true;
                break;
            end
        end
    end

    return r;
end

-- Wild Growth
local function get_WG_target(urgencies, min_deficit, max_ineligible_chars)

    local highest_total_urgency = 0;
    local wg_target;

    local eligible_groups = get_CoH_eligible_groups(get_raid_groups(), min_deficit, max_ineligible_chars);
    if table.get_num_elements(eligible_groups) == 1 then
        wg_target = next(eligible_groups);
    else
        for tar, group in pairs(eligible_groups) do
            local total_urgency = 0;
            for i, name in pairs(group) do
                if urgencies[name] then
                    total_urgency = total_urgency + urgencies[name];
                end
            end
            if total_urgency > highest_total_urgency then
                highest_total_urgency = total_urgency;
                wg_target = tar;
            end
        end
    end

    return wg_target;

end

local function raid_heal()

    if should_cast_tranquility() then
        L_CastSpellByName("Barkskin");
        do_tranquility = true;
        return true;
    elseif UnitHealth("player") < UnitHealthMax("player")*0.30 then
        L_CastSpellByName("Barkskin");
        L_TargetUnit("player");
        cast_heal("Swiftmend")
        cast_heal("Regrowth");
        return true;
    end

    local _, urgencies = get_raid_heal_target(true);

    if GetSpellCooldown("Wild Growth") == 0 then
        local wg_target = get_WG_target(urgencies, 8000, 1);
        if wg_target then
            L_TargetUnit(wg_target);
            cast_heal("Wild Growth");
            return true;
        end
    end

    local heal_targets = get_raid_heal_targets(urgencies, 4);
    local reju_checked = false;
    local wg_checked = false;
    for i, target in ipairs(heal_targets) do
        local target_HPP = health_percentage(target)

        local has_rg, timeleft_rg, stacks_rg = has_buff(target, "Regrowth");
        local has_rj, timeleft_rj, stacks_rj = has_buff(target, "Rejuvenation");
        local has_lb, timeleft_lb, stacks_lb = has_buff(target, "Lifebloom");

        if (target_HPP < 35) then
            cast_heal("Swiftmend", target);
            if not has_rg or timeleft_rg < 5 then
                cast_heal("Regrowth", target);
                return true;
            else
                cast_heal("Nourish", target);
                return true;
            end
        end

        if not wg_checked and GetSpellCooldown("Wild Growth") == 0 then
            local wg_target = get_WG_target(urgencies, 3000, 2);
            wg_checked = true;
            if wg_target then
                L_TargetUnit(wg_target);
                cast_heal("Wild Growth");
                return true;
            end
        end

        if not reju_checked then
            if refresh_rejuvenation(get_assigned_hottargets(UnitName("player"))) then return; end
            reju_checked = true;
        end

        if (target_HPP < 60) then
            if not has_rj then
                cast_heal("Rejuvenation", target);
                return true;
            elseif not has_lb or stacks_lb < 3 then
                cast_heal("Lifebloom", target);
                return true;
            end
        end

        if (target_HPP < 80) then
            if not has_lb or timeleft_lb < 1.8 then
                cast_heal("Lifebloom", target);
                return true;
            end
        end
    end

    return false;

end

local function cleanse_druid()

  local poisons = get_raid_debuffs_by_type("Poison")
  if cast_dispel(poisons, "Abolish Poison") then return true end

  local curses = get_raid_debuffs_by_type("Curse")
  if cast_dispel(curses, "Remove Curse") then return true end

  return nil

end

local function faeriefire()

      if not validate_target() then return end
      if not has_debuff("target", "Faerie Fire") then
        caster_range_check(0, 30)
        L_CastSpellByName("Faerie Fire")
        return true
      end

      return nil
end

combat_druid_resto = function()

  if player_casting() then return end

    if do_tranquility then
        if cast_if_nocd("Tranquility") then do_tranquility = false end
        return;
    end
    if casting_legit_heal() then return end

    if (not has_buff("player", "Tree of Life")) then
        L_CastSpellByName("Tree of Life")
        return
    end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Innervate") == 0 then
        L_TargetUnit("player");
		L_CastSpellByName("Innervate");
		return;
	end

    --if cleanse_raid("Curse of the Plaguebringer") then return end
    local debuffed_player = get_player_with_debuff("Incinerate Flesh")
    if debuffed_player then
        L_TargetUnit(debuffed_player)
        if not has_buff(debuffed_player, "Regrowth") then
            cast_heal("Regrowth")
        end
        cast_heal("Nourish")
        return
    end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal();

        faeriefire();

        return;
    end

    local heal_raid = false;
    if table.contains(heal_targets, "raid") then
        heal_raid = true;
        heal_targets = table.rid(heal_targets, "raid");
    end

    local target_HPP = health_percentage(heal_targets[1]);

    local has_rg, timeleft_rg, stacks_rg = has_buff(heal_targets[1], "Regrowth");
    local has_rj, timeleft_rj, stacks_rj = has_buff(heal_targets[1], "Rejuvenation");

    if target_HPP < 35 then
        cast_heal("Swiftmend", heal_targets[1]);
        if not has_rg or timeleft_rg < 5 then
            cast_heal("Regrowth", heal_targets[1]);
            return;
        else
            cast_heal("Nourish", heal_targets[1]);
            return;
        end
    end

    if health_percentage("player") < 30 then
        L_CastSpellByName("Barkskin");
        L_TargetUnit("player");
        cast_heal("Swiftmend")
        cast_heal("Regrowth");
        return;
    end

    local lb_candidate = nil;
    local unit_without_lb = nil;
    local least_stacks = 3;
    local unit_with_least_stacks = nil;
    for i, name in ipairs(heal_targets) do
        local has_lb, timeleft_lb, stacks_lb = has_buff(name, "Lifebloom");
    	if has_lb then
            if not lb_candidate then
                lb_candidate = {name=name, timeleft=timeleft_lb};
            elseif timeleft_lb < lb_candidate["timeleft"] then
                lb_candidate = {name=name, timeleft=timeleft_lb};
            end
            if stacks_lb < least_stacks then
                least_stacks = stacks_lb;
                unit_with_least_stacks = name;
            end
        elseif not unit_without_lb then
            unit_without_lb = name;
        end
    end
    if lb_candidate and lb_candidate["timeleft"] < 0.8 + #heal_targets * 0.2 then
        cast_heal("Lifebloom", lb_candidate["name"]);
        return;
    elseif unit_without_lb then
        cast_heal("Lifebloom", unit_without_lb);
        return;
    elseif unit_with_least_stacks then
        cast_heal("Lifebloom", unit_with_least_stacks);
        return;
    end

    if refresh_rejuvenation(get_assigned_hottargets(UnitName("player"))) then return; end

    if should_cast_tranquility() then
        L_CastSpellByName("Barkskin");
        do_tranquility = true;
        return;
    end

    if not has_rj then
        cast_heal("Rejuvenation", heal_targets[1]);
        return
    end

    if health_percentage("player") < 50 then
        L_TargetUnit("player");
        if (not has_buff("target", "Rejuvenation")) then
            cast_heal("Rejuvenation");
            return;
        end
    end

    if heal_raid then
        raid_heal();
    end

    if lole_get("dispelmode") == 1 then
      if cleanse_druid() then return end
    end

    faeriefire()



end
