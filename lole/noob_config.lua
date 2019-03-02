local function noobhunter_combat()
  if UnitMana("player") < 500 or
  ((not UnitAffectingCombat("player")) and (not has_buff("player", "Aspect of the Viper"))) then
    L_CastSpellByName("Aspect of the Viper")
    -- no return in this one, seems to work
  end

  if not validate_target() then return end

  if (not has_buff("player", "Aspect of the Hawk")) and UnitMana("player") > 1500 then
    L_CastSpellByName("Aspect of the Hawk")
    return
  end

  L_PetPassiveMode()
  L_PetAttack()

  if GetSpellCooldown("Kill Command") == 0 then
    L_CastSpellByName("Kill Command")
    return;
  end

  if GetSpellCooldown("Rake") == 0 then
    L_CastSpellByName("Rake")
  else
    L_CastSpellByName("Claw")
  end

  if GetSpellCooldown("Rabid") == 0 then
    L_CastSpellByName("Rabid")
    return;
  end

  caster_range_check(5,35)

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

  has, dur, count = has_buff("player", "Maelstrom Weapon")
  if has and count > 4 then
    L_CastSpellByName("Lightning Bolt")
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
local mh_applytime = 0
local oh_apply = nil
local oh_applytime = 0

local function reapply_poisons()

  if mh_apply then
    L_RunMacroText("/use 16")
    mh_applytime = GetTime()
    mh_apply = 0
    return
  elseif oh_apply then
    L_RunMacroText("/use 17")
    oh_applytime = GetTime()
    oh_apply = 0
    return
  end

  local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
  --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  if not has_mh and (GetTime() - mh_applytime) > 5 then
    L_RunMacroText("/use Instant Poison VII")
    mh_apply = 1
  elseif not has_oh and (GetTime() - oh_applytime) > 5 then
    L_RunMacroText("/use Deadly Poison VII")
    oh_apply = 1
  end

end

local function noobrogue_combat()

  reapply_poisons()

  if not validate_target() then return end

  if not UnitAffectingCombat("player") and GetSpellCooldown("Stealth") == 0 then
    L_CastSpellByName("Stealth")
    return
  end

  melee_attack_behind()

  if UnitCastingInfo("target") or UnitChannelInfo("target") then
    L_CastSpellByName("Kick")
  end

  if (not has_buff("player", "Hunger For Blood")) and GetSpellCooldown("Hunger For Blood") == 0 then
    L_CastSpellByName("Hunger For Blood")
    DEFAULT_CHAT_FRAME:AddMessage("HFB")
    return
  end

  if not has_buff("player", "Slice and Dice") then
    DEFAULT_CHAT_FRAME:AddMessage("SND1")
    if GetComboPoints("player", "target") < 1 then
      L_CastSpellByName("Mutilate")
      DEFAULT_CHAT_FRAME:AddMessage("MUT")
      return
    else
      L_CastSpellByName("Slice and Dice")
      DEFAULT_CHAT_FRAME:AddMessage("SND2")

      return
    end
  end

  if GetComboPoints("player", "target") < 4 then
    L_CastSpellByName("Mutilate")
    DEFAULT_CHAT_FRAME:AddMessage("MUT2")
    return
  else
    L_CastSpellByName("Envenom")
    DEFAULT_CHAT_FRAME:AddMessage("ENV")

    return
  end

end

function combat_noob()

  -- if has_debuff("player", "Intense Cold") then
  --   execute_script("JumpOrAscendStart()")
  -- end

  local _, class = UnitClass("player")

  if string.find(class, "ROGUE") then
    return noobrogue_combat()
  elseif string.find(class, "SHAMAN") then
    return noobshaman_combat()
  elseif string.find(class, "HUNTER") then
    return noobhunter_combat()
  end

end
