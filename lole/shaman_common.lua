TIME_TOTEMS_REFRESHED = 0;

TOTEM_TYPE_ID_MAP = {
    fire = 1,
    earth = 2,
    water = 3,
    air = 4,
};

local TOTEM_NAME_TYPE_MAP = {
    ["Tremor Totem"] = TOTEM_TYPE_ID_MAP["earth"],
    ["Stoneskin Totem"] = TOTEM_TYPE_ID_MAP["earth"],
    ["Strength of Earth Totem"] = TOTEM_TYPE_ID_MAP["earth"],

    ["Totem of Wrath"] = TOTEM_TYPE_ID_MAP["fire"],
    ["Frost Resistance Totem"] = TOTEM_TYPE_ID_MAP["fire"],
    ["Searing Totem"] = TOTEM_TYPE_ID_MAP["fire"],
    ["Flametongue Totem"] = TOTEM_TYPE_ID_MAP["fire"],

    ["Mana Spring Totem"] = TOTEM_TYPE_ID_MAP["water"],
    ["Mana Tide Totem"] = TOTEM_TYPE_ID_MAP["water"],
    ["Cleansing Totem"] = TOTEM_TYPE_ID_MAP["water"],

    ["Wrath of Air Totem"] = TOTEM_TYPE_ID_MAP["air"],
    ["Windfury Totem"] = TOTEM_TYPE_ID_MAP["air"],
    ["Grace of Air Totem"] = TOTEM_TYPE_ID_MAP["air"],
    ["Nature Resistance Totem"] = TOTEM_TYPE_ID_MAP["air"]
};

local TOTEM_NAME_BUFFNAME_MAP = {
    --	["Tremor Totem"] = nil,
    ["Stoneskin Totem"] = "Stoneskin",
    ["Strength of Earth Totem"] = "Strength of Earth",

    ["Totem of Wrath"] = "Totem of Wrath",
    ["Frost Resistance Totem"] = "Frost Resistance",
    ["Flametongue Totem"] = "Flametongue Totem",

    ["Mana Spring Totem"] = "Mana Spring",
    --	["Mana Tide Totem"] = nil,
    --	["Poison Cleansing Totem"] = nil,
    --	["Disease Cleansing Totem"] = nil,

    ["Wrath of Air Totem"] = "Wrath of Air Totem",
    --  ["Windfury Totem"] = nil,
    ["Grace of Air Totem"] = "Grace of Air"

};

function get_active_multicast_totems()
    echo("warning: tbc doesn't have totem multicast")
    if true then return end
    local r = {}
    for i = 1, 4 do
        local _, _, _, spellId = GetActionInfo(_G["MultiCastSlotButton" .. tostring(i)].actionButton.action)
        r[i] = GetSpellInfo(spellId)
    end
    return r
end

function get_active_multicast_summonspell()
    echo("warning: tbc doesn't have totem multicast")
    if true then return end
    return GetSpellInfo(MultiCastSummonSpellButton.spellId)
end

function get_totem_status(totems)
    local original_target = UnitGUID("target");
    local res = {}
    for slot, spell_info in pairs(totems) do
        if spell_info ~= nil then
            local should_recast = false
            local _, totemName, startTime, duration = GetTotemInfo(TOTEM_TYPE_ID_MAP[slot]);

            if totemName == "Mana Tide Totem" then
                should_recast = false
            elseif startTime == 0 and duration == 0 then
                should_recast = true
                -- TODO: check if totem is even the same name :D
            else
                L_TargetUnit(spell_info.name)
                should_recast = get_distance_between("player", "target") > (spell_info.range - 2)
            end
            res[slot] = should_recast
        end
    end

    target_unit_with_GUID(original_target)
    return res
end

function refresh_totems(TOTEMS, TOTEM_BAR)
    local totems_to_recast = {}
    local num_to_recast = 0
    for slot, name in pairs(TOTEMS) do
        if should_recast_totem(name) then
            totems_to_recast[name] = true
            num_to_recast = num_to_recast + 1
        end
        if num_to_recast > 2 then
            L_CastSpellByName(TOTEM_BAR)
            TIME_TOTEMS_REFRESHED = GetTime()
            return true
        end
    end

    for totem, _ in pairs(totems_to_recast) do
        L_CastSpellByName(totem)
        TIME_TOTEMS_REFRESHED = GetTime()
        return true
    end

    return false
end
