local function delrissa()

	if has_buff("Priestess Delrissa", "Power Word: Shield") or
	   has_buff("Priestess Delrissa", "Renew") then
		L_TargetUnit("Priestess Delrissa")
		L_CastSpellByName("Spellsteal")
		return;
	end


	if not validate_target() then return end;

	--caster_range_check(36);
--	caster_face_target();

	L_CastSpellByName("Fireball")



end

local function MAULGAR_KROSH()
	if UnitName("player") ~= "Dissona" then
		return false
	end

	L_TargetUnit("Krosh Firehand")

	if UnitIsDead("target") then return false end

	caster_range_check(30);

	if (has_buff("target", "Spell Shield")) then
		SpellStopCasting()
		L_CastSpellByName("Spellsteal")
		return true;
	end

	L_CastSpellByName("Fireball")

	return true;

end


combat_mage_fire = function()

	if player_casting() then return end

	if ((GetItemCount(33312) == 0) and (not UnitAffectingCombat("player"))) then
		L_CastSpellByName("Conjure Mana Gem");
		return;
	end

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	-- if mana < (maxmana - 2500) then
	-- 	if (GetItemCount(33312) > 0) and (GetItemCooldown(33312) == 0) then
	-- --		L_UseItemByName("Mana Sapphire");
	-- 	--	return;
	-- 	end
	-- end

	if mana < 2500 then
		if GetSpellCooldown("Evocation") == 0 then
			L_CastSpellByName("Evocation");
		end
	end

	if not validate_target() then return end
	caster_range_check(36);

	--if lole_subcommands.get("aoemode") == 1 then
		--	lole_subcommands.cast_gtaoe("Flamestrike", get_unit_position("target"))
		--	return
	--end

--	local name, iconPath, tier, column, currentRank, maxRank = GetTalentInfo(2, 11); -- improved scorch

	local hs, ts = has_debuff("target", "Improved Scorch");

	if ((not hs) or (hs and ts < 8)) then
		L_CastSpellByName("Scorch");
		return;
	end

	if has_buff("player", "Hot Streak") then
		L_CastSpellByName("Pyroblast")
		return
	end


	for i,g in pairs({get_combat_targets()}) do
			target_unit_with_GUID(g)

			if UnitIsEnemy("target", "player") and not has_debuff("target", "Living Bomb") then
					L_CastSpellByName("Living Bomb")
					return;
			end
		end

	if not validate_target() then return end -- this is re-target the main nuke target

	L_CastSpellByName("Frostfire Bolt");

end
