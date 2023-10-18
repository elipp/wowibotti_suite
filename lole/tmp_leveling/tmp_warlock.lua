local tap_started = 0;
local soul_shard_toggle = false
local toggle_on = 5
local toggle_off = 14

function tmp_warlock_combat()
    if GetItemCount("Soul Shard") < toggle_on and soul_shard_toggle == false then
        soul_shard_toggle = true
    elseif GetItemCount("Soul Shard") >= toggle_off and soul_shard_toggle == true then
        soul_shard_toggle = false
    else
        soul_shard_toggle = true
    end

    if not UnitAffectingCombat("player") then
        if not UnitExists("pet") then
            -- has a bug because of lag check
            CastSpellByName("Summon Imp")
            return
        end
        if mana_percentage("player") < 75 and
                health_percentage("player") > 30 then
            CastSpellByName("Life Tap")
        end
    end

    if validate_target() then
        stopfollow()
        caster_range_check(1, 29)

        if not has_debuff("target", "Corruption") and health_percentage("target") > 75 then
            CastSpellByName("Corruption")
        end

        if soul_shard_toggle == true and health_percentage("target") < 40 then
            if UnitChannelInfo("player") == "Drain Soul" then
                return
            else
                CastSpellByName("Drain Soul")
                return
            end
        end

        CastSpellByName("Shadow Bolt")
        PetAttack()
    end
end