local pom_time = 0;

local function refresh_healbuffs(hottargets)

    if not hottargets or not hottargets[1] then return false; end

    local pom_checked = false;
    for i, targetname in ipairs(hottargets) do
        if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then
        else
            if not pom_checked then
                pom_checked = true; -- first healable hot target always gets PoM
                if not has_buff(targetname, "Prayer of Mending") and time() - pom_time > 10 then
                    if cast_heal("Prayer of Mending", targetname) then
                        pom_time = time();
                    end
                    return true;
                end
            end
            local found, timeleft = has_buff(targetname, "Renew");
            if not found or not timeleft then
                cast_heal("Renew", targetname);
                return true;
            end
        end
    end

    return false

end

local function should_cast_PoH(min_deficit, min_healable_chars)
    if min_deficit == nil then min_deficit = 3000; end
    if min_healable_chars == nil then min_healable_chars = 4; end

    local is_channeling = false;
    -- TargetUnit("Hex Lord Malacrass");
    -- if UnitChannelInfo("target") == "Spirit Bolts" then
    --     is_channeling = true;
    --     min_deficit = 3000;
    -- end

	local HP_deficits = get_HP_deficits(true, true);

    local eligible_targets = {};
	for unit, deficit in pairs(HP_deficits) do
        local distance_to_unit = get_distance_between("player", UnitName(unit));
        if distance_to_unit and distance_to_unit <= 36 and deficit > min_deficit then
            table.insert(eligible_targets, unit);
		end
	end

    if #eligible_targets >= min_healable_chars then
        if is_channeling and UnitCastingInfo("player") and not UnitCastingInfo("player") == "Prayer of Healing" then
            SpellStopCasting();
        end
        POH_TARGETS = shallowcopy(eligible_targets);
        return true;
    end

	return false;
end

local function get_CoH_target(urgencies, min_deficit, max_ineligible_chars)
    if get_current_config().name == "priest_holy_ds" then
        return nil;
    end

    local highest_total_urgency = 0;
    local coh_target;

    local eligible_groups = get_CoH_eligible_groups(get_raid_groups(), min_deficit, max_ineligible_chars);
    if table.get_num_elements(eligible_groups) == 1 then
        coh_target = next(eligible_groups);
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
                coh_target = tar;
            end
        end
    end

    return coh_target;

end

local function cast_PoM_here(has_single_targets, from_raid_heal)

    local hottargets = get_assigned_hottargets(UnitName("player"));
    for i, targetname in ipairs(hottargets) do
        if not UnitExists(targetname) or not UnitIsConnected(targetname) or UnitIsDead(targetname) or has_buff(targetname, "Spirit of Redemption") or UNREACHABLE_TARGETS[targetname] > GetTime() then
        else
            return false;
        end
    end

    if has_single_targets and from_raid_heal then
        return false;
    end

    return true;

end


local function raid_heal(has_single_targets)
    local target, urgencies = get_raid_heal_target(true);
    if target then
        TargetUnit(target);
    else
        TargetUnit("player");
    end

    local target_HPP = health_percentage("target");

    local targeting_self = UnitName("target") == UnitName("player");

    if UnitHealth("Adieux") < 5000 then
        cast_heal("Flash Heal", "Adieux");
    end

    if not has_buff("player", "Prayer of Mending") and time() - pom_time > 10 then
        if cast_heal("Prayer of Mending", "player") then
            pom_time = time();
        end
    end

    local coh_target = get_CoH_target(urgencies, 1000, 2);
    if coh_target then
        TargetUnit(coh_target);
        cast_heal("Circle of Healing");
        return true
    end

    if should_cast_PoH(6000, 4) then
        cast_heal("Prayer of Healing");
        return true
    end

    if health_percentage("player") < 30 then
        TargetUnit("player");
        cast_heal("Power Word: Shield");
        cast_heal("Flash Heal");
        return true
    end

    if target_HPP < 20 then
        cast_heal("Power Word: Shield");
        cast_heal("Flash Heal");
        return true
    end

    if should_cast_PoH(3000, 4) then
        cast_heal("Prayer of Healing");
        return true
    end

    local coh_target = get_CoH_target(urgencies);
    if coh_target then
        TargetUnit(coh_target);
        cast_heal("Circle of Healing");
    elseif refresh_healbuffs(get_assigned_hottargets(UnitName("player"))) then
    elseif target_HPP < 50 then
        cast_heal("Greater Heal");
    elseif target_HPP < 75 then
        if not targeting_self and health_percentage("player") < 75 then
            cast_heal("Binding Heal");
        else
            cast_heal("Greater Heal(Rank 1)");
        end
    else
        TargetUnit("Hex Lord Malacrass");
        CastSpellByName("Shoot");
    end
    -- elseif target_HPP < 85 then
    --     if cast_PoM_here(has_single_targets, true) and not has_buff("target", "Prayer of Mending") and time() - pom_time > 10 then
    --         if cast_heal("Prayer of Mending") then
    --             pom_time = time();
    --         end
    --     else
    --         local found, timeleft = has_buff("target", "Renew");
    --         if not found or not timeleft then
    --             cast_heal("Renew");
    --         end
    --     end
    -- else
    --     return false;
    -- end

    return true;
end

combat_priest_holy = function()

	if casting_legit_heal() then return end

	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Shadowfiend") == 0 and validate_target() then
        TargetUnit("focus");
		CastSpellByName("Shadowfiend");
		return;
	end

    local heal_targets = sorted_by_urgency(get_assigned_targets(UnitName("player")));
    if heal_targets[1] == nil or heal_targets[1] == "raid" then
        raid_heal(false);
        return;
    end

    TargetUnit(heal_targets[1]);


  local target_HPP = health_percentage("target")

	local targeting_self = UnitName("target") == UnitName("player");
    local found, timeleft = has_buff("target", "Renew");

    if target_HPP < 15 then
        cast_heal("Power Word: Shield");
        cast_heal("Flash Heal");

  elseif target_HPP < 30 then
		cast_heal("Greater Heal");

  elseif health_percentage("player") < 30 then
        TargetUnit("player");
        cast_heal("Power Word: Shield");
        cast_heal("Flash Heal");

    elseif target_HPP < 50 then
        cast_heal("Greater Heal");

    elseif refresh_healbuffs(get_assigned_hottargets(UnitName("player"))) then

    elseif cast_PoM_here(true, false) and not has_buff("target", "Prayer of Mending") and time() - pom_time > 10 then
        if cast_heal("Prayer of Mending") then
            pom_time = time();
        end
    elseif (should_cast_PoH()) then
        cast_heal("Prayer of Healing");

	elseif target_HPP < 60 then
		if not targeting_self and health_percentage("player") < 50 then
			cast_heal("Binding Heal");
		else
			cast_heal("Greater Heal");
		end

  elseif target_HPP < 80 then
		if not targeting_self and health_percentage("player") < 70 then
			cast_heal("Binding Heal");
		else
			cast_heal("Greater Heal(Rank 1)");
		end
	elseif not found or not timeleft then
		cast_heal("Renew");
    elseif table.contains(heal_targets, "raid") then
        raid_heal(true);
    end

end
