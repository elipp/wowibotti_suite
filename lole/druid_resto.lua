config_druid_resto = {}
config_druid_resto.name = "druid_resto";

config_druid_resto.MODE_ATTRIBS = {
	["playermode"] = 0
}

config_druid_resto.combat = function()

	TargetUnit("Raimo");
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	if GetSpellCooldown("Remove Curse") > 0 then -- dont waste cycles on GCD
		return;
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
	
	if (health_cur < health_max * 0.60) then
		if GetSpellCooldown("Swiftment") == 0 then
			CastSpellByName("Swiftmend");
		else
			CastSpellByName("Regrowth");
		end
	elseif (health_cur < health_max * 0.88) then
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

config_druid_resto.buffs = function(MISSING_BUFFS_COPY)

end

config_druid_resto.other = function()

end;
