
combat_mage_frost = function()
	
	caster_range_check(36); 
	caster_face_target();
	
	if UnitCastingInfo("player") then return; end
	CastSpellByName("Frostbolt");

end


