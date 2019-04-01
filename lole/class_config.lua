class_config = {}
class_config.__index = class_config

function class_config:create(name, buffs, self_buffs, color, combatfunc, cooldown_spells, role, general_role, survivefunc)

	local c = {}
	setmetatable(c, class_config)

	c.name = name;
	c.buffs = buffs;
	c.self_buffs = self_buffs;
	c.color = color;
	c.combat = combatfunc;
	c.cooldowns = cooldown_spells;
	c.role = role;
	c.general_role = general_role;
  c.survive = survivefunc;

	return c;

end

survive_template = function() end
survive_death_knight_blood = survive_template;
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

local LOLE_CLASS_CONFIG = "default";

local available_configs = {
	default =
	class_config:create("default", {}, {}, "FFFFFF", function() end, {}, 0, "NONE", function() end),

  death_knight_blood =
  class_config:create("death_knight_blood", {}, {"Blood Presence", "Horn of Winter"}, get_class_color("death_knight"), combat_death_knight_blood, {"Hysteria", "Army of the Dead"}, ROLES.melee, "MELEE", survive_death_knight_blood),

  death_knight_uh =
  class_config:create("death_knight_uh", {}, {"Blood Presence", "Bone Shield", "Horn of Winter"}, get_class_color("death_knight"), combat_death_knight_uh, {"Summon Gargoyle", "Army of the Dead"}, ROLES.melee, "MELEE", survive_death_knight_uh),

	druid_resto =
	class_config:create("druid_resto", {"Mark of the Wild", "Thorns"}, {"Tree of Life"}, get_class_color("druid"), combat_druid_resto, {}, ROLES.healer, "HEALER", survive_druid_resto),

	hunter =
	class_config:create("hunter", {}, {}, get_class_color("hunter"), combat_hunter, {"Bestial Wrath", "Rapid Fire", "Call of the Wild"}, ROLES.mana_melee, "RANGED", survive_hunter),

	mage_fire =
	class_config:create("mage_fire", {"Arcane Intellect", "Amplify Magic"}, {"Molten Armor"}, get_class_color("mage"), combat_mage_fire, {"Icy Veins", "Combustion", "Mirror Image"}, ROLES.caster, "RANGED", survive_mage_fire),

	paladin_prot =
	class_config:create("paladin_prot", {}, {"Devotion Aura", "Righteous Fury", "Seal of Command"}, get_class_color("paladin"), combat_paladin_prot, {}, ROLES.paladin_tank, "TANK", survive_paladin_prot),

	paladin_holy =
	class_config:create("paladin_holy", {}, {"Concentration Aura"}, get_class_color("paladin"), combat_paladin_holy, {"Divine Favor", "Divine Illumination"}, ROLES.healer, "HEALER", survive_paladin_holy),

	paladin_retri =
	class_config:create("paladin_retri", {}, {"Sanctity Aura"}, get_class_color("paladin"), combat_paladin_retri, {"Avenging Wrath"}, ROLES.mana_melee, "MELEE", survive_paladin_retri),

	priest_holy =
	class_config:create("priest_holy", {"Power Word: Fortitude", "Divine Spirit", "Shadow Protection"}, {"Inner Fire"}, get_class_color("priest"), combat_priest_holy, {"Inner Focus"}, ROLES.healer, "HEALER", survive_priest_holy),

	priest_shadow =
	class_config:create("priest_shadow", {"Power Word: Fortitude", "Divine Spirit", "Shadow Protection"}, {"Shadowform", "Inner Fire"}, get_class_color("priest"), combat_priest_shadow, {"Inner Focus"}, ROLES.caster, "RANGED", survive_priest_shadow),

	rogue =
	class_config:create("rogue", {}, {}, get_class_color("rogue"), rogue_combat, {"Adrenaline Rush", "Blade Flurry"}, ROLES.melee, "MELEE", survive_rogue),

	shaman_resto =
	class_config:create("shaman_resto", {}, {"Water Shield"}, get_class_color("shaman"), combat_shaman_resto, {"Bloodlust"}, ROLES.healer, "HEALER", survive_shaman_resto),

  warlock_demo =
  class_config:create("warlock_demo", {}, {"Fel Armor"}, get_class_color("warlock"), combat_warlock_demo, {"Metamorphosis"}, ROLES.caster, "RANGED", survive_warlock_demo),

	warrior_fury =
	class_config:create("warrior_fury", {}, {"Battle Shout"}, get_class_color("warrior"), combat_warrior_fury, {"Death Wish", "Recklessness"}, ROLES.melee, "MELEE", survive_warrior_fury),

	warrior_prot =
	class_config:create("warrior_prot", {}, {"Commanding Shout"}, get_class_color("warrior"), combat_warrior_prot, {}, ROLES.warrior_tank, "TANK", survive_warrior_prot),

};

function get_available_configs()
	return available_configs;
end

function get_current_config()
	return available_configs[LOLE_CLASS_CONFIG]
end

function get_config(configname)
	return available_configs[configname]
end

function set_config(configname)
	local c = available_configs[configname]
	if c then
		LOLE_CLASS_CONFIG = configname
	end
end
