config_paladin_holy = {}
config_paladin_holy.name = "paladin_holy";

config_paladin_holy.MODE_ATTRIBS = {
	["buffmode"] = 0
};

config_paladin_holy.SELF_BUFFS = {"Concentration Aura"}

config_paladin_holy.combat = function()

	TargetUnit("focus");
	local mana_left = UnitMana("player");

	if mana_left < 6000 then
		CastSpellByName("Divine Illumination");
		CastSpellByName("Divine Favor");
	end

	if UnitHealth("player") < 2000 then
		if (GetSpellCooldown("Divine Shield") == 0) then
			SpellStopCasting();
			CastSpellByName("Divine Shield");
		end
	end
	
	if UnitCastingInfo("player") then return; end;
		
	local health_max = UnitHealthMax("target");
	local health_cur = UnitHealth("target");
	
	if has_debuff("target", "Shadow Word: Pain") then
		CastSpellByName("Cleanse");
	end
	
	if (health_cur < health_max * 0.60) then
		CastSpellByName("Holy Light");
	elseif (health_cur < health_max * 0.88) then
		CastSpellByName("Flash of Light");
	elseif (UnitHealth("player") < UnitHealthMax("player")*0.65) then
		TargetUnit("player");
		CastSpellByName("Flash of Light");
	end
	

end

config_paladin_holy.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local buffs = {
            ["Greater Blessing of Salvation"] = MISSING_BUFFS_COPY["Blessing of Salvation"],
            ["Greater Blessing of Wisdom"] = MISSING_BUFFS_COPY["Blessing of Wisdom"],
            ["Greater Blessing of Might"] = MISSING_BUFFS_COPY["Blessing of Might"]
        };

        local num_paladins = get_num_paladins();
        
        if num_paladins < 2 then
            buffs["Greater Blessing of Kings"] = MISSING_BUFFS_COPY["Blessing of Kings"];
        end

		local num_requests = get_num_buff_requests(buffs);		

        if num_requests > 0 then
            SPAM_TABLE = get_paladin_spam_table(buffs, num_requests);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_paladin_holy.desired_buffs = function()

    local desired_buffs = get_desired_buffs("healer");
    return desired_buffs;

end

config_paladin_holy.other = function()

end;
