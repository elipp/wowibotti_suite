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

    hunter =
        class_config:create("hunter", {}, {}, get_class_color("hunter"), combat_hunter,
            { "Bestial Wrath", "Rapid Fire", "Call of the Wild" }, ROLES.mana_melee, "RANGED", survive_hunter),

    paladin_prot =
        class_config:create("paladin_prot", {}, { "Devotion Aura", "Righteous Fury", "Seal of Command" },
            get_class_color("paladin"), combat_paladin_prot, {}, ROLES.mana_tank, "TANK", survive_paladin_prot),

    priest_holy =
        class_config:create("priest_holy", { "Power Word: Fortitude", "Divine Spirit", "Shadow Protection" },
            { "Inner Fire" }, get_class_color("priest"), combat_priest_holy, { "Inner Focus" }, ROLES.healer, "HEALER",
            survive_priest_holy),

    rogue =
        class_config:create("rogue", {}, {}, get_class_color("rogue"), rogue_combat,
            { "Adrenaline Rush", "Blade Flurry", "Killing Spree" }, ROLES.melee, "MELEE", survive_rogue),

    shaman_resto =
        class_config:create("shaman_resto", {}, { "Water Shield" }, get_class_color("shaman"), combat_shaman_resto,
            { "Bloodlust" }, ROLES.healer, "HEALER", survive_shaman_resto),

    warrior_prot =
        class_config:create("warrior_prot", {}, { "Commanding Shout" }, get_class_color("warrior"), combat_warrior_prot,
            {}, ROLES.tank, "TANK", survive_warrior_prot),

    ranged_hunter = class_config:create("ranged_hunter", {}, {}, get_class_color("hunter"), combat_ranged_hunter,
        { "Bestial Wrath" }, ROLES.caster, "RANGED", survive_template),

    enchantement_shaman = class_config:create("shaman_encha", {}, {}, get_class_color("shaman"), combat_shaman_encha, {},
        ROLES.mana_melee, "MELEE", hunter_survive),

    tmp_warlock = class_config:create("tmp_warlock", {}, {}, get_class_color("warlock"), tmp_warlock_combat, {},
        ROLES.caster, "RANGED", survive_template),
    tmp_priest = class_config:create("tmp_priest", { "Power Word: Fortitude" }, {}, get_class_color("priest"),
        tmp_priest_combat, {}, ROLES.caster, "RANGED", survive_template),
    tmp_mage = class_config:create("tmp_mage", {}, {}, get_class_color("mage"), tmp_mage_combat, {}, ROLES.caster,
        "RANGED", survive_template),
    -- tmp_paladin = class_config:create("tmp_paladin", {"Blessing of Wisdom"}, {}, get_class_color("paladin"), tmp_paladin_combat, {}, ROLES.healer, "HEALER", survive_template),
    tmp_paladin = class_config:create("tmp_paladin", {}, {}, get_class_color("paladin"), tmp_paladin_combat, { "Avenging Wrath" },
        ROLES.mana_melee, "MELEE", survive_template),

    aoe_druid_balance =
        class_config:create("aoe_druid_balance", {}, {}, get_class_color("druid"), aoe_combat_druid, {}, ROLES.caster,
            "RANGED", survive_template),

    aoe_warlock_destruction =
        class_config:create("aoe_warlock_destruction", {}, {}, get_class_color("warlock"), aoe_combat_warlock, {},
            ROLES.caster, "RANGED", survive_template),

    aoe_mage_frost =
        class_config:create("aoe_mage_frost", {}, {}, get_class_color("mage"), aoe_combat_mage, {}, ROLES.caster,
            "RANGED", survive_template),
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
    echo(c.combat);
    if c then
        LOLE_CLASS_CONFIG = configname
    end
end
