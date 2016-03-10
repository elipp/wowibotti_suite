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

    if not validate_target() then return end

    PetAttack()

    caster_range_check(35)

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

    if IsUsableSpell("Kill Command") then CastSpellByName("Kill Command") return; end

    CastSpellByName("Steady Shot");

end
