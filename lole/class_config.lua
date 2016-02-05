local function cooldowns()
	UseInventoryItem(13);
	UseInventoryItem(14);
end


local function class_config_create(name, mode_attribs, self_buffs, color, combatfunc, cooldown_spells, buff_pref)
	local config = {}
	config.name = name;
	config.mode_attribs = mode_attribs;
	config.self_buffs = self_buffs;
	config.color = color;
	config.combat = combatfunc;
	config.cooldowns = cooldown_spells;

	return config;	
end