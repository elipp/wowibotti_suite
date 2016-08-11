function class_config_create(name, buffs, self_buffs, color, combatfunc, cooldown_spells, role, general_role)

	local config = {}
	config.name = name;
	config.buffs = buffs;
	config.self_buffs = self_buffs;
	config.color = color;
	config.combat = combatfunc;
	config.cooldowns = cooldown_spells;
	config.role = role;
	config.general_role = general_role;

	return config;
end
