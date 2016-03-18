local scorpid_time = 0

combat_hunter = function()
    PetPassiveMode() -- just do this no matter what :D

    if not validate_target() then
        PetPassiveMode()
        PetStopAttack()
        PetWait()
        PetFollow()
        return;
    end

    if not PetHasActionBar() then
        CastSpellByName("Call Pet");
        CastSpellByName("Revive Pet");
    end

    if UnitMana("player") < 800 then
        if not has_buff("player", "Aspect of the Viper") then
            CastSpellByName("Aspect of the Viper")
            return;
        end
    elseif UnitMana("player") > 2500 then
        if not has_buff("player", "Aspect of the Hawk") then
            CastSpellByName("Aspect of the Hawk")
        end
    end

    if not has_buff("pet", "Mend Pet") and UnitHealthMax("pet") - UnitHealth("pet") > 2000 then
        CastSpellByName("Mend Pet");
        return;
    end

    if not validate_target() then return end

    PetAttack()
    caster_range_check(35)

    if GetSpellCooldown("Claw") == 0 then
        CastSpellByName("Scorpid Poison")
        CastSpellByName("Claw")
    end

    if not has_debuff("target", "Hunter's Mark") then
        CastSpellByName("Hunter's Mark");
        return
    end

    if not has_debuff("target", "Scorpid Sting") then
        CastSpellByName("Scorpid Sting");
        return
    end


    if lole_subcommands.get("aoemode") then
        if cast_if_nocd("Multi Shot") then return end
    end

    if IsUsableSpell("Kill Command") then
        if cast_if_nocd("Kill Command") then return; end
    end

    CastSpellByName("Steady Shot");

end
