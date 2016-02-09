combat_paladin_retri = function()
	
	if cleanse_party("Arcane Shock") then return; end
	
	if not validate_target() then return; end
	
	melee_close_in()
	
	if not has_debuff("target", "Judgement of the Crusader") then
		if not has_buff("player", "Seal of the Crusader") then
			cast_if_nocd("Seal of the Crusader");
		end
		cast_if_nocd("Judgement");
		return;
	end

	if not has_buff("player", "Seal of Blood") then
		cast_if_nocd("Seal of Blood");
		return;
	end

	if cast_if_nocd("Crusader Strike") then return end

	if cast_if_nocd("Judgement") then return end


end

