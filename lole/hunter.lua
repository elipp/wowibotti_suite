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

local petframe_dummy = nil

local aspect_changetime = 0
local function change_aspect(aspectname)

  -- explanation: on Warmane, there's a weird quirk that aspect changes get somehow "queued"
  -- so in order to avoid getting trapped in a aspect change spam cycle for a while, we add a timeout

  if GetTime() - aspect_changetime > 5 then
    L_CastSpellByName(aspectname)
    aspect_changetime = GetTime()
    return true
  end
  return nil

end


function combat_hunter()

  if not petframe_dummy then
    petframe_dummy = CreateFrame("frame",nil, UIParent)
    petframe_dummy:SetScript("OnUpdate", petfollow_default)
  end

  if UnitExists("pet") and UnitIsDead("pet") then
    L_CastSpellByName("Revive Pet")
  elseif not UnitExists("pet") or not PetHasActionBar() then
    L_CastSpellByName("Call Pet")
    return
  end

  if not UnitAffectingCombat("player") then
    --L_PetPassiveMode()

    if not has_buff("player", "Aspect of the Viper") then
      change_aspect("Aspect of the Viper")
      -- must NOT have return in this
    end
  end

  --if not validate_target() then return end
  L_TargetUnit("Onyxia")

  caster_range_check(9,35)

  if UnitMana("player") > 4000 and (not has_buff("player", "Aspect of the Dragonhawk")) then
    if change_aspect("Aspect of the Dragonhawk") then return end
  elseif UnitMana("player") < 500 and (not has_buff("player", "Aspect of the Viper")) then
    if change_aspect("Aspect of the Viper") then return end
    return
  end

  L_StartAttack()
  L_PetAttack()

  if GetSpellCooldown("Kill Command") == 0 then
    L_CastSpellByName("Kill Command")
    return;
  end

  local hppercentage = UnitHealth('target')/UnitHealthMax('target')
  if hppercentage < 0.20 then
    L_CastSpellByName("Kill Shot")
    -- no return, will not be cast if incooldown
  end

  if not has_debuff("target", "Hunter's Mark") then
    L_CastSpellByName("Hunter's Mark")
  end

  if GetSpellCooldown("Rabid") == 0 then
    L_CastSpellByName("Rabid")
    return;
  end

  if GetSpellCooldown("Rake(Rank 6)") == 0 then
    L_CastSpellByName("Rake(Rank 6)")
  else
    L_CastSpellByName("Claw(Rank 11)")
  end

  if not has_debuff("target", "Serpent Sting") then
    L_CastSpellByName("Serpent Sting")
    return;
  end

  if GetSpellCooldown("Arcane Shot") == 0 then
    L_CastSpellByName("Arcane Shot")
    return
  end

  if GetSpellCooldown("Multi-Shot") == 0 then
    L_CastSpellByName("Multi-Shot")
    return;
  end

  L_CastSpellByName("Steady Shot")

end
