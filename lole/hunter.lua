local scorpid_time = 0

local pet_warning_given = nil
local function feed_pet_if_need_to()

    if has_buff("pet", "Feed Pet Effect") then return false end
    local happiness = GetPetHappiness()

    if happiness ~= nil and happiness < 3 then
        local _, link = GetItemInfo(29451) -- Clefthoof Ribs
        local bag, slot = get_item_bag_position(link)

        if not bag then
            if not pet_warning_given then
                SendChatMessage("Pet is unhappy, and I don't's gots no [ribs] for him!", "GUILD")
                pet_warning_given = true
            end
            return false
        end

        PickupContainerItem(bag, slot)

        DropItemOnUnit("pet")

        return true
    end

    return false

end

local PET_NAME = "prwlr"

combat_hunter = function()

    if not UnitExists(PET_NAME) then
      L_CastSpellByName("Call Pet")
    end

    --PetPassiveMode()

    if not UnitAffectingCombat("player") then
      L_CastSpellByName("Aspect of the Viper")


      if UnitIsDead(PET_NAME) then
        L_CastSpellByName("Revive Pet")
        return
      end

      if feed_pet_if_need_to() then
        return
      end

    end


    if not validate_target() then
        PetStopAttack()
        PetWait()
        PetFollow()
        return;
    end

    if UnitMana("player") < 800 then
        if not has_buff("player", "Aspect of the Viper") then
            L_CastSpellByName("Aspect of the Viper")
            return;
        end
    elseif UnitMana("player") > 2500 then
        if not has_buff("player", "Aspect of the Dragonhawk") then
            L_CastSpellByName("Aspect of the Dragonhawk")
            return
        end
    end

    if not has_buff("pet", "Mend Pet")
    and (UnitHealthMax("pet") - UnitHealth("pet")) > 1000 then
        L_CastSpellByName("Mend Pet");
        return;
    end

    PetAttack()
    caster_range_check(0,35)

    StartAttack()

    cast_if_nocd("Kill Command")


    if GetSpellCooldown("Gore") == 0 then
        if GetSpellCooldown("Bite") == 0 then
            L_CastSpellByName("Bite")
        else
            L_CastSpellByName("Gore")
        end
    end

    if player_casting() then return end

  --  if lole_subcommands.get("aoemode") == 1 then
  		--	lole_subcommands.cast_gtaoe("Volley", get_unit_position("target"))
      --  return
  --	end


    if not has_debuff("target", "Hunter's Mark") then
        L_CastSpellByName("Hunter's Mark");
        return
    end

    if lole_subcommands.get("aoemode") then
        if cast_if_nocd("Multi Shot") then return end
    end

    L_CastSpellByName("Steady Shot");

end
