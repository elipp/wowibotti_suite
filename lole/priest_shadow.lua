config_priest_shadow = {}
config_priest_shadow.name = "priest_shadow";

config_priest_shadow.MODE_ATTRIBS = {
    ["combatbuffmode"] = 0,
	["buffmode"] = 0,
    ["selfbuffmode"] = 0
};

local ve_guard = false;

config_priest_shadow.SELF_BUFFS = {"Inner Fire", "Shadowform"};


config_priest_shadow.combat = function()

	if UnitChannelInfo("player") then return; end	-- don't clip mind flay
	if UnitCastingInfo("player") then return; end  
	
--[[
	-- shackle berrybuck
	TargetUnit("Lady Keira Berrybuck");
	if UnitExists("target") then
		a, d = has_debuff("target", "Shackle Undead");
		if not a then
			CastSpellByName("Shackle Undead");
			return;
		elseif d < 10 then
			CastSpellByName("Shackle Undead");
		end
	end --]]

	if cleanse_party("Enveloping Wind") then return; end
	if cleanse_party("Lung Burst") then return; end
	
	if not UnitExists("target") or UnitIsDead("target") then 
		return;
	end
	
	caster_range_check(20); -- 20 yd on mind flay
	caster_face_target();

	if (UnitMana("player") < 4000 and UnitHealth("target") > 50000) then if cast_if_nocd("Shadowfiend") then return; end end
	
	-- this has a slight bug, the debuffs take a while (ie. too long, longer than your avg spaminterval delay) to actually show up in the list
	-- the ve_guard stuff along with the UnitCasting/ChannelInfo is to combat that
	if not has_debuff("target", "Vampiric Touch") and not ve_guard then 
		CastSpellByName("Vampiric Touch");
		ve_guard = true;
	elseif not has_debuff("target", "Shadow Word: Pain") then
		CastSpellByName("Shadow Word: Pain");
		ve_guard = false;
	elseif GetSpellCooldown("Mind Blast") == 0 then
		CastSpellByName("Mind Blast");
		ve_guard = false;
	--elseif GetSpellCooldown("Shadow Word: Death") == 0 then CastSpellByName("Shadow Word: Death"); 
	else
		CastSpellByName("Mind Flay");
		ve_guard = false;
	end
	

end

config_priest_shadow.cooldowns = function()
	UseInventoryItem(13);
	UseInventoryItem(14);
end

config_priest_shadow.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP = { 
            ["Power Word: Fortitude"] = "Prayer of Fortitude",
            ["Shadow Protection"] = "Prayer of Shadow Protection"
        };

        local buffs = {
            ["Power Word: Fortitude"] = MISSING_BUFFS_COPY["Power Word: Fortitude"],
            ["Shadow Protection"] = MISSING_BUFFS_COPY["Shadow Protection"],
        };

        local num_requests = get_num_buff_requests(buffs);
        
        if num_requests > 0 then
            SPAM_TABLE = get_spam_table(buffs, GROUP_BUFF_MAP);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_priest_shadow.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_priest_shadow.other = function()

end
