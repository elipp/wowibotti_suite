
local TOTEMS = {
--["air"] = "Windfury Totem",
--["air"] = "Nature Resistance Totem",
["air"] = "Wrath of Air Totem",

["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Totem of Wrath"
}


combat_shaman_elem = function()

    local spell = UnitCastingInfo("target");
    if spell == "Heal" or spell == "Holy Light" or spell == "Healing Wave" then
        if GetSpellCooldown("Earth Shock") > 1 then
            lole_subcommands.override("Consona", "Counterspell");
        else
            SpellStopCasting();
            CastSpellByName("Earth Shock");
            SendChatMessage("Cast Earth Shock", "GUILD");
        end
    end

	if player_casting() then return end

	if not has_buff("player", "Water Shield") then
		CastSpellByName("Water Shield");
		return;
	end

	if refresh_totems(TOTEMS) then return; end

	if not validate_target() then return end
	caster_range_check(30);

	if lole_subcommands.get("aoemode") == 1 then
		CastSpellByName("Elemental Mastery")
		if cast_if_nocd("Chain Lightning") then return end
	end

	--if cast_if_nocd("Chain Lightning") then return end
	CastSpellByName("Lightning Bolt");

end
