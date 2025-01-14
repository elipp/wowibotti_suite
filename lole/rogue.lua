local mh_apply = nil
local mh_at = 0
local oh_apply = nil
local oh_at = 0

local rotation = Rotation(
    {
        Spell("Backstab"),
    }
)

local function reapply_poisons()
    if mh_apply then
        L_RunMacroText("/use 16")
        mh_at = GetTime()
        mh_apply = nil
    elseif oh_apply then
        L_RunMacroText("/use 17")
        oh_at = GetTime()
        oh_apply = nil
    end

    local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
    -- echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

    if not has_mh then
        L_RunMacroText("/use Instant Poison IV")
        if GetTime() - mh_at > 5 then
            mh_apply = 1
        end
    elseif not has_oh then
        L_RunMacroText("/use Deadly Poison III")
        if GetTime() - oh_at > 5 then
            oh_apply = 1
        end
    end
end


function rogue_combat_before()
    reapply_poisons()

    if not validate_target() then return end

    if not UnitAffectingCombat("player") then
        L_CastSpellByName("Stealth")
        L_CastSpellByName("Sprint")
    end

    melee_attack_behind(1.5) -- 8 FOR THE RUBY SANCTUM GUY, 1.5 for magic

    if health_percentage("player") < 15 then
        L_CastSpellByName("Cloak of Shadows")
    end

    if unit_castorchannel("target") then
        L_CastSpellByName("Kick")
    end

    if not has_buff("player", "Hunger For Blood") then
        L_CastSpellByName("Hunger For Blood")
        -- don't return, this will fail if no bleed active
    end

    if GetSpellCooldown("Vanish") == 0 and not has_buff("player", "Overkill") then
        L_CastSpellByName("Vanish")
    end


    if GetComboPoints("player", "target") < 4 then
        if UnitHealth("target") < 15000 then
            L_CastSpellByName("Envenom")
        else
            L_CastSpellByName("Mutilate")
        end
    else
        if GetComboPoints("player", "target") == 5 then
            L_CastSpellByName("Cold Blood")
        end
        L_CastSpellByName("Envenom")
    end
end

function rogue_combat_leveling_70()
    reapply_poisons()
    if not validate_target() then return end

    melee_attack_behind(1.5);
    L_StartAttack();

    local target_health = UnitHealth("target")
    local combo_points = GetComboPoints("player", "target")

    if get_aoe_feasibility("player", 8) > 2.5 and total_combat_mob_health() > 30000 and cast_if_nocd("Blade Flurry") then
        return
    elseif unit_castorchannel("target") then
        L_CastSpellByName("Kick")
    elseif (not has_buff("player", "Slice and Dice")) and ((target_health < 5000 and combo_points > 2) or combo_points >= 4) then
        return L_CastSpellByName("Slice and Dice")
    elseif (combo_points >= 1 and target_health < 5000) or (UnitHealth("target") > 10000 and combo_points >= 5) then
        return L_CastSpellByName("Eviscerate")
    else
        return L_CastSpellByName("Sinister Strike")
    end
end

local function jump_interval()
    local m = math.random()
    return (m^3*5 + 2) * 1000
end

local function jump()
    if UnitAffectingCombat("player") then
        L_JumpOrAscendStart()
        local interval = jump_interval()
        echo("setting timeout ", interval)
        TIMER = setTimeout(jump, interval)
    else
        TIMER = nil
    end
end

TIMER = nil


local palli = CreateFrame("frame", nil, UIParent)
palli:SetScript("OnUpdate", function()
    if playermode() then return end

    local function run_and_cancel(callback, cancel)
        if math.random() < 0.01 then
            callback()
            setTimeout(function() cancel() end, (50+math.random()*300))
        end
    end

    run_and_cancel(L_StrafeLeftStart, L_StrafeLeftStop)
    run_and_cancel(L_StrafeRightStart, L_StrafeRightStop)
    run_and_cancel(L_MoveForwardStart, L_MoveForwardStop)
    run_and_cancel(L_TurnLeftStart, L_TurnLeftStop)
    run_and_cancel(L_TurnRightStart, L_TurnRightStop)
    run_and_cancel(L_JumpOrAscendStart, L_AscendStop)

end)

function rogue_combat()
    if not validate_target() then
        return
    end

    melee_attack_behind(1.5);
    L_StartAttack();

    local target_health = UnitHealth("target")
    local combo_points = GetComboPoints("player", "target")

    L_CastSpellByName("Sinister Strike")

    if TIMER == nil then
        local interval = jump_interval()
        TIMER = setTimeout(jump, interval)
    end

    -- if GetSpellCooldown("Vanish") == 0 and not has_buff("player", "Overkill") then
    --     L_CastSpellByName("Vanish")
    -- end

    -- if combo_points < 4 then
    --     if combo_points > 1 and UnitHealth("target") < 10000 then
    --         L_CastSpellByName("Envenom")
    --     else
    --         L_CastSpellByName("Mutilate")
    --     end
    -- else
    --     if GetComboPoints("player", "target") == 5 then
    --         L_CastSpellByName("Cold Blood")
    --     end
    --     L_CastSpellByName("Envenom")
    -- end

end
