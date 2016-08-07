combat_druid_balance = function()

	if UnitCastingInfo("player") then return; end

	--if decurse_party("Curse of the Shattered Hand") then return; end

	if not validate_target() then return end
--    TargetUnit("Kiggler the Crazed");

	caster_range_check(30)

	if not has_debuff("target", "Insect Swarm") then
		CastSpellByName("Insect Swarm");
	-- elseif not target_has_debuff("target", "Moonfire") then CastSpellByName("Moonfire");
	else
		CastSpellByName("Starfire");
	--	CastSpellByName("Wrath")
	end

end
