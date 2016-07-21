local TOTEMS = {
--["air"] = "Grace of Air Totem",

["air"] = "Windfury Totem",
--["air"] = "Wrath of Air Totem",

--["earth"] = "Tremor Totem",
["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",
}

function check_WF()
    local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
    --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

    if (not has_mh or not has_oh) then
        CastSpellByName("Windfury Weapon")
    end

end

combat_shaman_enh = function()

    check_WF()

    if refresh_totems(TOTEMS) then return; end

    if not validate_target() then return end
	melee_attack_behind()

    if not has_buff("player", "Water Shield") then
        CastSpellByName("Water Shield")
        return
    end

    if cast_if_nocd("Stormstrike") then return end

    if (UnitMana("player") < 1000) then
        if cast_if_nocd("Shamanistic Rage") then return end
    end

    if cast_if_nocd("Earth Shock") then return end;

    CastSpellByName("Purge")

end
