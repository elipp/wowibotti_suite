
local TOTEMS = {
--["air"] = "Windfury Totem",
["air"] = "Wrath of Air Totem",

["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Totem of Wrath"
}


combat_shaman_elem = function()

	if UnitCastingInfo("player") then return; end

	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	if refresh_totems(TOTEMS) then return; end

	
	caster_range_check(30); 
	caster_face_target();

	CastSpellByName("Lightning Bolt");
	
end

