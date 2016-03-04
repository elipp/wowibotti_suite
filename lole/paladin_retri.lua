combat_paladin_retri = function()
	
	--if cleanse_party("Arcane Shock") then return; end
	
	if not validate_target() then return; end
	
	melee_close_in()
	
    local jud_on_cd = true;
	if GetSpellCooldown("Judgement") == 0 then
        jud_on_cd = false;
    end

    if not jud_on_cd and not has_debuff("target", "Judgement of the Crusader") then
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

	if not jud_on_cd then
        CastSpellByName("Judgement");
    end

end

