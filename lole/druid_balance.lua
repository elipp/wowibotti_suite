combat_druid_balance = function()

	if UnitCastingInfo("player") then return; end

	if not validate_target() then return end

	caster_range_check(35)
	caster_face_target()
	
	if not has_debuff("target", "Insect Swarm") then
		CastSpellByName("Insect Swarm");
	-- elseif not target_has_debuff("target", "Moonfire") then CastSpellByName("Moonfire");
	else
		CastSpellByName("Starfire");
	--	CastSpellByName("Wrath")
	end

end
