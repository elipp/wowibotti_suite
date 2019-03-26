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


local function noobhunter_combat()

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

  if not validate_target() then return end

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

  if not has_buff("player", "Lightning Shield") then
    L_CastSpellByName("Lightning Shield")
    return
  end


  noobshaman_renew_weaponenchant()

  if not validate_target() then return end

  melee_attack_behind()
  L_StartAttack()

  if UnitMana("player") < 1000 then
    L_CastSpellByName("Shamanistic Rage")
  end

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
local mh_at = 0
local oh_apply = nil
local oh_at = 0

local function reapply_poisons()

  if mh_apply then
    L_RunMacroText("/use 16")
    mh_at = GetTime()
    mh_apply = nil
  elseif oh_apply then
    L_RunMacroText("/use 17")
    oh_at = GetTime()
    oh_apply = nil
  end

  local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
--  echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  if not has_mh then
    L_RunMacroText("/use Instant Poison IX")
    if GetTime() - mh_at > 5 then
      mh_apply = 1
    end
  elseif not has_oh then
    L_RunMacroText("/use Deadly Poison IX")
    if GetTime() - oh_at > 5 then
      oh_apply = 1
    end
  end

end

local function noobrogue_combat()

  reapply_poisons()

  if not validate_target() then return end

  if not UnitAffectingCombat("player") then
    L_CastSpellByName("Stealth")
    L_CastSpellByName("Sprint")
  end

  melee_attack_behind()

  if health_percentage("player") < 15 then
    L_CastSpellByName("Cloak of Shadows")
  end

  if UnitCastingInfo("target") or UnitChannelInfo("target") then
    L_CastSpellByName("Kick")
  end

  if not has_buff("player", "Hunger For Blood") then
    L_CastSpellByName("Hunger For Blood")
    -- don't return, this will fail if no bleed active
  end

  if GetSpellCooldown("Vanish") == 0 and not has_buff('player', "Overkill") then
    L_CastSpellByName("Vanish")
  end

  if not has_buff("player", "Slice and Dice") then
    if GetComboPoints("player", "target") > 1 then
      L_CastSpellByName("Slice and Dice")
      return
    end
  end

  if GetComboPoints("player", "target") < 4 then
    if UnitHealth("target") < 15000 then
      L_CastSpellByName("Envenom")
    else
      L_CastSpellByName("Mutilate")
    end
  else
    if GetComboPoints("player", "target") == 5 then
      L_CastSpellByName("Cold Blood")
    end
    L_CastSpellByName("Envenom")
  end




end

function noobwarrior_combat()

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

local function noobpriest_combat()

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

  if not validate_target() then return end

  caster_range_check(1, 32)

  local c = UnitChannelInfo("player")

  if c == "Mind Flay" or c == "Mind Sear" then
    return -- in order not to clip mind flay/sear
  end

  L_CastSpellByName("Dispel Magic")

  if GetSpellCooldown("Shadowfiend") > 0 and UnitMana("player") < 2000 then
    L_CastSpellByName("Dispersion")
  elseif UnitHealth("target") > 30000 and UnitMana("player") < 1500 then
    L_CastSpellByName("Shadowfiend")
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
  elseif string.find(class, "WARRIOR") then
    return noobwarrior_combat()
  elseif string.find(class, "PRIEST") then
    return noobpriest_combat()
  end

end
