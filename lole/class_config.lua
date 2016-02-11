function class_config_create(name, buffs, self_buffs, color, combatfunc, cooldown_spells, role)
	
	local config = {}
	config.name = name;
	config.buffs = buffs;
	config.self_buffs = self_buffs;
	config.color = color;
	config.combat = combatfunc;
	config.cooldowns = cooldown_spells;
	config.role = role;
	
	return config;	
end