
local TOTEMS = {
--["air"] = "Windfury Totem",
["air"] = "Wrath of Air Totem",

["earth"] = "Tremor Totem",
--["earth"] = "Strength of Earth Totem",
--["earth"] = "Stoneskin Totem",

["water"] = "Mana Spring Totem",

["fire"] = "Totem of Wrath"
}


config_shaman_elem_combat = function()

	if UnitCastingInfo("player") then return; end

	if not has_buff("player", "Water Shield") then 
		CastSpellByName("Water Shield");
		return;
	end
	
	if refresh_totems(TOTEMS) then return; end

	
	caster_range_check(30); 
	caster_face_target();

	CastSpellByName("Lightning Bolt");
	
end


config_shaman_elem.buffs = function()

    if SELF_BUFF_SPAM_TABLE[1] == nil then
        lole_subcommands.set("buffmode", 0)
    else
        buff_self();
    end

end

config_shaman_elem.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

