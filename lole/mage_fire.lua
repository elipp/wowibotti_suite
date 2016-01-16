config_mage_fire = {}
config_mage_fire.name = "mage_fire";

config_mage_fire.MODE_ATTRIBS = {
    ["buffmode"] = 0,
    ["combatbuffmode"] = 0,
    ["selfbuffmode"] = 0,
	["scorchmode"] = 0,
    ["playermode"] = 0
};

config_mage_fire.SELF_BUFFS = {"Molten Armor"};

local function delrissa()
	
	if keep_CCd("JokuHomo", "Polymorph") then return end
	
	if has_buff("Priestess Delrissa", "Power Word: Shield") then 
		TargetUnit("Priestess Delrissa")
		CastSpellByName("Spellsteal")
		return;
	end

end

config_mage_fire.combat = function()

	if UnitCastingInfo("player") then return; end
	if UnitChannelInfo("player") then return; end -- mainly in reference to evocation
	
	
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
	
	if config_mage_fire.MODE_ATTRIBS["scorchmode"] == 1 then
		local num_stacks = get_num_debuff_stacks("target", "Fire Vulnerability");

		if num_stacks < 5 then
			CastSpellByName("Scorch");
			return;
		end
	
		local hasdebuff, timeleft = has_debuff("target", "Fire Vulnerability");
		
		if (hasdebuff and timeleft < 8) then
			CastSpellByName("Scorch");
			return;
		end
	end
	
	CastSpellByName("Fireball");	
		
end

config_mage_fire.cooldowns = function() 
	UseInventoryItem(13);
	UseInventoryItem(14);

	cast_if_nocd("Berserking");
	cast_if_nocd("Combustion");
	cast_if_nocd("Icy Veins");
end

config_mage_fire.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP = { 
            ["Arcane Intellect"] = "Arcane Brilliance",
        };

        local buffs = {
            ["Arcane Intellect"] = MISSING_BUFFS_COPY["Arcane Intellect"],
        };

        local num_requests = get_num_buff_requests(buffs);
        
        if num_requests > 0 then
            SPAM_TABLE = get_spam_table(buffs, GROUP_BUFF_MAP);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_mage_fire.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_mage_fire.other = function()

end;
