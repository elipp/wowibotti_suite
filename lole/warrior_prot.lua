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
	
	CastSpellByName("Heroic Strike");

	if UnitCastingInfo("target") then
		if not cast_if_nocd("Spell Reflect") then
			cast_if_nocd("Shield Bash");
		end
		return;
	end

	if cast_if_nocd("Shield Block") then return; end
	if cast_if_nocd("Shield Bash") then return; end
	
	if IsUsableSpell("Revenge") then
		CastSpellByName("Revenge");
		return;
	end

	if not has_buff("player", "Commanding Shout") then
		CastSpellByName("Commanding Shout");
	end

	if not has_debuff("target", "Thunder Clap") then
		CastSpellByName("Thunder Clap");
		return;
	end

	if not has_debuff("target", "Demoralizing Shout") then
		CastSpellByName("Demoralizing Shout");
		return;
	end

	CastSpellByName("Devastate");

end


