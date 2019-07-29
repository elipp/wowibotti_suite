local petfollow_called = nil

local function petfollow_default()
  if lole_get("blast") == 0 then
    if not petfollow_called then
      L_PetFollow()
      L_PetPassiveMode()
      petfollow_called = true
    end
  else
    petfollow_called = nil
  end
end

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
        if in_raid() then
            if GetNumRaidMembers() > 10 then
                HP_threshold = 40000
            else
                HP_threshold = 20000
            end
        end

        if UnitLevel("target") > 71 and (UnitHealth("target") < HP_threshold) then
            L_CastSpellByName("Drain Soul");
            return true;
        end
    end

    return false
end

local time_from_pet_summon = 0
local prev_seed_target = NOTARGET
local immolate_lock = 0


local cast_barrier = 0
local soc_bans = {}

local function clear_expired_soc_bans()
  local T = GetTime()

  for G, t in pairs(soc_bans) do
    if (T - t) > 5 then
      soc_bans[G] = nil
    end
  end
end

local function add_soc_ban(GUID)
  soc_bans[GUID] = GetTime()
end

local function soc_banned(GUID)
  for G,t in pairs(soc_bans) do
    if G == GUID then return true end
  end
  return false
end

local petfollow_called = nil

combat_warlock_demo = function()
    if not petframe_dummy then
        petframe_dummy = CreateFrame("frame",nil, UIParent)
        petframe_dummy:SetScript("OnUpdate", petfollow_default)
    end
    local mana = UnitMana("player");
    local maxmana = UnitManaMax("player");

    if not UnitAffectingCombat("player") then
        if (mana < 0.90*maxmana) then
            L_CastSpellByName("Life Tap");
        end

        if (GetItemCount(41196) == 0) then
          L_CastSpellByName("Create Spellstone");
          return;
        end

        if not GetWeaponEnchantInfo() then
          if GetItemCount(41196) > 0 then
            L_RunMacroText("/use Grand Spellstone")
            L_RunMacroText("/use 16")
          end
        end

    end

    if not has_buff("player", "Fel Armor") then
      L_CastSpellByName("Fel Armor")
      return
    end

    if not UnitExists("pet") or not PetHasActionBar() then
      if GetSpellCooldown("Fel Domination") == 0 then
        L_CastSpellByName("Fel Domination")
      end
      L_CastSpellByName("Summon Felguard")
    else
      if UnitCastingInfo("player") == "Summon Felguard" then
        L_SpellStopCasting()
      end
    end

    if UnitAffectingCombat("player") then
      L_PetAttack()
    end

    --if not validate_target() then return end
    L_RunMacroText("/lole target 0xF1300027C8000007") -- FOR ONYXIA, REPLACE GUID OFC


    caster_range_check(0,30);

    if tap_if_need_to() then return; end

    -- CONSIDER HOOKING UNIT_SPELLCAST_SENT

    clear_expired_soc_bans()

-- explanation: cast_barrier is something that should prevent a laggy AURA_UPDATE from fucking us up
-- a second should be enough to
    if lole_get("aoemode") == 1 and get_aoe_feasibility(15) > 3 then
        if GetTime() - cast_barrier < 1 or UnitCastingInfo("player") == "Seed of Corruption" then
          return
        end
      	for i,g in pairs({get_combat_targets()}) do
      			target_unit_with_GUID(g)
      			if not soc_banned(g) and not has_debuff("target", "Seed of Corruption") then
      					L_CastSpellByName("Seed of Corruption")
                add_soc_ban(g)
                cast_barrier = GetTime()
        				return;
            end
      		end
    end

    last_seed_target = NOTARGET

    if not validate_target() then return end -- to target the actual blast target

    if drain_soul_if_needed() then return end
    if cast_assigned_curse() then return end

    if not has_debuff("target", "Shadow Mastery") then
        L_CastSpellByName("Shadow Bolt");
    elseif not has_debuff_by_self("target", "Immolate") and immolate_lock == 0 then
        L_CastSpellByName("Immolate");
        immolate_lock = 1
    elseif not has_debuff_by_self("target", "Corruption") then
        L_CastSpellByName("Corruption");
    elseif has_buff("player", "Decimation") then
        L_CastSpellByName("Soul Fire");
    elseif has_buff("player", "Molten Core") then
        L_CastSpellByName("Incinerate");
    else
        L_CastSpellByName("Shadow Bolt");
    end
    immolate_lock = 0

end

function survive_warlock_demo()

end
