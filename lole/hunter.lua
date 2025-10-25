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


local FROST_SPHERE_TARGET = nil

local function FROST_TARGET_STUFF()
    if FROST_SPHERE_TARGET then
        target_unit_with_GUID(FROST_SPHERE_TARGET)
    else
        L_ClearTarget()
    end

    if not UnitExists("target") then
        FROST_SPHERE_TARGET = nil
    end

    if not FROST_SPHERE_TARGET then
        for i, g in pairs({ get_combat_targets() }) do
            target_unit_with_GUID(g)
            if UnitName("target") == "Frost Sphere" and not UnitIsDead("target") then
                FROST_SPHERE_TARGET = g
                echo("setting frost sphere target to " .. g)
                break
            end
        end
    end
end


    -------------- THIS STUFF IS FOR ANUB ARAK ------------------------
    -- MUTUALLY EXCLUSIVE WITH DEFAULT VALIDATE TARGET
    --   if (UnitExists("focus") and UnitName("focus") == "Anub'arak") then
    --       if health_percentage("focus") < 30 then
    --         if not validate_target() then return end
    --       else
    --         FROST_TARGET_STUFF()
    --       end
    --   elseif not validate_target() then return end

    ----------------------------------------------------------------


local function cast_if_nocd_pet(spell, rank)
    -- local name, rank = GetSpellInfo(1-whatever, "pet") could be used to find newest rank
    return cast_if_nocd(spell, rank, "pet")
end


function pet_combat()
    L_PetAttack()

    if not UnitExists("pettarget") then return end
    if UnitPower("pet") <= 15 then return end

    if get_distance_between("pet", "pettarget") > 10 and cast_if_nocd_pet("Charge") then
        return
    end

    if get_aoe_feasibility("pet", 8) > 1.0 and cast_if_nocd_pet("Thunderstomp") then
        return
    end

    if cast_if_nocd_pet("Growl", 7) then
        return
    end
    if cast_if_nocd_pet("Gore", 4) then
        return
    end
    if cast_if_nocd_pet("Bite", 8) then
        return
    end
end


function combat_hunter()
    if not petframe_dummy then
        petframe_dummy = CreateFrame("frame", nil, UIParent)
        petframe_dummy:SetScript("OnUpdate", petfollow_default)
    end

    if UnitExists("pet") and UnitIsDead("pet") then
        L_CastSpellByName("Revive Pet")
    elseif not UnitExists("pet") or not PetHasActionBar() then
        return L_CastSpellByName("Call Pet")
    end

    if not UnitAffectingCombat("player") then
        --L_PetPassiveMode()

        if not has_buff("player", "Aspect of the Viper") then
            change_aspect("Aspect of the Viper")
            -- must NOT have return in this
        end
    end

    local pet_pct = health_percentage("pet")
    if pet_pct < 80 and not has_buff("pet", "Mend Pet") then
        return L_CastSpellByName("Mend Pet")
    end


    if not validate_target() then return end -- DEFAULT
    -- caster_range_check(11, 35)

    --local BEST_ASPECT = "Aspect of the Dragonhawk"
    -- local BEST_ASPECT = "Aspect of the Wild"
    local BEST_ASPECT = "Aspect of the Hawk"

    local mana_pct = mana_percentage("player")

    if mana_pct > 50 and (not has_buff("player", BEST_ASPECT)) then
        if change_aspect(BEST_ASPECT) then return end
    elseif UnitMana("player") < 500 and (not has_buff("player", "Aspect of the Viper")) then
        if change_aspect("Aspect of the Viper") then return end
        return
    end

    pet_combat()
    L_StartAttack()

    -- if GetSpellCooldown("Kill Command") == 0 then
    --     L_CastSpellByName("Kill Command")
    --     return;
    -- end

    local target_health_pct = health_percentage("target")
    -- if target_health_pct < 20 then
    --     L_CastSpellByName("Kill Shot")
    --     -- no return, will not be cast if incooldown
    -- end

    if not has_debuff("target", "Hunter's Mark") then
        L_CastSpellByName("Hunter's Mark")
    end


    if not has_debuff("target", "Serpent Sting") then
        return L_CastSpellByName("Serpent Sting")
    end

    if cast_if_nocd("Arcane Shot") then return end
    if cast_if_nocd("Multi-Shot") then return end

    L_CastSpellByName("Steady Shot")
end
