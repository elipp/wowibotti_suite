local function curator()
	TargetUnit("Astral Flare")
	if (UnitExists("target") and not UnitIsDead("target") and UnitName("target") == "Astral Flare") then
		lole_subcommands.broadcast("target", UnitGUID("target"));
		set_target(UnitGUID("target"))
		RunMacroText("/cast [nostance:3] Berserker Stance")
		RunMacroText("/cast Intercept");
		return true;
	else
		TargetUnit("The Curator")
		if (UnitExists("target") and not UnitIsDead("target") and UnitName("target") == "The Curator") then
			local spell = UnitChannelInfo("target");
			if spell == "Evocation" then
				lole_subcommands.broadcast("target", UnitGUID("target"));
				set_target(UnitGUID("target"))
				return true;
			end
		end
	end

	return false;
end


local function terestian()
	TargetUnit("Demon Chains")
	local GUID = UnitGUID("target")
	if (UnitExists("target") and not UnitIsDead("target") and UnitName("target") == "Demon Chains") then
		lole_subcommands.broadcast("target", UnitGUID("target"));
		set_target(UnitGUID("target"))
		return true;
	else
		TargetUnit("Terestian Illhoof")
		if (UnitExists("target") and not UnitIsDead("target") and UnitName("target") == "Terestian Illhoof") then
			lole_subcommands.broadcast("target", UnitGUID("target"));
			set_target(UnitGUID("target"))
		end
	end

	return false;
end

combat_warrior_arms = function()

	--if terestian() then return end

	--if curator() then return end

	if not validate_target() then return end

	melee_attack_behind()

	if (not UnitAffectingCombat("player")) then
		RunMacroText("/cast [nostance:1] Battle Stance"); -- charge doesnt work
		RunMacroText("/cast Charge");
		return;
	end

	RunMacroText("/cast [nostance:3] Berserker Stance"); -- overall, its probably better to be in zerg stance :D

	if GetSpellCooldown("Bloodrage") == 0 then
		L_CastSpellByName("Bloodrage")
	end

	if not has_buff("player", "Battle Shout") then
		L_CastSpellByName("Battle Shout");
		return;
	end

	if UnitCastingInfo("target") then
		RunMacroText("/cast [nostance:3] Berserker Stance");
		cast_if_nocd("Pummel");
	end

	if not has_debuff("target", "Blood Frenzy") then
		RunMacroText("/cast [nostance:1] Battle Stance");
		RunMacroText("/cast Rend");
	end

	if cast_if_nocd("Mortal Strike") then return; end

	if lole_subcommands.get("aoemode") == 1 then
		if cast_if_nocd("Whirlwind") then return; end
		if UnitMana("player") > 65 then
			L_CastSpellByName("Cleave")
		end
	else
		if UnitMana("player") > 65 then
		--if GetTime() - swing_starttime < 1.5 then
			if cast_if_nocd("Slam") then return end;
		--end
		end
	end



	if not has_debuff("target", "Thunder Clap") then
		if cast_if_nocd("Thunder Clap") then return; end
	end

	if not has_debuff("target", "Demoralizing Shout") then
		L_CastSpellByName("Demoralizing Shout");
		return;
	end


end
