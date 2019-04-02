function combat_warrior_fury()

  if not validate_target() then return end

  melee_attack_behind()

  L_RunMacroText("/cast [nostance:3] Berserker Stance");

  local ppos = vec3:create(get_unit_position("player"))
  local tpos = vec3:create(get_unit_position("target"))
  local dist = ppos:distance(tpos)

  if (dist > 8 and dist < 25) then
    L_CastSpellByName("Intercept")
  end

  if UnitCastingInfo("target") or UnitChannelInfo("target") then
    L_CastSpellByName("Pummel")
  end

  if GetSpellCooldown("Bloodrage") == 0 then
    L_CastSpellByName("Bloodrage")
  end


  if UnitHealthMax("target") > 250000 then
    local has, dur, stacks = has_debuff("target", "Sunder Armor")
    if (not has) or dur < 5 or stacks < 5 then
      L_CastSpellByName("Sunder Armor")
      return
    end
  end

-- This is somehow fucked in raid setup
  if not has_buff("player", "Battle Shout") then
    L_CastSpellByName("Battle Shout")
  end

  if has_buff("player", "Slam!") then
    L_CastSpellByName("Slam")
    return
  end

  if GetSpellCooldown("Bloodthirst") == 0 then
    L_CastSpellByName("Bloodthirst")
    return
  end

  if GetSpellCooldown("Whirlwind") == 0 then
    L_CastSpellByName("Whirlwind")
    return
  end

  if lole_get("aoemode") == 1 and get_aoe_feasibility(8) > 2.50 then
  --if lole_get("aoemode") == 1 then
    L_CastSpellByName("Cleave")
  else
    L_CastSpellByName("Heroic Strike")
  end

  if GetSpellCooldown("Heroic Throw") == 0 then
    L_CastSpellByName("Heroic Throw")
    return
  end

  if IsUsableSpell("Execute") then
    L_CastSpellByName("Execute")
    return
  end

end
