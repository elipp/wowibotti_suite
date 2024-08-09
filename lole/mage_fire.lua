local spellsteal_lock = 0
local scorch_lock = 0

local function cleanse_mage()
	local debuffs = get_raid_debuffs_by_type("Curse")
	return cast_dispel(debuffs, "Remove Curse")
end

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

	if lole_get("dispelmode") == 1 then
		if cleanse_mage() then return end
	end

	if not validate_target() then return end

	caster_range_check(0, 36);

	if unit_castorchannel("target") then
		L_CastSpellByName("Counterspell")
	end

	if lole_subcommands.get("aoemode") == 1 and get_aoe_feasibility("target", 15) > 3 then
		cast_gtaoe("Flamestrike", get_unit_position("target"))
		return
	end

	-- FOR JARAXXUS :D
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
		if GetTime() - scorch_lock > 2.3 then
			L_CastSpellByName("Scorch");
			scorch_lock = GetTime()
			return;
		end
	end

	if has_buff("player", "Hot Streak") then
		L_CastSpellByName("Pyroblast")
		return
	end

	if lole_get("aoemode") == 1 and get_aoe_feasibility("target", 15) > 3 then
		for i, g in pairs({ get_combat_targets() }) do
			target_unit_with_GUID(g)

			if UnitIsEnemy("target", "player") and not has_debuff("target", "Living Bomb") then
				L_CastSpellByName("Living Bomb")
				return;
			end
		end
	end

	if not validate_target() then return end -- this is to re-target the main nuke target

	if UnitHealth("target") > 50000 and not has_debuff("target", "Living Bomb") then
		L_CastSpellByName("Living Bomb")
		return;
	end

	L_CastSpellByName("Frostfire Bolt");
end
