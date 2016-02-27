local function delrissa()
	
	if keep_CCd("Warlord Salaris", "Polymorph") then return end
	
	if has_buff("Priestess Delrissa", "Power Word: Shield") or 
	   has_buff("Priestess Delrissa", "Renew") then 
		TargetUnit("Priestess Delrissa")
		CastSpellByName("Spellsteal")
		return;
	end
	
	
	if not validate_target() then return end;
	
	--caster_range_check(36); 
--	caster_face_target();
	
	CastSpellByName("Fireball")
	
	

end

combat_mage_fire = function()

	if UnitCastingInfo("player") then return; end
	if UnitChannelInfo("player") then return; end -- mainly in reference to evocation
	
	--delrissa();
	
	--if true then return end;
	
	if ((GetItemCount(22044) == 0) and (not UnitAffectingCombat("player"))) then
		CastSpellByName("Conjure Mana Emerald");
		return;
	end

	
	local mana = UnitMana("player");
	local maxmana = UnitManaMax("player");
	
	if mana < (maxmana - 2500) then
		if (GetItemCount(22044) > 0) and (GetItemCooldown(22044) == 0) then 
			UseItemByName("Mana Emerald");
			return;
		end
	end
	
	if mana < 2500 then
		if GetSpellCooldown("Evocation") == 0 then
			CastSpellByName("Evocation");
		end
	end

	
	caster_range_check(36); 
	caster_face_target();
	
	-- replace this shit with gettalentinfo :D
	
	-- if <HAVE TALENT:IMPROVED SCORCH> then
		-- local num_stacks = get_num_debuff_stacks("target", "Fire Vulnerability");

		-- if num_stacks < 5 then
			-- CastSpellByName("Scorch");
			-- return;
		-- end
	
		-- local hasdebuff, timeleft = has_debuff("target", "Fire Vulnerability");
		
		-- if (hasdebuff and timeleft < 8) then
			-- CastSpellByName("Scorch");
			-- return;
		-- end
	-- end
	
	CastSpellByName("Fireball");	
		
end


