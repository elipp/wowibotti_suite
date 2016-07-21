local tap_warning_given = false;
local ua_guard = true;
local immolate_guard = true;


local function tap_if_need_to()
	if UnitMana("player") < 2500 then
		if UnitHealth("player") > 3500 then
			CastSpellByName("Life Tap");
			return true;
		end
	else
		return false;
	end
end

local function vexallus()

	if tap_if_need_to() then return true; end

	caster_range_check(30);
	caster_face_target();

	TargetUnit("Pure Energy")

	if UnitExists("target") and UnitName("target") == "Pure Energy" and not UnitIsDead("target") then
		CastSpellByName("Searing Pain(Rank 2)")
		return true;
	else
		if UnitCastingInfo("player") then return; end
		if UnitChannelInfo("player") then return; end
		TargetUnit("Vexallus")

		CastSpellByName("Drain Life")

		return true;
	end

	return false;

end

local function MAULGAR()
	warlock_banish_felhound()
end

combat_warlock_affli = function()

	MAULGAR()

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	if not UnitAffectingCombat("player") then
		if (mana < 0.90*maxmana) then
			CastSpellByName("Life Tap");
			return;
		end
	end

	if not validate_target() then return end

	caster_range_check(30);

	if UnitCastingInfo("player") then return; end
	if UnitChannelInfo("player") then return; end
	if GetSpellCooldown("Corruption") > 0 then return; end -- check gcd. this could add unnecessary latency to spam though

	if lole_subcommands.get("aoemode") == 1 then
		for i=1,16,1 do
			TargetNearestEnemy();
			if (UnitExists("target") and not has_debuff_by_self("target", "Seed of Corruption")) then
				CastSpellByName("Seed of Corruption");
				return;
			end
		end
	end

	if (GetItemCount(6265) < 20) then -- 6265 -- soul shard
		if (UnitExists("target") and not UnitIsDead("target") and UnitHealth("target") < 20000) then
			SpellStopCasting();
			CastSpellByName("Drain Soul");
			return;
		end
	end

	if tap_if_need_to() then return true; end

	if not has_debuff("target", "Curse of the Elements") then
		CastSpellByName("Curse of the Elements");
		return;
	end

	if has_buff("player", "Nightfall") then
		CastSpellByName("Shadow Bolt")
		ua_guard = false;
		immolate_guard = false;
		return;
	elseif not has_debuff("target", "Unstable Affliction") and not ua_guard then
		CastSpellByName("Unstable Affliction");
		ua_guard = true;
		immolate_guard = false;
		return;
	elseif not has_debuff("target", "Corruption") then
		CastSpellByName("Corruption");
		ua_guard = false;
		immolate_guard = false;
		return;
	elseif not has_debuff("target", "Immolate") and not immolate_guard then
		CastSpellByName("Immolate");
		immolate_guard = true;
		ua_guard = false;
		return;
	else
		if (UnitHealthMax("player") - UnitHealth("player") > 2000) then
			CastSpellByName("Drain Life");
			return
		else
			CastSpellByName("Shadow Bolt");
		end
		ua_guard = false;
		immolate_guard = true;
		return;
	end

	tap_warning_given = false;

end
