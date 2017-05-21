local poison_warning_given = false

function check_apply_OH_poison()
    local has_mh, mh_exp, mh_charges, has_oh, oh_exp, oh_charges = GetWeaponEnchantInfo()
    --echo(tostring(has_mh) .. ", " .. tostring(mh_exp) .. ", " .. tostring(mh_charges)  .. ", " .. tostring(has_oh) .. ", " .. tostring(oh_exp)  .. ", " .. tostring(oh_charges))

    if (not has_oh) then
        if (GetItemCount(22054) == 0) and not poison_warning_given then
            SendChatMessage("I've run out of Deadly Poison VII. RIP!", "GUILD")
            poison_warning_given = true
            return false;
        end

        L_RunMacroText("/use Deadly Poison VII")
        L_UseInventoryItem(17)
    end

end

combat_rogue_combat = function()

    check_apply_OH_poison()

    if not validate_target() then return end

  --  melee_avoid_aoe_buff(33238)
	melee_attack_behind()

	if (not UnitAffectingCombat("player")) then
	    L_CastSpellByName("Stealth")
		return;
	end

    if lole_subcommands.get("aoemode") == 1 then
        if cast_if_nocd("Blade Flurry") then return end
    end

    if (GetComboPoints("player", "target") < 5) then
        L_CastSpellByName("Sinister Strike")
        return
    end

    local has, timeleft = has_buff("player", "Slice and Dice")

    if (not has or timeleft < 5) then
        L_CastSpellByName("Slice and Dice")
        return
    end

    if (not has_debuff("target", "Rupture")) then
        L_CastSpellByName("Rupture")
        return
    end

    L_CastSpellByName("Eviscerate")

end
