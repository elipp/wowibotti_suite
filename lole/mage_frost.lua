config_mage_frost = {}
config_mage_frost.name = "mage_frost";

config_mage_frost.role = ROLES.CASTER;


config_mage_frost.MODE_ATTRIBS = { ["playermode"] = 0 };

config_mage_frost.SELF_BUFFS = {"Molten Armor"};
config_mage_frost.COLOR = CLASS_COLORS["mage"];

config_mage_frost.combat = function()
	if UnitCastingInfo("player") then return; end
	local t = target_mob_with_charm("skull");
	if (t < 1) then return; end

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

config_mage_frost.other = function()

end;
