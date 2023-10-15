local pet_food = "Tangy Clam Meat";
local last_feed_time = 0;
local FEED_INTERVAL = 60;

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
    if not has_buff("player", "Aspect of the Hawk") then
        L_CastSpellByName("Aspect of the Hawk")
    end
end

local function set_hunters_mark()
    -- Only apply Hunter's Mark on downtime or when not in combat
    if can_attack_target() and
            health_percentage("target") > 85 and
            not has_debuff("target", "Hunter's Mark") then
        L_CastSpellByName("Hunter's Mark");
    end
end

local function attack()
    StartAttack();
    PetAttack();
    if player_is_targeted() and get_distance_between("player", "target") < 15 then
        melee_rotation:run();
        return;
    else
        caster_range_check(12, 30);
        rotation:run();
        return;
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
    -- if need_mana() then return end;
    check_buffs();
    set_pet_state();
    if validate_target() then
        -- -- when the padit loppuivat: melee mode XD 
        -- StartAttack()
        -- CastSpellByName("Raptor Strike")
        set_hunters_mark();
        attack();
    end
    --echo("Combat activated")
    --caster_range_check(5,25)
end
