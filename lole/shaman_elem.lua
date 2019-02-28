
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

	if player_casting() then return end

	if not has_buff("player", "Water Shield") then
		L_CastSpellByName("Water Shield");
		return;
	end

	if refresh_totems(TOTEMS) then return; end

	if not validate_target() then return end
	caster_range_check(0,30);

	if lole_subcommands.get("aoemode") == 1 then
		L_CastSpellByName("Elemental Mastery")
		if cast_if_nocd("Chain Lightning") then return end
	end

	if cast_if_nocd("Chain Lightning") then return end
	L_CastSpellByName("Lightning Bolt");

end
