-- MAIN_TANK = "Adieux";
-- local reverse_target = {Noctur="Adieux", Adieux="Noctur"};

-- local function on_spell_sent_event(self, event, caster, spell, rank, target)
-- 	if reverse_target[target] then
-- 		MAIN_TANK = reverse_target[target];
-- 	end
-- end

-- local spell_sent_frame = CreateFrame("Frame");
-- spell_sent_frame:RegisterEvent("UNIT_SPELLCAST_SENT");
-- spell_sent_frame:SetScript("OnEvent", on_spell_sent_event);

combat_druid_resto = function()
   
	TargetUnit(MAIN_TANK);
	local mana_left = UnitMana("player");

	if mana_left < 3000 and GetSpellCooldown("Innervate") == 0 then
		CastSpellByName("Innervate", "player");
		return;
	end

	local has, timeleft, stacks = has_buff("target", "Lifebloom")

	if has then
		if stacks < 3 then
			CastSpellByName("Lifebloom")
			return
		elseif timeleft < 0.7 then
			CastSpellByName("Lifebloom")
			return
		end
	else
		CastSpellByName("Lifebloom")
		return
	end

	if (not has_buff("target", "Rejuvenation")) then
		CastSpellByName("Rejuvenation")
		return
	end

	if (UnitHealth("target") < 5500) then
		CastSpellByName("Swiftmend")
		return
	end

	-- if MAIN_TANK == "Adieux" then
 --    	MAIN_TANK = "Noctur"
 --    else
 --    	MAIN_TANK = "Adieux"
 --    end

end
