local function curator()
	TargetUnit("Astral Flare")
	if (UnitExists("target") and not UnitIsDead("target") and UnitName("target") == "Astral Flare") then
		lole_subcommands.broadcast("target", UnitGUID("target"));
		set_target(UnitGUID("target"))
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

combat_druid_feral = function()

  --if curator() then return end
if terestian() then return end

    if not validate_target() then return end

    --melee_avoid_aoe_buff(33238)
	melee_attack_behind()

    if (not has_buff("player", "Omen of Clarity")) then
        CastSpellByName("Omen of Clarity")
        return
    end

    if (not has_buff("player", "Cat Form")) then
        CastSpellByName("Cat Form")
        return
    end

    if (not has_debuff("target", "Faerie Fire")) then
        CastSpellByName("Faerie Fire (Feral)(Rank 5)")
        return
    end

    if (not has_debuff("target", "Mangle")) then
        CastSpellByName("Mangle (Cat)(Rank 3)")
        return
    end

    if (GetComboPoints("player", "target") < 5) then
        CastSpellByName("Shred")
        return;
    end

    if (UnitMana("player") < 70) then
        return
    end

    CastSpellByName("Rip")



end
