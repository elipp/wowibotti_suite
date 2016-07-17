combat_druid_resto = function()

	TargetUnit("focus");
	local mana_left = UnitMana("player");

	local has, stacks, timeleft = has_buff("focus", "Lifebloom")

	if has then
		if stacks < 3 then
			CastSpellByName("Lifebloom")
			return
		else if timeleft < 0.7 then
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
