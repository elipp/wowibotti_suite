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

    PetAttack()

    if not has_debuff("target", "Hunter's Mark") then
            CastSpellByName("Hunter's Mark");
            return
    end

    if not has_debuff("target", "Serpent Sting") then
        CastSpellByName("Serpent Sting");
        return
    end

    if IsUsableSpell("Kill Command") then CastSpellByName("Kill Command") return; end

    CastSpellByName("Steady Shot");

end
