local ve_guard = false;

combat_priest_shadow = function()

	if UnitChannelInfo("player") then return; end	-- don't clip mind flay
	if UnitCastingInfo("player") then return; end  
	

	if cleanse_party("Enveloping Wind") then return; end
	if cleanse_party("Lung Burst") then return; end
	
	if not UnitExists("target") or UnitIsDead("target") then 
		return;
	end
	
	caster_range_check(20); -- 20 yd on mind flay  :()
	caster_face();

	if (UnitMana("player") < 4000 and UnitHealth("target") > 50000) then if cast_if_nocd("Shadowfiend") then return; end end
	
	-- this has a slight bug, the debuffs take a while (ie. too long, longer than your avg spaminterval delay) to actually show up in the list
	-- the ve_guard stuff along with the UnitCasting/ChannelInfo is to combat that
	if not has_debuff("target", "Vampiric Touch") and not ve_guard then 
		CastSpellByName("Vampiric Touch");
		ve_guard = true;
	elseif not has_debuff("target", "Shadow Word: Pain") then
		CastSpellByName("Shadow Word: Pain");
		ve_guard = false;
	elseif GetSpellCooldown("Mind Blast") == 0 then
		CastSpellByName("Mind Blast");
		ve_guard = false;
	--elseif GetSpellCooldown("Shadow Word: Death") == 0 then CastSpellByName("Shadow Word: Death"); 
	else
		CastSpellByName("Mind Flay");
		ve_guard = false;
	end
	

end

