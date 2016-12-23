local function auto_stancedance()
	local name = UnitCastingInfo("target");
	if (name == "Bellowing Roar") then
		echo("Voi vittu, castaa bellowing roarii!!");
		-- CastSpellByName("Berserker Stance");
		-- CastSpellByName("Berserker Rage");
		-- CastSpellByName("Defensive Stance");
	end

end

combat_warrior_prot = function()

	RunMacroText("/cast [nostance:2] Defensive Stance")

	if not has_buff("player", "Commanding Shout") then
		CastSpellByName("Commanding Shout");
	end

	if cast_if_nocd("Shield Block") then return; end
	if cast_if_nocd("Revenge") then return end

	CastSpellByName("Heroic Strike");

	if UnitCastingInfo("target") then
		if not cast_if_nocd("Spell Reflect") then
			if cast_if_nocd("Shield Bash") then return end
		end
	end

	if cast_if_nocd("Shield Slam") then return; end

	if not has_debuff("target", "Thunder Clap") then
		CastSpellByName("Thunder Clap");
		return;
	end

	local num_stacks, timeleft = get_num_debuff_stacks("target", "Sunder Armor");
	if num_stacks < 5 or timeleft < 12 then
		CastSpellByName("Devastate")
		return
	end

	if not has_debuff("target", "Demoralizing Shout") then
		CastSpellByName("Demoralizing Shout");
		return;
	end

	CastSpellByName("Devastate")


end
