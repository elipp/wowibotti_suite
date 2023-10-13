local rotation = Rotation(
    {
        Spell("Flame Shock", nil, true),
        Spell("Earth Shock")
    }
)


local function imbue_weapons()
    local mh_imbue, _, _, oh_imbue, _, _ = GetWeaponEnchantInfo();
    if not mh_imbue then
        CastSpellByName("Windfury Weapon");
    end
end

local TOTEMS = {
    fire=nil,
    earth={name="Strength of Earth Totem", range=30},
    water={name="Mana Spring Totem", range=30},
    air={name="Windfury Totem", range=20},
}

local function set_totems()
    local totem_status = get_totem_status(TOTEMS)
    -- if all(totem_status, function(_, need_recast) return need_recast == true end) then
    --     return CastSpellByName("Totemic Call")
    -- end
    for slot, needs_recasting in pairs(totem_status) do
        if needs_recasting then
            return CastSpellByName(TOTEMS[slot].name)
        end
    end
end

function combat_shaman_encha()
    imbue_weapons()
     if validate_target() then
        set_totems()
        if melee_attack_behind(1.5) then end
        StartAttack();
        
        -- rotation:run();
      
        if has_buff("player", "Focused") then
            return CastSpellByName("Earth Shock")
        end        
        CastSpellByName("Purge")
    end
    
end
