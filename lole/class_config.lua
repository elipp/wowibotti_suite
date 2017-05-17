function class_config_create(name, buffs, self_buffs, color, combatfunc, cooldown_spells, role, general_role, survivefunc)

	local config = {}
	config.name = name;
	config.buffs = buffs;
	config.self_buffs = self_buffs;
	config.color = color;
	config.combat = combatfunc;
	config.cooldowns = cooldown_spells;
	config.role = role;
	config.general_role = general_role;
    config.survive = survivefunc;

	return config;
end

survive_template = function() end
survive_death_knight_uh = survive_template;
survive_druid_balance = survive_template;
survive_druid_feral = survive_template;
survive_druid_resto = survive_template;
survive_hunter = survive_template;
survive_mage_fire = survive_template;
survive_mage_frost = survive_template;
survive_paladin_holy = survive_template;
survive_paladin_prot = survive_template;
survive_paladin_retri = survive_template;
survive_priest_holy = survive_template;
survive_priest_shadow = survive_template;
survive_rogue_combat = survive_template;
survive_shaman_elem = survive_template;
survive_shaman_enh = survive_template;
survive_shaman_resto = survive_template;
survive_warrior_arms = survive_template;
survive_warrior_fury = survive_template;
survive_warrior_prot = survive_template;

survive_warlock_affli = function()
    cast_if_nocd("Soulshatter");
end
survive_warlock_sb = survive_warlock_affli;