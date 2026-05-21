TIME_TOTEMS_REFRESHED = 0;

TOTEM_TYPE_ID_MAP = {
    fire  = 1,
    earth = 2,
    water = 3,
    air   = 4,
};

local TOTEM_NAME_TYPE_MAP = {
    ["Tremor Totem"]            = TOTEM_TYPE_ID_MAP["earth"],
    ["Stoneskin Totem"]         = TOTEM_TYPE_ID_MAP["earth"],
    ["Strength of Earth Totem"] = TOTEM_TYPE_ID_MAP["earth"],
    ["Totem of Wrath"]          = TOTEM_TYPE_ID_MAP["fire"],
    ["Frost Resistance Totem"]  = TOTEM_TYPE_ID_MAP["fire"],
    ["Searing Totem"]           = TOTEM_TYPE_ID_MAP["fire"],
    ["Flametongue Totem"]       = TOTEM_TYPE_ID_MAP["fire"],
    ["Mana Spring Totem"]       = TOTEM_TYPE_ID_MAP["water"],
    ["Mana Tide Totem"]         = TOTEM_TYPE_ID_MAP["water"],
    ["Cleansing Totem"]         = TOTEM_TYPE_ID_MAP["water"],
    ["Wrath of Air Totem"]      = TOTEM_TYPE_ID_MAP["air"],
    ["Windfury Totem"]          = TOTEM_TYPE_ID_MAP["air"],
    ["Grace of Air Totem"]      = TOTEM_TYPE_ID_MAP["air"],
    ["Nature Resistance Totem"] = TOTEM_TYPE_ID_MAP["air"],
};

local TOTEM_RANGE = 30; -- yards, same for all totems in vanilla/wotlk

local MANA_TIDE_TOTEM = "Mana Tide Totem";

function get_active_multicast_totems()
    local r = {}
    for i = 1, 4 do
        local _, _, _, spellId = GetActionInfo(_G["MultiCastSlotButton" .. tostring(i)].actionButton.action)
        r[i] = GetSpellInfo(spellId)
    end
    return r
end

function get_active_multicast_summonspell()
    return GetSpellInfo(MultiCastSummonSpellButton.spellId)
end

-- totems: array of spell name strings indexed 1-4 (from get_active_multicast_totems)
-- returns: array of booleans indexed 1-4, true = should recast
function get_totem_status(totems)
    local original_target = UnitGUID("target");
    local res = {}

    for slot, spell_name in pairs(totems) do
        if spell_name ~= nil then
            local should_recast = false
            local totem_type = TOTEM_NAME_TYPE_MAP[spell_name]
            local _, totemName, startTime, duration = GetTotemInfo(totem_type);

            if spell_name == MANA_TIDE_TOTEM then
                should_recast = false
            elseif startTime == 0 and duration == 0 then
                should_recast = true
            else
                L_TargetUnit(spell_name)
                should_recast = get_distance_between("player", "target") > (TOTEM_RANGE - 2)
            end

            res[slot] = should_recast
        end
    end

    if original_target then
        target_unit_with_GUID(original_target)
    end
    return res
end

-- totems: array of spell name strings (from get_active_multicast_totems)
-- summon_spell: string spell name for the multicast bar (from get_active_multicast_summonspell)
function refresh_totems(totems, summon_spell)
    local totems_to_recast = {}
    local num_to_recast = 0

    local totem_status = get_totem_status(totems)
    for slot, should_recast in pairs(totem_status) do
        if should_recast then
            totems_to_recast[slot] = totems[slot]
            num_to_recast = num_to_recast + 1
        end
    end

    if num_to_recast == 0 then
        return false
    end

    if num_to_recast > 2 then
        L_CastSpellByName(summon_spell)
        TIME_TOTEMS_REFRESHED = GetTime()
        return true
    end

    for _, name in pairs(totems_to_recast) do
        L_CastSpellByName(name)
    end
    TIME_TOTEMS_REFRESHED = GetTime()
    return true
end
