combat_druid_resto = function()

	TargetUnit("Josp"); 
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	local own_hp_max = UnitHealthMax("player");
	local own_hp_cur = UnitHealth("player");
	
	if GetSpellCooldown("Remove Curse") > 0 then -- dont waste cycles on GCD
		return;
	end
	
	if own_hp_cur < 5500 then 
		TargetUnit("player")
	end
	
	--if not has_buff("player", "Tree of Life") then
		--CastSpellByName("Tree of Life");
		--return;
	--end
	
	if has_debuff_of_type("target", "Curse") then
		CastSpellByName("Remove Curse");
		return;
	end
	
	if not has_buff("target", "Lifebloom") then
		CastSpellByName("Lifebloom");
		return;
	end
	
	if (health_cur < health_max * 0.88) then
		if not has_buff("target", "Rejuvenation") then
			CastSpellByName("Rejuvenation");
		end
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.80) then
		TargetUnit("player");
		if not has_buff("target", "Rejuvenation") then
			CastSpellByName("Rejuvenation");
		end
	end
	

end

