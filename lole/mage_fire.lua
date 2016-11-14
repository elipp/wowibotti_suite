local function delrissa()

	if keep_CCd("Warlord Salaris", "Polymorph") then return end

	if has_buff("Priestess Delrissa", "Power Word: Shield") or
	   has_buff("Priestess Delrissa", "Renew") then
		TargetUnit("Priestess Delrissa")
		CastSpellByName("Spellsteal")
		return;
	end


	if not validate_target() then return end;

	--caster_range_check(36);
--	caster_face_target();

	CastSpellByName("Fireball")



end

local function MAULGAR_KROSH()
	if UnitName("player") ~= "Dissona" then
		return false
	end

	TargetUnit("Krosh Firehand")

	if UnitIsDead("target") then return false end

	caster_range_check(30);

	if (has_buff("target", "Spell Shield")) then
		SpellStopCasting()
		CastSpellByName("Spellsteal")
		return true;
	end

	CastSpellByName("Fireball")

	return true;

end


combat_mage_fire = function()

	--if MAULGAR_KROSH() then return end
	if player_casting() then return end

	if ((GetItemCount(22044) == 0) and (not UnitAffectingCombat("player"))) then
		CastSpellByName("Conjure Mana Emerald");
		return;
	end

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	if mana < (maxmana - 2500) then
		if (GetItemCount(22044) > 0) and (GetItemCooldown(22044) == 0) then
			UseItemByName("Mana Emerald");
			return;
		end
	end

	if mana < 2500 then
		if GetSpellCooldown("Evocation") == 0 then
			CastSpellByName("Evocation");
		end
	end


	if not validate_target() then return end
	caster_range_check(36);

	if lole_subcommands.get("aoemode") == 1 then
			lole_subcommands.cast_gtaoe("Flamestrike", get_unit_position("target"))
			return
	end

	local talent_name, _, tier, column, talent_rank, _, _, _ = GetTalentInfo(2, 10); -- improved scorch

	if talent_rank == 3 then
		local num_stacks = get_num_debuff_stacks("target", "Fire Vulnerability");

		if num_stacks < 5 then
			CastSpellByName("Scorch");
			return;
		end

		local hasdebuff, timeleft = has_debuff("target", "Fire Vulnerability");

		if (hasdebuff and timeleft < 8) then
			CastSpellByName("Scorch");
			return;
		end
	end

	CastSpellByName("Fireball");

end
