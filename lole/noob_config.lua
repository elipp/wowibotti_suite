local function noobhunter_combat()

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

  if not has_debuff("target", "Serpent Sting") then
    L_CastSpellByName("Serpent Sting")
    return;
  end

  if GetSpellCooldown("Arcane Shot") == 0 then
    L_CastSpellByName("Arcane Shot")
    return;
  end


end

local TOTEMS = {
["earth"] = "Stoneskin Totem",
--["fire"] = "Searing Totem"
}

local function noobshaman_renew_weaponenchant()
  local has_mh, mh_exp, mh_charges = GetWeaponEnchantInfo()
  --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  if (not has_mh) then
      L_CastSpellByName("Flametongue Weapon")
  end
end

local function noobshaman_combat()

  melee_attack_behind()

  L_StartAttack()

  if not has_buff("player", "Water Shield") then
    L_CastSpellByName("Water Shield")
    return
  end

  noobshaman_renew_weaponenchant()
  if refresh_totems(TOTEMS) then return; end

  local _, searing = GetTotemInfo(1)
  if string.find("0", tostring(searing)) then
    L_CastSpellByName("Searing Totem")
  end

  if not validate_target() then return end

  if not has_debuff("target", "Flame Shock") then
      if cast_if_nocd("Flame Shock") then return end
  else
      if cast_if_nocd("Earth Shock") then return end;
  end

end

local function noobrogue_combat()
  melee_attack_behind()

  if (GetComboPoints("player", "target") < 5) then
      L_CastSpellByName("Backstab")
      return
  else
    L_CastSpellByName("Eviscerate")
  end

end

function combat_noob()

  if not validate_target() then return end

  local _, class = UnitClass("player")

  if string.find(class, "ROGUE") then
    return noobrogue_combat()
  elseif string.find(class, "SHAMAN") then
    return noobshaman_combat()
  elseif string.find(class, "HUNTER") then
    return noobhunter_combat()
  end

end
