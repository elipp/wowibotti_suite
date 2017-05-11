local function tap_if_need_to(threshold)
	if UnitMana("player") < threshold then -- higher threshold for affli since it's being done only once all the dots are in place
		if UnitHealth("player") > 1450 then
			L_CastSpellByName("Life Tap");
			return true;
		end
	else
		return false;
	end
end

local function vexallus()

	if tap_if_need_to() then return true; end

	caster_range_check(30);

	L_TargetUnit("Pure Energy")

	if UnitExists("target") and UnitName("target") == "Pure Energy" and not UnitIsDead("target") then
		L_CastSpellByName("Searing Pain(Rank 2)")
		return true;
	else
		if UnitCastingInfo("player") then return; end
		if UnitChannelInfo("player") then return; end
		L_TargetUnit("Vexallus")

		L_CastSpellByName("Drain Life")

		return true;
	end

	return false;

end

local function MAULGAR()
	warlock_banish_felhound()
end

local function enough_shards()
	if (GetItemCount(6265) < 20) then
		return false
	else
		return true
	end-- 6265 -- soul shard
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
			L_CastSpellByName("Drain Soul");
			return true;
		end
	end

	return false
end


local LAST_SPELL_CAST = ""

local cast_frame = CreateFrame("Frame")
cast_frame:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)

	if event == "UNIT_SPELLCAST_SUCCEEDED" then
		LAST_SPELL_CAST = message
		--LAST_SPELL_RESIST = ""
	end
	-- elseif event == "COMBAT_LOG_EVENT_UNFILTERED" then
	-- 	if message == "SPELL_MISSED" then
	-- 		LAST_SPELL_CAST = ""
	-- 		LAST_SPELL_RESIST = arhhh -- arhhh is SIX arguments after sender
	-- 	--	echo(tostring(arhhh))
	-- 	end
	-- end
end)

cast_frame:RegisterEvent("UNIT_SPELLCAST_SUCCEEDED")
cast_frame:RegisterEvent("COMBAT_LOG_EVENT_UNFILTERED")

combat_warlock_affli = function()

--	MAULGAR()
	--vexallus()
	--if true then return end


	local S = player_casting()
	if S and S == LAST_SPELL_CAST and S ~= "Shadow Bolt" and S ~= "Drain Life" then SpellStopCasting() end

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");

	if not UnitAffectingCombat("player") then
		if (mana < 0.90*maxmana) then
			L_CastSpellByName("Life Tap");
			return;
		end
	end

	if not validate_target() then return end
	caster_range_check(30);

	if tap_if_need_to(2000) then return end

	if lole_subcommands.get("aoemode") == 1 then
		for i=1,16,1 do
			TargetNearestEnemy();
			if (UnitExists("target") and not has_debuff_by_self("target", "Seed of Corruption")) then
				L_CastSpellByName("Seed of Corruption");
				return;
			end
		end
	end

	if drain_soul_if_needed() then return end

	if not has_debuff("target", "Curse of the Elements") then
		L_CastSpellByName("Curse of the Elements");
		return;

	elseif not has_debuff_by_self("target", "Unstable Affliction") then
		L_CastSpellByName("Unstable Affliction");
		return;

	elseif not has_debuff_by_self("target", "Siphon Life") then
		L_CastSpellByName("Siphon Life")
		return;

	elseif not has_debuff_by_self("target", "Corruption") then
		L_CastSpellByName("Corruption");
		return;

	elseif not has_debuff_by_self("target", "Immolate") then
		L_CastSpellByName("Immolate");
		return;

	elseif has_buff("player", "Nightfall") then
		L_CastSpellByName("Shadow Bolt")
		return;

	elseif tap_if_need_to(5000) then
		return;

	elseif UnitHealth("player") < 4000 then
		L_CastSpellByName("Drain Life");
		return
	else
		L_CastSpellByName("Shadow Bolt");
	end

end
