local tap_warning_given = false;
local ua_guard = true;
local immolate_guard = true;

config_warlock_affli_combat = function()
	
	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");
	
	if not UnitAffectingCombat("player") then
		if (mana < 0.90*maxmana) then
			CastSpellByName("Life Tap");
			return;
		end
	end
	
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
	-- REPLACE THIS WITH IF LESS THAN N SHARDS, SOUL DRAIN!
	-- --elseif  == 1 then
	    -- if (UnitExists("target") and not UnitIsDead("target") and UnitHealth("target") < 20000) then
			-- --SpellStopCasting();
			-- CastSpellByName("Drain Soul");
			-- return;
		-- end
	-- end
	
	if mana < 2500 then
		if UnitHealth("player") > 3500 then
			CastSpellByName("Life Tap");
			return;
		elseif not tap_warning_given then
			SendChatMessage("FUCK! Running LOW ON MANA, but too low HP to safely fap. Healz plx!", "YELL");
			tap_warning_given = true;
			return;
		end
	
	end
	
	local t = target_mob_with_charm("skull");
	if (t < 1) then return; end
	
	
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
		CastSpellByName("Shadow Bolt");
		ua_guard = false;
		immolate_guard = true;
		return;
	end
	
	tap_warning_given = false;
	
end

local BUFF_TABLE_READY = true;

config_warlock_affli.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
		lole_subcommands.set("buffmode", 0);
    else
        buff_self();
    end

end

config_warlock_affli.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

