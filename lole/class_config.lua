ClassConfig = {}
ClassConfig.__index = ClassConfig

function ClassConfig:GlobalCooldown() 
    return self.gcd_spell and GetSpellCooldown(self.gcd_spell) == 0
end

function ClassConfig:create(name, buffs, self_buffs, color, combatfunc, cooldown_spells, role, general_role, survivefunc, spellerror_handlers, gcd_spell)
    local c = {}
    setmetatable(c, ClassConfig)

    c.name = name
    c.buffs = buffs or {}
    c.self_buffs = self_buffs or {}
    c.color = color
    c.combat = combatfunc
    c.cooldowns = cooldown_spells or {}
    c.role = role
    c.general_role = general_role
    c.survive = survivefunc
    c.spellerror_handlers = spellerror_handlers or {}
    c.gcd_spell = gcd_spell

    return c
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

local AUTO_FACING_HANDLERS = {
    [SpellError.YouAreFacingTheWrongWay] = function() face_mob() end,
    [SpellError.TargetNeedsToBeInFrontOfYou] = function() face_mob() end,
}

function table_concat(t1, t2)
    local res = {}
    for k,v in pairs(t1) do
        res[k] = v
    end
    for k, v in pairs(t2) do
        res[k] = v
    end
    return res
end

local available_configs = {
    default =
        ClassConfig:create("default", {}, {}, "FFFFFF", function() end, {}, 0, "NONE", function() end),

    hunter =
        ClassConfig:create("hunter", {}, {}, get_class_color("hunter"), combat_hunter,
            { "Bestial Wrath", "Rapid Fire", "Call of the Wild" }, ROLES.mana_melee, "RANGED", survive_hunter, "Hunter's Mark"),

    paladin_prot =
        ClassConfig:create("paladin_prot", {}, { "Devotion Aura", "Righteous Fury", "Seal of Command" },
            get_class_color("paladin"), combat_paladin_prot, {}, ROLES.mana_tank, "TANK", survive_paladin_prot, "Devotion Aura"),

    priest_holy =
        ClassConfig:create("priest_holy", { "Power Word: Fortitude", "Divine Spirit", "Shadow Protection" },
            { "Inner Fire" }, get_class_color("priest"), combat_priest_holy, { "Inner Focus" }, ROLES.healer, "HEALER",
            survive_priest_holy, "Flash Heal"),

    rogue =
        ClassConfig:create("rogue", {}, {}, get_class_color("rogue"), rogue_combat,
            { "Adrenaline Rush", "Blade Flurry", "Killing Spree" }, ROLES.melee, "MELEE", survive_rogue, AUTO_FACING_HANDLERS, "Sinister Strike"),

    shaman_resto =
        ClassConfig:create("shaman_resto", {}, { "Water Shield" }, get_class_color("shaman"), combat_shaman_resto,
            { "Bloodlust" }, ROLES.healer, "HEALER", survive_shaman_resto, "Healing Wave"),

    warrior_prot =
        ClassConfig:create("warrior_prot", {}, { "Commanding Shout" }, get_class_color("warrior"), combat_warrior_prot,
            {}, ROLES.tank, "TANK", survive_warrior_prot, "Battle Shout"),

    ranged_hunter = ClassConfig:create("ranged_hunter", {}, {}, get_class_color("hunter"), combat_ranged_hunter,
        { "Bestial Wrath" }, ROLES.caster, "RANGED", survive_template,
        table_concat(AUTO_FACING_HANDLERS, {
            [SpellError.TargetTooClose] = function()
                -- -- this gets confused with ranged_check...
                -- face_mob()
                -- L_MoveBackwardStart()
                -- setTimeout(function() L_MoveBackwardStop() end, 1000)
            end
        }),
        "Hunter's Mark"
    ),

    enchantement_shaman = ClassConfig:create("shaman_encha", {}, {}, get_class_color("shaman"), combat_shaman_encha, {},
        ROLES.mana_melee, "MELEE", hunter_survive, "Healing Wave"),

    tmp_warlock = ClassConfig:create("tmp_warlock", {}, {}, get_class_color("warlock"), tmp_warlock_combat, {},
        ROLES.caster, "RANGED", survive_template, "Shadow Bolt"),
    tmp_priest = ClassConfig:create("tmp_priest", { "Power Word: Fortitude" }, {}, get_class_color("priest"),
        tmp_priest_combat, {}, ROLES.caster, "RANGED", survive_template, "Flash Heal"),
    tmp_mage = ClassConfig:create("tmp_mage", {}, {}, get_class_color("mage"), tmp_mage_combat, {}, ROLES.caster,
        "RANGED", survive_template, "Frostbolt"),
    -- tmp_paladin = class_config:create("tmp_paladin", {"Blessing of Wisdom"}, {}, get_class_color("paladin"), tmp_paladin_combat, {}, ROLES.healer, "HEALER", survive_template),
    tmp_paladin = ClassConfig:create("tmp_paladin", {}, {}, get_class_color("paladin"), tmp_paladin_combat, { "Avenging Wrath" },
        ROLES.mana_melee, "MELEE", survive_template, AUTO_FACING_HANDLERS, "Devotion Aura"),

    aoe_druid_balance =
        ClassConfig:create("aoe_druid_balance", {}, {}, get_class_color("druid"), aoe_combat_druid, {}, ROLES.caster,
            "RANGED", survive_template, "Wrath"),

    aoe_warlock_destruction =
        ClassConfig:create("aoe_warlock_destruction", {}, {}, get_class_color("warlock"), aoe_combat_warlock, {},
            ROLES.caster, "RANGED", survive_template, "Shadow Bolt"),

    aoe_mage_frost =
        ClassConfig:create("aoe_mage_frost", {}, {}, get_class_color("mage"), aoe_combat_mage, {}, ROLES.caster,
            "RANGED", survive_template, "Frostbolt"),

    shaman_resto_leveling =
        ClassConfig:create("shaman_resto_leveling", {}, {"Water Shield"}, get_class_color("shaman"), 
            combat_shaman_resto_leveling, {}, ROLES.healer, "HEALER", survive_shaman_resto),
};

function get_available_configs()
    return available_configs
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
