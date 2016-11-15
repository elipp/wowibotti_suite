-- TODO: fel domination->summon succubus->demonic sacrifice

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

local curse_assignments = {
	["Nilck"] = "Curse of Weakness",
	["Mulck"] = "Curse of Doom",
	["Jobim"] = "Curse of the Elements",
}

local function enough_shards()
	if (GetItemCount(6265) < 20) then
		return false
	else
		return true
	end-- 6265 -- soul shard
end

local function cast_assigned_curse()
	for name,curse in pairs(curse_assignments) do
		if name == UnitName("player") and UnitExists("target") then
			if not has_debuff_by_self("target", curse) then
				if cast_if_nocd(curse) then return true end
			end
		end
	end

	return false
end

local function drain_soul_if_needed()
	if not enough_shards() then
		local HP_threshold = 10000 -- for 5-man groups
		if in_raid_group() then
			if GetNumRaidMembers() > 10 then
				HP_threshold = 40000
			else
				HP_threshold = 20000
			end
		end

		if (UnitHealth("target") < HP_threshold) then
			CastSpellByName("Drain Soul");
			return true;
		end
	end

	return false
end

local time_from_succubus_summon = 0

combat_warlock_sb = function()
	if player_casting() then return end

	--MAULGAR()

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	if not UnitAffectingCombat("player") then
		if (mana < 0.90*maxmana) then
			CastSpellByName("Life Tap");
		end
	end

	if not has_buff("player", "Touch of Shadow") then
		CastSpellByName("Fel Domination")
		if (time() - time_from_succubus_summon) > 8 then
			CastSpellByName("Summon Succubus")
			time_from_succubus = time()
		end
		CastSpellByName("Demonic Sacrifice")
		return
	end

	if not validate_target() then return end

	caster_range_check(30);

	if tap_if_need_to() then return; end

	if lole_subcommands.get("aoemode") == 1 then
		local eligible_seed_target = get_seed_target();
		if eligible_seed_target then
			target_unit_with_GUID(eligible_seed_target)
			CastSpellByName("Seed of Corruption")
			return
		end
	end

	if drain_soul_if_needed() then return end
	if cast_assigned_curse() then return end

	CastSpellByName("Shadow Bolt");


end
