local ENCOUNTER_FOLDER = '.\\Interface\\AddOns\\lole\\encounters\\'
local ENCOUNTER_TABLE = nil
ENCOUNTER_PHASE = 0

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

local function update_encounter_phase(encounter_tbl)
    local phases = encounter_tbl.fight.phases
    if ENCOUNTER_PHASE == #phases then
        return
    end
    
    local next_phase = ENCOUNTER_PHASE + 1
    local check_phase_shifted = phases[next_phase].phaseShiftCondition
    local is_new_phase = run_action(check_phase_shifted)

    if is_new_phase then
        ENCOUNTER_PHASE = next_phase
        if IsRaidLeader() then
            SendChatMessage('Phase ' .. ENCOUNTER_PHASE .. ' started' , 'RAID_WARNING')
        end
    end
end

function run_encounter_actions()
    if ENCOUNTER_TABLE == nil then
        ENCOUNTER_TABLE = parse_encounter_file('encounter_template.json')
    end
    update_encounter_phase(ENCOUNTER_TABLE)
end