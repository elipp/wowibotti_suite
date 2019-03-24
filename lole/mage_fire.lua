local spellsteal_lock = 0

combat_mage_fire = function()

	-- L_TargetUnit("Ghostly Priest")
	-- if UnitExists("target") then
	-- 	if UnitCastingInfo("target") and string.find(UnitCastingInfo("target"), "Fear") and GetSpellCooldown("Counterspell") == 0 then
	-- 		L_SpellStopCasting()
	-- 		L_CastSpellByName("Counterspell")
	-- 	end
	-- end

	if player_casting() then return end

	if ((GetItemCount(33312) == 0) and (not UnitAffectingCombat("player"))) then
		L_CastSpellByName("Conjure Mana Gem");
		return;
	end

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	if mana < (maxmana - 2500) then
		if (GetItemCount(33312) > 0) and (GetItemCooldown(33312) == 0) then
			L_UseItemByName("Mana Sapphire");
			return;
		end
	end

	if mana < 2500 then
		if GetSpellCooldown("Evocation") == 0 then
			L_CastSpellByName("Evocation");
		end
	end

	if cleanse_raid("Curse of the Plaguebringer") then return end

	if not validate_target() then return end
	caster_range_check(0,36);

	if lole_subcommands.get("aoemode") == 1 and get_aoe_feasibility(15) > 3 then
			lole_subcommands.cast_gtaoe("Flamestrike", get_unit_position("target"))
			return
	end

	if has_buff("target", "Nether Power") then
		local t = GetTime()
		if (t - spellsteal_lock > 12) then
			L_CastSpellByName("Spellsteal")
			spellsteal_lock = t
			return
		end
	end

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

	if not validate_target() then return end -- this is to re-target the main nuke target

	L_CastSpellByName("Frostfire Bolt");

end
