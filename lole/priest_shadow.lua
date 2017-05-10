local LAST_SPELL_CAST = ""

local cast_frame = CreateFrame("Frame")

cast_frame:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
	if event == "UNIT_SPELLCAST_SUCCEEDED" then
		LAST_SPELL_CAST = message
	end
end)

cast_frame:RegisterEvent("UNIT_SPELLCAST_SUCCEEDED")
cast_frame:RegisterEvent("COMBAT_LOG_EVENT_UNFILTERED")
combat_priest_shadow = function()


	local S = player_casting()
	if S and S == "Mind Flay" then return end
	if S and S == LAST_SPELL_CAST and S ~= "Mind Blast" then SpellStopCasting() end

	if not validate_target() then return end
	caster_range_check(20); -- 20 yd on mind flay  :()

	if (UnitMana("player") < 4000 and UnitHealth("target") > 50000) then if cast_if_nocd("Shadowfiend") then return; end end

	--if UnitMana("player") < 800 then
	--	L_CastSpellByName("Shoot")
	--	return
	--end

	if not has_debuff("target", "Vampiric Touch") then
		L_CastSpellByName("Vampiric Touch");
		return

	elseif not has_debuff("target", "Shadow Word: Pain") then
		L_CastSpellByName("Shadow Word: Pain");
		return

	elseif GetSpellCooldown("Mind Blast") == 0 then
		L_CastSpellByName("Mind Blast");
	--elseif GetSpellCooldown("Shadow Word: Death") == 0 then L_CastSpellByName("Shadow Word: Death");
	elseif S ~= "Mind Flay" then
		L_CastSpellByName("Mind Flay");
	end


end
