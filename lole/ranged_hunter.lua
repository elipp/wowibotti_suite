local pet_food = "Soft Banana Bread";
local last_feed_time = 0;
local FEED_INTERVAL = 60;

function run_out_of_combat()
    if not UnitAffectingCombat("player") then
        if not has_buff("player", "Aspect of the Pack") and not has_buff("player", "Aspect of the Viper") then
            return L_CastSpellByName("Aspect of the Viper")
        end
    end
end


local dummy_frame = CreateFrame("frame",nil, UIParent)
dummy_frame:SetScript("OnUpdate", run_out_of_combat)

local rotation = Rotation(
    {
        Spell("Arcane Shot", function() return true end),
        Spell("Serpent Sting", nil, true),
        --Spell("Concussive Shot")
    }
)

local melee_rotation = Rotation({
    Spell("Raptor Strike")
})

function hunter_survive()
    -- this is for surviving aggro as hunter :)
    L_CastSpellByName("Feign Death");
end

local function set_pet_state()
    if UnitExists("pet") and UnitIsDead("pet") then
        L_CastSpellByName("Revive Pet");
    elseif not UnitExists("pet") then
        L_CastSpellByName("Call Pet");
    end

    if health_percentage("pet") < 25 and not has_buff("pet", "Mend Pet") then
        L_CastSpellByName("Mend Pet");
    end

    -- Should we feed the pet? Full happiness of 4 gives 150% dmg
    if not UnitAffectingCombat("player") then
        L_PetPassiveMode();
        local pet_happiness, _, _ = GetPetHappiness();
        if (pet_happiness ~= nil and pet_happiness < 3 and
            not has_buff("pet", "Feed Pet Effect") and
            GetTime() - last_feed_time > FEED_INTERVAL) then
            if GetItemCount(pet_food) == 0 then
                echo("Out of pet food!");
            else
                L_CastSpellByName("Feed Pet");
                L_UseItemByName(pet_food);
            end
            last_feed_time = GetTime();
        end
    end
end

local function check_buffs()
    if not UnitAffectingCombat("player") then
        if not has_buff("player", "Aspect of the Viper") then
            L_CastSpellByName("Aspect of the Viper")
        end
    elseif not has_buff("player", "Aspect of the Hawk") then
        L_CastSpellByName("Aspect of the Hawk")
    end
end

local function attack()
    if player_is_targeted() and get_distance_between("player", "target") < 15 then
        return melee_rotation:run()
    else
        set_pet_state();
        
        L_PetAttack()
        if UnitChannelInfo("player") == "Volley" then return end
        L_StartAttack();
        
        if not has_buff("player", "Aspect of the Hawk") then
            L_CastSpellByName("Aspect of the Hawk")
        end
        
        caster_range_check(6, 30)
        
        local feasibility = get_aoe_feasibility("target", 8)
        if lole_subcommands.get("aoemode") == 1 and get_distance_between("player", "target") < 30 and UnitMana("player") > 600 and feasibility > 4.5 then
            return cast_gtaoe("Volley(Rank 2)", get_unit_position("target"))
        end
        
        if UnitHealth("target") > 7500 and not has_debuff("target", "Hunter's Mark") then
            return L_CastSpellByName("Hunter's Mark");
        end

        if UnitMana("player") < 500 and not has_buff("player", "Aspect of the Viper") then
            return L_CastSpellByName("Aspect of the Viper")
        end
        
        local has_viper_sting = has_debuff("target", "Viper Sting")
        if (UnitPowerType("target") == 0) and (UnitHealth("target") > 2000) and (UnitMana("target") > 200) and (not has_viper_sting) then
            return L_CastSpellByName("Viper Sting")
        elseif (not has_viper_sting) and (not has_debuff("target", "Serpent Sting")) then
            return L_CastSpellByName("Serpent Sting")
        elseif feasibility > 1.5 and cast_if_nocd("Multi-Shot") then
            return
        elseif cast_if_nocd("Arcane Shot") then
            return
        else
            L_CastSpellByName("Steady Shot")
        end
    end
end

local function need_mana()
    if mana_percentage("player") < 30 then
        if not has_buff("player", "Drink") then
            L_UseItemByName("Refreshing Spring Water");
        end
        return true;
    end
    if has_buff("player", "Drink") and not mana_percentage("player") > 85 then
        return true;
    end
    return false;
end


function combat_ranged_hunter()
    if validate_target() then
        attack()
    end
end
