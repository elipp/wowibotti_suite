local ENCOUNTER_FOLDER = '.\\Interface\\AddOns\\lole\\encounters\\'
local ENCOUNTER_TABLE = nil
ENCOUNTER_STATE = nil

local function get_mob_in_combat(name)
    L_TargetUnit(name)
    if UnitAffectingCombat("target") then
        return true
    else
        return false
    end
end

local function mob_health_pct_lte(name, pct)
    L_TargetUnit(name)
    local health_pct = UnitHealth("target") / UnitHealthMax("target") * 100
    return health_pct <= pct
end

local ENCOUNTER_ACTIONS = {
    MOB_IN_COMBAT = {
        func = get_mob_in_combat, 
        args = {"name"}
    },
    MOB_HEALTH_PCT_LTE = {
        func = mob_health_pct_lte, 
        args = {"name", "pct"}
    }
}

local function run_action(action_tbl)
    local func_tbl = ENCOUNTER_ACTIONS[action_tbl.action]
    local action_func = func_tbl.func
    local args = {}
    for i, arg in ipairs(func_tbl.args) do
        table.insert(args, action_tbl.args[arg])
    end
    return action_func(unpack(args))
end

local function parse_encounter_file(filename)
    local filepath = ENCOUNTER_FOLDER .. filename
    local json_str = read_file(filepath)
    local encounter_tbl = json.decode(json_str)
    return encounter_tbl
end

function lole_parse_encounter()
    local encounter_tbl = parse_encounter_file('encounter_template.json')
    console_print(to_string(encounter_tbl))
end

local function update_encounter_state(encounter_tbl)
    local phases = encounter_tbl.fight.phases
    local current_phase = 0
    if ENCOUNTER_STATE and starts_with(ENCOUNTER_STATE, "Phase") then
        current_phase = tonumber(ENCOUNTER_STATE:sub(#ENCOUNTER_STATE))
    end

    local next_phase = current_phase + 1
    if next_phase > #phases then
        return
    end
    
    local check_phase_shifted = phases[next_phase].phaseShiftCondition
    local is_new_phase = run_action(check_phase_shifted)

    if is_new_phase then
        ENCOUNTER_STATE = 'Phase ' .. tostring(next_phase)
        if IsRaidLeader() then
            SendChatMessage(ENCOUNTER_STATE .. ' started' , 'RAID_WARNING')
        end
    end
end

function run_encounter_actions()
    if ENCOUNTER_TABLE == nil then
        ENCOUNTER_TABLE = parse_encounter_file('encounter_template.json')
    end
    update_encounter_state(ENCOUNTER_TABLE)
end