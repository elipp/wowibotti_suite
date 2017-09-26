local function noobhunter_combat()
  if not UnitAffectingCombat("player") and not has_buff("player", "Aspect of the Viper") then
    L_CastSpellByName("Aspect of the Viper")
  end

  if not validate_target() then return end


  if not has_buff("player", "Aspect of the Hawk") then
    L_CastSpellByName("Aspect of the Hawk")
    return
  end

  L_PetPassiveMode()
  L_PetAttack()

  if GetSpellCooldown("Rake") == 0 then
    L_CastSpellByName("Rake")
  else
    L_CastSpellByName("Claw")
  end

  caster_range_check(35)

  if not has_debuff("target", "Hunter's Mark") then
    L_CastSpellByName("Hunter's Mark")
    return
  end

  if GetSpellCooldown("Multi-Shot") == 0 then
    L_CastSpellByName("Multi-Shot")
    return;
  end

  if not has_debuff("target", "Serpent Sting") then
    L_CastSpellByName("Serpent Sting")
    return;
  end

  -- if GetSpellCooldown("Arcane Shot") == 0 then
  --   L_CastSpellByName("Arcane Shot")
  --   return;
  -- end

  L_CastSpellByName("Steady Shot")

end

local function noobshaman_renew_weaponenchant()
  local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
  --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  if (not has_mh) then
      L_CastSpellByName("Windfury Weapon")
  end
  if (not has_oh) then
    L_CastSpellByName("Flametongue Weapon")
  end
end

local function noobshaman_totemsexist()
  local _,a = GetTotemInfo(1)
  local _,b = GetTotemInfo(2)
  local _,c = GetTotemInfo(3)
  local _,d = GetTotemInfo(4)

  if (a ~= "" or b ~= "" or c ~= "" or d ~= "") then
    return true
  else
    return nil
  end

end

local function noobshaman_managetotems()
  local totems_exist = noobshaman_totemsexist()
  local within_range = has_buff("player", "Windfury Totem")

  if totems_exist and not within_range then
    L_CastSpellByName("Totemic Recall")
    return 1
  end

  if not totems_exist then
    L_CastSpellByName("Call of the Elements")
    return 1
  end

  return nil

end

local function noobshaman_combat()

  if not has_buff("player", "Water Shield") then
    L_CastSpellByName("Water Shield")
    return
  end

  noobshaman_renew_weaponenchant()

  if not validate_target() then return end

  melee_attack_behind()
  L_StartAttack()

  if noobshaman_managetotems() then return end

  if GetSpellCooldown("Stormstrike") == 0 then
    L_CastSpellByName("Stormstrike")
    return
  end

  if GetSpellCooldown("Lava Lash") == 0 then
    L_CastSpellByName("Lava Lash")
    return
  end

  if not has_debuff("target", "Flame Shock") then
      if cast_if_nocd("Flame Shock") then return end
  else
      if cast_if_nocd("Earth Shock") then return end;
  end

end

local mh_apply = nil
local oh_apply = nil

local function reapply_poisons()

  if mh_apply then
    L_RunMacroText("/use 16")
    mh_apply = nil
    return
  elseif oh_apply then
    L_RunMacroText("/use 17")
    oh_apply = nil
    return
  end

  local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
  --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  if not has_mh then
    L_RunMacroText("/use Crippling Poison")
    mh_apply = GetTime()
  elseif not has_oh then
    L_RunMacroText("/use Deadly Poison IV")
    oh_apply = GetTime()
  end

end

local function noobrogue_combat()

  reapply_poisons()

  if not validate_target() then return end

  melee_attack_behind()

  if UnitCastingInfo("target") or UnitChannelInfo("target") then
    L_CastSpellByName("Kick")
  end

  if UnitHealth("target") < 2000 and GetComboPoints("player", "target") > 0 then
    if not has_buff("player", "Slice and Dice") then
      L_CastSpellByName("Slice and Dice")
      return
    else
      L_CastSpellByName("Eviscerate")
      return
    end
  end

  if (GetComboPoints("player", "target") < 5) then
      L_CastSpellByName("Sinister Strike")
      return
  else
    local hassnd, timeleft = has_buff("player", "Slice and Dice")
    if not hassnd or timeleft < 6 then
      L_CastSpellByName("Slice and Dice")
    else
      L_CastSpellByName("Eviscerate")
    end
  end

end

function combat_noob()


  local _, class = UnitClass("player")

  if string.find(class, "ROGUE") then
    return noobrogue_combat()
  elseif string.find(class, "SHAMAN") then
    return noobshaman_combat()
  elseif string.find(class, "HUNTER") then
    return noobhunter_combat()
  end

end
