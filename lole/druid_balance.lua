combat_druid_balance = function()

	caster_range_check(35);
	caster_face_target();
	
	if UnitCastingInfo("player") then return; end

	if not UnitExists("target") then return; end

	
	if not has_debuff("target", "Insect Swarm") then 
		CastSpellByName("Insect Swarm"); 
	-- elseif not target_has_debuff("target", "Moonfire") then CastSpellByName("Moonfire");
	else 
		CastSpellByName("Starfire"); 
	end

	
	 --special shit for curator
	--CastSpellByName("Wrath");
	
end



