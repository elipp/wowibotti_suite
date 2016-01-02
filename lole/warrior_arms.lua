local function auto_stancedance() 
	local name = UnitCastingInfo("target");
	if (name == "Bellowing Roar") then
		echo("Voi vittu, castaa bellowing roarii!!");
		-- CastSpellByName("Berserker Stance");
		-- CastSpellByName("Berserker Rage");
		-- CastSpellByName("Defensive Stance");
	end

end

config_warrior_arms = {}
config_warrior_arms.name = "warrior_arms";

config_warrior_arms.SELF_BUFFS = { "Battle Shout" };

config_warrior_arms.combat = function()
	
	ClosePetStables(); -- this is a hooked function that makes the warrior walk behind/toward the target.
	-- CTM_MOVE_AND_ATTACK is performed, so no need to mess around with StartAttack()
	
	if UnitIsDead("target") then ClearTarget() end; -- this is probably not good, sometimes follow is lost?
	
	if (not UnitAffectingCombat("player")) then
		RunMacroText("/cast [nostance:1] Battle Stance"); -- charge doesnt work
		RunMacroText("/cast Charge");
		return;
	end
		
	RunMacroText("/cast [nostance:3] Berserker Stance"); -- overall, its probably better to be in zerg stance :D

	if UnitCastingInfo("target") then
		RunMacroText("/cast [nostance:3] Berserker Stance");
		cast_if_nocd("Pummel");
	end

	if not has_debuff("target", "Blood Frenzy") then
		RunMacroText("/cast [nostance:1] Battle Stance");
		RunMacroText("/cast Rend");
	end
	
	if cast_if_nocd("Mortal Strike") then return; end
	if cast_if_nocd("Whirlwind") then return; end
	
	
	if not has_buff("player", "Battle Shout") then
		CastSpellByName("Battle Shout");
		return;
	end
		
	if not has_debuff("target", "Thunder Clap") then
		if cast_if_nocd("Thunder Clap") then return; end
	end

	if not has_debuff("target", "Demoralizing Shout") then
		CastSpellByName("Demoralizing Shout");
		return;
	end

	-- if more than X rage, slam
	
end

config_warrior_arms.cooldowns = function() 

end


config_warrior_arms.buffs = function()

end

config_warrior_arms.desired_buffs = function()

    local desired_buffs = {
        "Blessing of Kings",
        "Blessing of Might",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Mark of the Wild",
        "Thorns",
    }

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;

end

config_warrior_arms.other = function()

end;




