combat_druid_balance = function()

	if player_casting() then return end

    if (not has_buff("player", "Moonkin Form")) then
        CastSpellByName("Moonkin Form")
        return
    end

	--if decurse_party("Curse of the Shattered Hand") then return; end

    TargetUnit("Kiggler the Crazed");
    if UnitExists("target") and not UnitIsDead("target") then
    else
        if not validate_target() then return end
        caster_range_check(30)
        if not has_debuff("target", "Insect Swarm") then
            CastSpellByName("Insect Swarm");
            return;
        end
    end

	caster_range_check(30)

	if lole_subcommands.get("aoemode") == 1 then
		if GetSpellCooldown("Hurricane") == 0 then
					lole_subcommands.cast_gtaoe("Hurricane", get_unit_position("target"))
		end
	end


    CastSpellByName("Starfire");

end
