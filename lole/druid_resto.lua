MAIN_TANK = "Gawk";
combat_druid_resto = function()
    if MAIN_TANK == "Gawk" then
    	MAIN_TANK = "Noctur"
    else
    	MAIN_TANK = "Gawk"
    end

	TargetUnit(MAIN_TANK);
	local mana_left = UnitMana("player");

	local has, stacks, timeleft = has_buff("target", "Lifebloom")

	if has then
		if stacks < 3 then
			CastSpellByName("Lifebloom")
			return
		elseif timeleft < 0.7 then
			CastSpellByName("Lifebloom")
			return
		end
	else
		CastSpellByName("Lifebloom")
		return
	end

	if (not has_buff("focus", "Rejuvenation")) then
		CastSpellByName("Rejuvenation")
		return
	end

	if (UnitHealth("focus") < 5500) then
		CastSpellByName("Swiftmend")
		return
	end

end
