combat_hunter = function()
    -- PetPassiveMode() -- just do this no matter what :D
    --
    -- if not validate_target() then
    --     PetPassiveMode()
    --     PetStopAttack()
    --     PetWait()
    --     PetFollow()
    --     return;
    -- end
    --
    --
    -- if not PetHasActionBar() then
    --     CastSpellByName("Call Pet");
    --     CastSpellByName("Revive Pet");
    -- end
    --
    -- PetAttack()

    if not validate_target() then return end

    caster_range_check(35)

    if not has_debuff("target", "Hunter's Mark") then
            CastSpellByName("Hunter's Mark");
            return
    end

    if not has_debuff("target", "Scorpid Sting") then
        CastSpellByName("Scorpid Sting");
        return
    end

    --if IsUsableSpell("Kill Command") then CastSpellByName("Kill Command") return; end

    CastSpellByName("Steady Shot");

end
