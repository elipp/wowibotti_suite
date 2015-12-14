config_shaman_elem = {}
config_shaman_elem.name = "shaman_elem";

config_shaman_elem.MODE_ATTRIBS = {
    ["buffmode"] = 0
};

config_shaman_elem.SELF_BUFFS = {"Water Shield"};


config_shaman_elem.combat = function()

	if UnitCastingInfo("player") then return; end

	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	if recast_totem_if_not_active_or_in_range("Tremor Totem") then return; end
	if recast_totem_if_not_active_or_in_range("Totem of Wrath") then return; end
	if recast_totem_if_not_active_or_in_range("Wrath of Air Totem") then return; end
	if recast_totem_if_not_active_or_in_range("Mana Spring Totem") then return; end
	
	local t = target_mob_with_charm("skull");
	if (t < 1) then return; end

	CastSpellByName("Lightning Bolt");
	
end

config_shaman_elem.cooldowns = function() 

	UseInventoryItem(13);
	UseInventoryItem(14);
	
	cast_if_nocd("Elemental Mastery");
	cast_if_nocd("Blood Fury");
	cast_if_nocd("Bloodlust");
	
end

config_shaman_elem.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
        config_shaman_elem.MODE_ATTRIBS["buffmode"] = 0;
        echo("lole_set: attrib \"buffmode\" set to 0");
    else
        buff_self();
    end

end

config_shaman_elem.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_shaman_elem.other = function()

end;
