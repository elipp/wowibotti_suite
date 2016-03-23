local TOTEMS = {
["air"] = "Grace of Air Totem",

--["air"] = "Windfury Totem",
--["air"] = "Wrath of Air Totem",

["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

--["fire"] = "Totem of Wrath"
}


combat_shaman_enh = function()

    if refresh_totems(TOTEMS) then return; end

    if not validate_target() then return end
	melee_attack_behind()


    if cast_if_nocd("Stormstrike") then return end

    if (UnitMana("player") < 1000) then
        if cast_if_nocd("Shamanistic Rage") then return end
    end

    cast_if_nocd("Earth Shock");

end
