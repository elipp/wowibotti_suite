local rotation = Rotation(
    {
        Spell("Flame Shock", nil, true),
        Spell("Earth Shock")
    }
)


local function imbue_weapons()
    local mh_imbue, _, _, oh_imbue, _, _ = GetWeaponEnchantInfo();
    if not mh_imbue then
        L_CastSpellByName("Windfury Weapon");
    elseif not oh_imbue then
        L_CastSpellByName("Flametongue Weapon")
    end
end

local TOTEMS = {
    fire=nil,
    earth={name="Strength of Earth Totem", range=30},
    -- earth={name="Stoneskin Totem", range=30},
    -- earth={name="Tremor Totem", range=20},
    water={name="Mana Spring Totem", range=30},
    -- water={name="Poison Cleansing Totem", range=20},
    air={name="Windfury Totem", range=20},
}

local function set_totems()
    local totem_status = get_totem_status(TOTEMS)
    -- if all(totem_status, function(_, need_recast) return need_recast == true end) then
    --     return CastSpellByName("Totemic Call")
    -- end
    for slot, needs_recasting in pairs(totem_status) do
        if needs_recasting then
            return L_CastSpellByName(TOTEMS[slot].name)
        end
    end
end

function combat_shaman_encha()
    imbue_weapons()
    if validate_target() then
        set_totems()
        L_StartAttack();
        if melee_attack_behind(1.5) then end
        
        -- rotation:run();

        if cast_if_nocd("Stormstrike") then return end
        if has_buff("player", "Focused") and cast_if_nocd("Earth Shock") then return end
        if (mana_percentage("player") < 20) and (#{get_combat_mobs()} > 2) and cast_if_nocd("Shamanistic Rage") then return end
        if cast_if_nocd("Lava Lash") then return end
        L_CastSpellByName("Purge")
    end
    
end
