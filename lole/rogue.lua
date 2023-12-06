local mh_apply = nil
local mh_at = 0
local oh_apply = nil
local oh_at = 0

local rotation = Rotation(
  {
    Spell("Backstab"),
  }
)

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
 -- echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

  -- if not has_mh then
  if false then -- windfury totem brah
    L_RunMacroText("/use Instant Poison V")
    if GetTime() - mh_at > 5 then
      mh_apply = 1
    end
  elseif not has_oh then
    L_RunMacroText("/use Instant Poison V")
    if GetTime() - oh_at > 5 then
      oh_apply = 1
    end
  end

end


function rogue_combat_before()

  reapply_poisons()

  if not validate_target() then return end

  if not UnitAffectingCombat("player") then
    L_CastSpellByName("Stealth")
    L_CastSpellByName("Sprint")
  end

  melee_attack_behind(1.5) -- 8 FOR THE RUBY SANCTUM GUY, 1.5 for magic

  if health_percentage("player") < 15 then
    L_CastSpellByName("Cloak of Shadows")
  end

  if unit_castorchannel("target") then
    L_CastSpellByName("Kick")
  end

  if not has_buff("player", "Hunger For Blood") then
    L_CastSpellByName("Hunger For Blood")
    -- don't return, this will fail if no bleed active
  end

  if GetSpellCooldown("Vanish") == 0 and not has_buff("player", "Overkill") then
    L_CastSpellByName("Vanish")
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


function rogue_combat()

  reapply_poisons()

  if validate_target() then
    melee_attack_behind(1.5);
    L_StartAttack();
    if #{get_combat_mobs()} > 2 and cast_if_nocd("Blade Flurry") then return end
    if unit_castorchannel("target") then
      L_CastSpellByName("Kick")
    end
    if not has_buff("player", "Slice and Dice") then
      if GetComboPoints("player", "target") >= 2 then
        L_CastSpellByName("Slice and Dice")
        return
      end
    else
      if GetComboPoints("player", "target") >= 4 then
        L_CastSpellByName("Eviscerate")
        return
      end
    end
    L_CastSpellByName("Sinister Strike")
  end
end
