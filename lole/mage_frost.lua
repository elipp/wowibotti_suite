
combat_mage_frost = function()

  if player_casting() then return end

    if ((GetItemCount(22044) == 0) and (not UnitAffectingCombat("player"))) then
        CastSpellByName("Conjure Mana Emerald");
        return;
    end


    local mana = UnitMana("player");
    local maxmana = UnitManaMax("player");

    if mana < (maxmana - 2500) then
        if (GetItemCount(22044) > 0) and (GetItemCooldown(22044) == 0) then
            UseItemByName("Mana Emerald");
            return;
        end
    end

    if mana < 2500 then
        if GetSpellCooldown("Evocation") == 0 then
            CastSpellByName("Evocation");
        end
    end

    if not validate_target() then return end

    caster_range_check(36);

    -- if has_buff("target", "Power Word: Shield") then
    --  CastSpellByName("Spellsteal")
    --  return
    -- end
    --
    -- if has_buff("target", "Renew") then
    --  CastSpellByName("Spellsteal")
    --  return
    -- end

	CastSpellByName("Frostbolt");

end
