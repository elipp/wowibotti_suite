combat_paladin_holy = function()

	TargetUnit("focus");
	local mana_left = UnitMana("player");

	if mana_left < 6000 then
		CastSpellByName("Divine Illumination");
		CastSpellByName("Divine Favor");
	end

	if UnitHealth("player") < 2000 then
		if (GetSpellCooldown("Divine Shield") == 0) then
			SpellStopCasting();
			CastSpellByName("Divine Shield");
		end
	end

	if casting_legit_heal() then return end
	
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	if has_debuff("target", "Shadow Word: Pain") then
		cast_spell("Cleanse");
	end
	
	if (health_cur < health_max * 0.60) then
		cast_spell("Holy Light");
	elseif (health_cur < health_max * 0.88) then
		cast_spell("Flash of Light");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.65) then
		TargetUnit("player");
		cast_spell("Flash of Light");
	end
	

end

