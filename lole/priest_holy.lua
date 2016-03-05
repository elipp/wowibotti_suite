local pom_time = 0;

combat_priest_holy = function()

	if casting_legit_heal() then return end

	TargetUnit("Adieux");
	local mana_left = UnitMana("player");

	if mana_left < 3000 then
		CastSpellByName("Shadowfiend");
		return;
	end


	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");

	if time() - pom_time > 20 then
		if cast_if_nocd("Prayer of Mending") then
			pom_time = time();
			return;
		end
	end

	if not has_buff("target", "Renew") then
		CastSpellByName("Renew");
		return;
	end

	if cleanse_party("Static Disruption") then return end

	if casting_legit_heal() then return end

	if (health_cur < health_max * 0.60) then
		cast_spell("Greater Heal");
	elseif (health_cur < health_max * 0.88) then
		cast_spell("Greater Heal(Rank 4)");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.65) then
		cast_spell("Binding Heal");
	end

end
