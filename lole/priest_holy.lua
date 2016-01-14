config_priest_holy = {}
config_priest_holy.name = "priest_holy";

config_priest_holy.MODE_ATTRIBS = {
    ["combatbuffmode"] = 0,
	["buffmode"] = 0,
    ["selfbuffmode"] = 0,
    ["playermode"] = 0
};

config_priest_holy.SELF_BUFFS = {"Inner Fire"};

local pom_time = 0;

config_priest_holy.combat = function()

	if UnitCastingInfo("player") then return; end;
	
	TargetUnit("focus");
	local mana_left = UnitMana("player");

	if mana_left < 3000 then
		CastSpellByName("Shadowfiend");
		return;
	end

		
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	if time() - pom_time > 20 then
		if cast_if_nocd("Prayer of Mending") then 
			pom_time = time();
			return; 
		end
	end
	
	if not has_buff("target", "Renew") then
		CastSpellByName("Renew");
		return;
	end
	
	if casting_legit_heal() then return end
	
	if (health_cur < health_max * 0.60) then
		cast_spell("Greater Heal");
	elseif (health_cur < health_max * 0.88) then
		cast_spell("Greater Heal(Rank 4)");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.65) then
		cast_spell("Binding Heal");
	end
	
end

config_priest_holy.cooldowns = function()

end

config_priest_holy.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP = { 
            ["Power Word: Fortitude"] = "Prayer of Fortitude",
            ["Divine Spirit"] = "Prayer of Spirit",
            ["Shadow Protection"] = "Prayer of Shadow Protection"
        };

        local buffs = {
            ["Power Word: Fortitude"] = MISSING_BUFFS_COPY["Power Word: Fortitude"],
            ["Divine Spirit"] = MISSING_BUFFS_COPY["Divine Spirit"],
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

config_priest_holy.desired_buffs = function()

    local desired_buffs = get_desired_buffs("healer");
    return desired_buffs;

end

config_priest_holy.other = function()

end
