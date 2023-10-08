local rotation = Rotation(
    {
        Spell("Flame Shock", nil, true),
        Spell("Earth Shock")
    }
)

local function set_totems()
    return nil;
end

local function imbue_weapons()
    local mh_imbue, _, _, oh_imbue, _, _ = GetWeaponEnchantInfo();
    if not mh_imbue then
        L_CastSpellByName("Rockbiter Weapon");
    end
end

function combat_shaman_encha()
    set_totems();
    imbue_weapons();
    if validate_target() then
        melee_attack_behind(1);
        L_StartAttack();
        rotation:run();
    end
end
