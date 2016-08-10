combat_paladin_holy = function()
    MAIN_TANK = "Adieux";

	TargetUnit(MAIN_TANK);
	local mana_left = UnitMana("player");

	if mana_left < 4000 then
		CastSpellByName("Divine Illumination");
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

    caster_range_check(35);

	if (health_cur < health_max * 0.30) then
		CastSpellByName("Divine Favor");
		CastSpellByName("Divine Illumination");
		cast_spell("Holy Light");
    elseif (UnitHealth("player") < UnitHealthMax("player")*0.30) then
        TargetUnit("player");
        cast_spell("Holy Light");
	elseif (health_cur < health_max * 0.70) then
		cast_spell("Holy Light");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.50) then
		TargetUnit("player");
		cast_spell("Holy Light");
	elseif (health_cur < health_max * 0.90) then
		cast_spell("Flash of Light");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.70) then
		TargetUnit("player");
		cast_spell("Flash of Light");
	end


end
