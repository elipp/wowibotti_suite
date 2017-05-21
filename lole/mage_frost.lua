
combat_mage_frost = function()

  if player_casting() then return end

    if ((GetItemCount(22044) == 0) and (not UnitAffectingCombat("player"))) then
        L_CastSpellByName("Conjure Mana Emerald");
        return;
    end


    local mana = UnitMana("player");
    local maxmana = UnitManaMax("player");

    if mana < (maxmana - 2500) then
        if (GetItemCount(22044) > 0) and (GetItemCooldown(22044) == 0) then
            L_UseItemByName("Mana Emerald");
            return;
        end
    end

    if mana < 2500 then
        if GetSpellCooldown("Evocation") == 0 then
            L_CastSpellByName("Evocation");
        end
    end

    if not validate_target() then return end

    caster_range_check(36);

    -- if has_buff("target", "Power Word: Shield") then
    --  L_CastSpellByName("Spellsteal")
    --  return
    -- end
    --
    -- if has_buff("target", "Renew") then
    --  L_CastSpellByName("Spellsteal")
    --  return
    -- end

	L_CastSpellByName("Frostbolt");

end
