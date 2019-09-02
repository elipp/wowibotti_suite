function survive_death_knight_blood()

end

-- local MARROWGAR = nil
-- local MARROWGAR_GUID = NOTARGET
-- local function set_MGUID(g)
--   if MARROWGAR_GUID == NOTARGET then
--     MARROWGAR_GUID = g
--     echo("MARROWGAR_GUID set to " .. g)
--   end
-- end
--
-- local function MARROWGAR_STUFF()
--
--   if not UnitAffectingCombat("player") then return end
--
--   local ts = {get_combat_targets()}
--   for i,g in pairs(ts) do
--     target_unit_with_GUID(g)
--     local tname = UnitName("target")
--     if not UnitIsDead("target") and tname == "Bone Spike" and BLAST_TARGET_GUID ~= g then
--       lole_subcommands.sendmacro("RAID", "/lole target", g);
--       return
--
--     elseif tname == "Lord Marrowgar" then
--       set_MGUID(g)
--     end
--   end
--
--   if BLAST_TARGET_GUID ~= MARROWGAR_GUID then
--     lole_subcommands.sendmacro("RAID", "/lole target", MARROWGAR_GUID);
--   end
--
-- end

local DK_DELETE_FRAME = nil
local FROZEN_ORB_TARGET = nil

local function TORAVON_STUFF()
  if FROZEN_ORB_TARGET then
    target_unit_with_GUID(FROZEN_ORB_TARGET)
  else
    L_ClearTarget()
  end

  if not UnitExists("target") then
    FROZEN_ORB_TARGET = nil
  end

  if not FROZEN_ORB_TARGET then
    for i, g in pairs({get_combat_targets()}) do
      target_unit_with_GUID(g)
      if UnitName("target") == "Frozen Orb" and not UnitIsDead("target") then
        FROZEN_ORB_TARGET = g
        echo("setting target to " .. g)
        lole_subcommands.broadcast("target", g)
        break
      end
    end
  end
end


local TANK_ACTION_SWITCH_TARGET = 1
local TANK_ACTION_TAUNT = 2

local function dk_taunt()

  -- return value of nil means "continue blast as usual"

  local action_needed, guid = lole_subcommands.tank("act")
  --echo(action_needed, guid)

  if action_needed == TANK_ACTION_TAUNT then
    L_CastSpellByName("Frost Presence")
    set_target(guid)
    caster_range_check(29)
    if cast_if_nocd("Dark Command") then
      return true
    elseif cast_if_nocd("Death Grip") then
      return true
    elseif cast_if_nocd("Icy Touch") then
      return true
    else
      return nil
    end

  elseif action_needed == TANK_ACTION_SWITCH_TARGET then
    set_target(guid)
    return true
  end

  return nil

end

combat_death_knight_blood = function()
  --
  -- if not MARROWGAR then
  --   MARROWGAR = CreateFrame("frame", nil, UIParent)
  --   MARROWGAR:SetScript("OnUpdate", MARROWGAR_STUFF)
  -- end

--  avoid_npc_with_name("Fire Bomb")

--  L_TargetUnit("Ghostly Priest")
--     if UnitExists("target") then
--       if UnitCastingInfo("target") and string.find(UnitCastingInfo("target"), "Fear") and GetSpellCooldown("Mind Freeze") == 0 then
--         L_CastSpellByName("Mind Freeze")
--       end
--     end
    --
    -- if not DK_DELETE_FRAME then
    --   DK_DELETE_FRAME = CreateFrame("frame", nil, UIParent)
    --   DK_DELETE_FRAME:SetScript("OnUpdate", TORAVON_STUFF)
    -- end

    if (tank_get("active") == 1) then
      -- TODO make a mechanism to also get a new target when the current one dies =D
      if dk_taunt() then return end
  --    if not validate_target() then return; end
      --caster_range_check(8)
    else
      if not validate_target() then return; end
      melee_attack_behind(5)
    end

    if not PetHasActionBar() and GetSpellCooldown("Raise Dead") == 0 then
        L_CastSpellByName("Raise Dead");
    end

    if not has_buff("player", "Horn of Winter") and GetSpellCooldown("Horn of Winter") == 0 then
        L_CastSpellByName("Horn of Winter");
    end

    -- Rune cooldown data
    local b1_start, b1_dur, b1_rdy = GetRuneCooldown(1);
    local b2_start, b2_dur, b2_rdy = GetRuneCooldown(2);
    local uh1_start, uh1_dur, uh1_rdy = GetRuneCooldown(3);
    local uh2_start, uh2_dur, uh2_rdy = GetRuneCooldown(4);
    local f1_start, f1_dur, f1_rdy = GetRuneCooldown(5);
    local f2_start, f2_dur, f2_rdy = GetRuneCooldown(6);

    local blood_rdy = b1_rdy or b2_rdy;
    local unholy_rdy = uh1_rdy or uh2_rdy;
    local frost_rdy = f1_rdy or f2_rdy;

    local frost_fever_expires = get_self_debuff_expiration("target", "Frost Fever");

    if IsUsableSpell("Rune Strike") then
      L_CastSpellByName("Rune Strike")
    end

    if blood_rdy == true and frost_fever_expires and frost_fever_expires - GetTime() < 8 and has_debuff_by_self("target", "Blood Plague") then
        L_CastSpellByName("Pestilence");
    elseif not has_debuff_by_self("target", "Frost Fever") and frost_rdy == true then
        L_CastSpellByName("Icy Touch");
    elseif not has_debuff_by_self("target", "Blood Plague") and unholy_rdy == true then
        L_CastSpellByName("Plague Strike");
    elseif unholy_rdy == true and frost_rdy == true then
        L_CastSpellByName("Death Strike");
    elseif blood_rdy == true then
        L_CastSpellByName("Heart Strike");
    else
        L_CastSpellByName("Death Coil");
    end

end
