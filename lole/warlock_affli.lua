local petframe_dummy = nil

function combat_warlock_affliction()

    if not petframe_dummy then
        petframe_dummy = CreateFrame("Frame", nil, UIParent)
        petframe_dummy:SetScript("OnUpdate", petfollow_default)
    end

    if not validate_target() then return end

    local pet_spell = "Summon Felhunter"
    if not UnitExists("pet") or not PetHasActionBar() then
        L_CastSpellByName(pet_spell)
    else
        if UnitCastingInfo("player") == pet_spell then
            L_SpellStopCasting()
        end
    end

    if UnitAffectingCombat("player") then
        L_PetAttack()
    end

    caster_range_check(11, 35)

    -- ================================================
    -- GLYPH OF LIFE TAP - Keep buff active before DoTs
    -- ================================================
    if not has_buff("player", "Glyph of Life Tap") then
        L_CastSpellByName("Life Tap")
        return
    end

    -- ================================================
    -- SHADOW MASTERY - Must be up before Corruption
    -- ================================================
    if not has_debuff("target", "Shadow Mastery") then
        L_CastSpellByName("Shadow Bolt")
        return
    end

    -- ================================================
    -- MANA - Life Tap if running low
    -- ================================================
    local mana = UnitMana("player") / UnitManaMax("player")
    if mana < 0.20 then
        L_CastSpellByName("Life Tap")
        return
    end

    -- ================================================
    -- DOTS / DEBUFF PRIORITY
    -- ================================================

    -- Unstable Affliction (keep up at all times)
    local ua_has, ua_dur = has_debuff("target", "Unstable Affliction")
    if not ua_has or ua_dur < 1.5 then
        L_CastSpellByName("Unstable Affliction")
        return
    end

    -- Haunt (20% periodic damage boost - keep up at all times)
    local haunt_has, haunt_dur = has_debuff("target", "Haunt")
    if not haunt_has or haunt_dur < 1.5 then
        if GetSpellCooldown("Haunt") == 0 then
            L_CastSpellByName("Haunt")
            return
        end
    end

    -- Corruption (highest priority DoT, only apply after Shadow Mastery is up)
    local corr_has, corr_dur = has_debuff("target", "Corruption")
    if not corr_has or corr_dur < 1.5 then
        L_CastSpellByName("Corruption")
        return
    end

    -- Curse of Agony (if Curse of Elements is covered by another player)
    -- Swap to Curse of the Elements if no other source in raid
    local coa_has, coa_dur = has_debuff("target", "Curse of Agony")
    if not coa_has or coa_dur < 1.5 then
        L_CastSpellByName("Curse of Agony")
        return
    end

    -- ================================================
    -- FILLER - Drain Soul sub-25%, Shadow Bolt otherwise
    -- ================================================
    local hp_pct = UnitHealth("target") / UnitHealthMax("target")

    if hp_pct < 0.25 then
        -- Drain Soul: clip after each tick to re-snapshot or reapply DoTs
        L_CastSpellByName("Drain Soul")
    else
        L_CastSpellByName("Shadow Bolt")
    end

end
