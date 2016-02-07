
config_mage_frost_combat = function()
	
	caster_range_check(36); 
	caster_face_target();
	
	if UnitCastingInfo("player") then return; end
	CastSpellByName("Frostbolt");

end

config_mage_frost.cooldowns = function() 
	if not has_buff("player", "Icy Veins") then
		cast_if_nocd("Icy Veins");
	end

	cast_if_nocd("Arcane Power");
	
	if GetSpellCooldown("Icy Veins") > 0 then
		cast_if_nocd("Cold Snap");
	end

end


config_mage_frost.buffs = function(MISSING_BUFFS_COPY)

end

config_mage_frost.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end
