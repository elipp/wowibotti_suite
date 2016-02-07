config_druid_resto_combat = function()

	TargetUnit("Josp"); 
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	local own_hp_max = UnitHealthMax("player");
	local own_hp_cur = UnitHealth("player");
	
	if GetSpellCooldown("Remove Curse") > 0 then -- dont waste cycles on GCD
		return;
	end
	
	if own_hp_cur < 5500 then 
		TargetUnit("player")
	end
	
	--if not has_buff("player", "Tree of Life") then
		--CastSpellByName("Tree of Life");
		--return;
	--end
	
	if has_debuff_of_type("target", "Curse") then
		CastSpellByName("Remove Curse");
		return;
	end
	
	if not has_buff("target", "Lifebloom") then
		CastSpellByName("Lifebloom");
		return;
	end
	
	if (health_cur < health_max * 0.88) then
		if not has_buff("target", "Rejuvenation") then
			CastSpellByName("Rejuvenation");
		end
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.80) then
		TargetUnit("player");
		if not has_buff("target", "Rejuvenation") then
			CastSpellByName("Rejuvenation");
		end
	end
	

end

config_druid_resto.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP = { 
            ["Mark of the Wild"] = "Gift of the Wild",
        };

        local buffs = {
            ["Mark of the Wild"] = MISSING_BUFFS_COPY["Mark of the Wild"],
            ["Thorns"] = MISSING_BUFFS_COPY["Thorns"],
        };
        
        local num_requests = get_num_buff_requests(buffs);
        
        if num_requests > 0 then
            SPAM_TABLE = get_spam_table(buffs, GROUP_BUFF_MAP);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_druid_resto.desired_buffs = function()

    local desired_buffs = get_desired_buffs("healer");
    return desired_buffs;

end

