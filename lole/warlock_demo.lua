local function tap_if_need_to()
    if UnitMana("player") < 2500 then
        if UnitHealth("player") > 3500 then
            L_CastSpellByName("Life Tap");
            return true;
        end
    else
        return false;
    end
end

local curse_assignments = {
    ["Robins"] = "Curse of the Elements",
    --["Mulck"] = "Curse of Doom",
    --["Mulck"] = "Curse of the Elements",

    --["Jobim"] = "Curse of the Elements",
}

local function enough_shards()
    if (GetItemCount(6265) < 32) then
        return false
    else
        return true
    end-- 6265 -- soul shard
end

local function cast_assigned_curse()

    if UnitHealth("target") < 30000 then return end

    for name,curse in pairs(curse_assignments) do
        if name == UnitName("player") and UnitExists("target") then
            if not has_debuff_by_self("target", curse) then
                if cast_if_nocd(curse) then return true end
            end
        end
    end

    return false
end

local function drain_soul_if_needed()
    if not enough_shards() then
        local HP_threshold = 10000 -- for 5-man groups
        if in_raid_group() then
            if GetNumRaidMembers() > 10 then
                HP_threshold = 40000
            else
                HP_threshold = 20000
            end
        end

        if (UnitHealth("target") < HP_threshold) then
            L_CastSpellByName("Drain Soul");
            return true;
        end
    end

    return false
end

local time_from_pet_summon = 0
local last_soc_target = nil

combat_warlock_demo = function()


    if player_casting() then return end
    echo("MOROI")

    local mana = UnitMana("player");
    local maxmana = UnitManaMax("player");

    if not UnitAffectingCombat("player") then
        if (mana < 0.90*maxmana) then
            L_CastSpellByName("Life Tap");
        end
        L_PetPassiveMode()
        L_PetFollow()

        if (GetItemCount(41196) == 0) then
          L_CastSpellByName("Create Spellstone");
          return;
        end

        if not GetWeaponEnchantInfo() then
          if GetItemCount(41196) > 0 then
            L_RunMacro("spellstone")
          end
        end

    end



    if not has_buff("player", "Master Demonologist") then
        L_CastSpellByName("Fel Domination")
        if GetTime() - time_from_pet_summon > 16 then
            L_CastSpellByName("Summon Felguard")
            time_from_pet_summon = GetTime()
        end
    else
        time_from_pet_summon = 0
    end

    if not validate_target() then return end

    if UnitAffectingCombat("player") then
      L_PetAttack()
    end

    caster_range_check(30);

    if tap_if_need_to() then return; end

    if lole_subcommands.get("aoemode") == 1 then
      	for i,g in pairs({get_combat_targets()}) do
      			target_unit_with_GUID(g)

      			if UnitIsEnemy("target", "player") and not has_debuff("target", "Seed of Corruption") then
                if not last_soc_target or (last_soc_target and last_soc_target ~= g) then
        					L_CastSpellByName("Seed of Corruption")
                  last_soc_target = g
        					return;
                end
      			end
      		end
    end

    last_soc_target = nil

    if drain_soul_if_needed() then return end
    if cast_assigned_curse() then return end

    if not has_debuff("target", "Shadow Mastery") then
        L_CastSpellByName("Shadow Bolt");
    elseif not has_debuff_by_self("target", "Immolate") then
        L_CastSpellByName("Immolate");
    elseif not has_debuff_by_self("target", "Corruption") then
        L_CastSpellByName("Corruption");
    elseif has_buff("player", "Decimation") then
        L_CastSpellByName("Soul Fire");
    elseif has_buff("player", "Molten Core") then
        L_CastSpellByName("Incinerate");
    else
        L_CastSpellByName("Shadow Bolt");
    end

end

function survive_warlock_demo()

end
