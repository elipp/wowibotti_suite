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
        CastSpellByName("Windfury Weapon");
    end
end

function combat_shaman_encha()
    if not has_buff("player", "Strength of Earth") then
        CastSpellByName("Strength of Earth Totem")
        return
    end
    set_totems();
    imbue_weapons();
    if validate_target() then
        melee_attack_behind(1.5);
        StartAttack();
        rotation:run();
    end
end
