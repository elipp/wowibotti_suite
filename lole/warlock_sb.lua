config_warlock_sb = {}
config_warlock_sb.name = "warlock_sb";

config_warlock_sb.MODE_ATTRIBS = {
    ["buffmode"] = 0,
	["aoemode"] = 0,
	["shardmode"] = 0
};

local tap_warning_given = false;

config_warlock_sb.SELF_BUFFS = {"Fel Armor"};
-- TODO: fel domination->summon succubus->demonic sacrifice


config_warlock_sb.combat = function()

	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");
	
	if not UnitAffectingCombat("player") then
		if (mana < 0.90*maxmana) then
			CastSpellByName("Life Tap");
		end
	end

	if UnitCastingInfo("player") then return; end
	if UnitChannelInfo("player") then return; end

	if UnitMana("player") < 2500 then
		if UnitHealth("player") > 3500 then
			CastSpellByName("Life Tap");
			return;
		elseif not tap_warning_given then
			SendChatMessage("FUCK! Running LOW ON MANA, but too low HP to safely fap. Healz plx!", "YELL");
			tap_warning_given = true;
			return;
		end
	
	end
	
	if config_warlock_sb.MODE_ATTRIBS["aoemode"] == 1 then			
		for i=1,16,1 do 
			TargetNearestEnemy();
			if (UnitExists("target") and not has_debuff_by_self("target", "Seed of Corruption")) then
				CastSpellByName("Seed of Corruption");
				break;
			end
		end
		return;
	elseif config_warlock_sb.MODE_ATTRIBS["shardmode"] == 1 then
	    if (UnitExists("target") and not UnitIsDead("target") and UnitHealth("target") < 20000) then
			--SpellStopCasting();
			CastSpellByName("Drain Soul");
			return;
		end
	end

	if not has_debuff("target", "Curse of the Elements") then
		CastSpellByName("Curse of the Elements")
		return;
	end

	CastSpellByName("Shadow Bolt");

	tap_warning_given = false;


end

config_warlock_sb.cooldowns = function() 

	UseInventoryItem(13);
	UseInventoryItem(14);

end

config_warlock_sb.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
        config_warlock_sb.MODE_ATTRIBS["buffmode"] = 0;
        echo("lole_set: attrib \"buffmode\" set to 0");
    else
        buff_self();
    end

end

config_warlock_sb.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_warlock_sb.other = function()

end;
