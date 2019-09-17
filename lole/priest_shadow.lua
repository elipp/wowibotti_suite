local function cleanse_priest()
  local magicks = get_raid_debuffs_by_type("Magic")
  if cast_dispel(magicks, "Dispel Magic") then return true end

  local diseases = get_raid_debuffs_by_type("Disease")
  if cast_dispel(diseases, "Abolish Disease") then return true end

  return nil

end

function combat_priest_shadow()

  if not UnitAffectingCombat("player") and UnitMana("player") < 10000 then
    L_CastSpellByName("Dispersion")
  end

  if not has_buff("player", "Shadowform") then
    L_CastSpellByName("Shadowform")
    return
  end

  if not has_buff("player", "Inner Fire") then
    L_CastSpellByName("Inner Fire")
    return
  end


  if lole_get("dispelmode") == 1 then
    if cleanse_priest() then return end
  end

  if not validate_target() then return end

  caster_range_check(1, 32)

  local c = UnitChannelInfo("player")

  if c == "Mind Flay" or c == "Mind Sear" then
    return -- in order not to clip mind flay/sear
  end

  L_CastSpellByName("Dispel Magic")

  if lole_get("dispelmode") == 1 then
    if cleanse_priest() then return end
  end

  if UnitMana("player") < 10000 then
    if GetSpellCooldown("Shadowfiend") == 0 and UnitHealth("target") > 70000 then
      L_CastSpellByName("Shadowfiend")
    elseif GetSpellCooldown("Dispersion") == 0 then
      L_CastSpellByName("Dispersion")
    end
  end

  if lole_get("aoemode") == 1 and get_aoe_feasibility(15) > 3 then
    L_CastSpellByName("Mind Sear")
    return
  end

  if UnitHealth("target") < 15000 then
    L_CastSpellByName("Shadow Word: Death")
  end

  if not has_debuff("target", "Vampiric Touch") then
    L_CastSpellByName("Vampiric Touch")
    return
  elseif UnitCastingInfo("player") == "Vampiric Touch" then
    L_SpellStopCasting()
  end


  if not has_debuff("target", "Devouring Plague") then
    L_CastSpellByName("Devouring Plague")
    return
  end

  if not has_debuff("target", "Shadow Word: Pain") then
    L_CastSpellByName("Shadow Word: Pain")
    return
  end

  if GetSpellCooldown("Mind Blast") == 0 then
    L_CastSpellByName("Mind Blast")
    return
  end

  L_CastSpellByName("Mind Flay")

end
