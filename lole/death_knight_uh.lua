function survive_death_knight_uh()

end

combat_death_knight_uh = function()
    L_TargetUnit("Ghostly Priest")
    if UnitExists("target") then
        if UnitCastingInfo("target") and string.find(UnitCastingInfo("target"), "Fear") and GetSpellCooldown("Mind Freeze") == 0 then
            L_CastSpellByName("Mind Freeze")
        end
    end

    if not validate_target() then return; end

    melee_attack_behind()

    if not PetHasActionBar() and GetSpellCooldown("Raise Dead") == 0 then
        L_CastSpellByName("Raise Dead");
    end

    if not has_buff("player", "Bone Shield") and GetSpellCooldown("Bone Shield") == 0 then
        L_CastSpellByName("Bone Shield");
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

    local frost_fever_timeleft = get_self_debuff_expiration("target", "Frost Fever");

    if blood_rdy == true and frost_fever_expires and frost_fever_timeleft < 8 and has_debuff_by_self("target", "Blood Plague") then
        L_CastSpellByName("Pestilence");
    elseif not has_debuff_by_self("target", "Frost Fever") and frost_rdy == true then
        L_CastSpellByName("Icy Touch");
    elseif not has_debuff_by_self("target", "Blood Plague") and unholy_rdy == true then
        L_CastSpellByName("Plague Strike");
    elseif unholy_rdy == true and frost_rdy == true then
        L_CastSpellByName("Scourge Strike");
    elseif blood_rdy == true then
        L_CastSpellByName("Blood Strike");
    else
        L_CastSpellByName("Death Coil");
    end
end
