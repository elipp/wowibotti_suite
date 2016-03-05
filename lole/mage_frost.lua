
combat_mage_frost = function()

	caster_range_check(36);
	
	if UnitCastingInfo("player") then return; end
	CastSpellByName("Frostbolt");

end
