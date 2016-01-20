local function auto_stancedance() 
	local name = UnitCastingInfo("target");
	if (name == "Bellowing Roar") then
		echo("Voi vittu, castaa bellowing roarii!!");
		-- CastSpellByName("Berserker Stance");
		-- CastSpellByName("Berserker Rage");
		-- CastSpellByName("Defensive Stance");
	end

end

config_warrior_prot = {}
config_warrior_prot.name = "warrior_prot";
config_warrior_prot.SELF_BUFFS = {};
config_warrior_prot.MODE_ATTRIBS = { ["playermode"] = 0 }
config_warrior_prot.COLOR = CLASS_COLORS["warrior"]


config_warrior_prot.combat = function()
	
	CastSpellByName("Heroic Strike");

	if UnitCastingInfo("target") then
		if not cast_if_nocd("Spell Reflect") then
			cast_if_nocd("Shield Bash");
		end
		return;
	end

	if cast_if_nocd("Shield Block") then return; end
	if cast_if_nocd("Shield Bash") then return; end
	
	if IsUsableSpell("Revenge") then
		CastSpellByName("Revenge");
		return;
	end

	if not has_buff("player", "Commanding Shout") then
		CastSpellByName("Commanding Shout");
	end

	if not has_debuff("target", "Thunder Clap") then
		CastSpellByName("Thunder Clap");
		return;
	end

	if not has_debuff("target", "Demoralizing Shout") then
		CastSpellByName("Demoralizing Shout");
		return;
	end

	CastSpellByName("Devastate");

end

config_warrior_prot.cooldowns = function() 

end


config_warrior_prot.buffs = function()

end

config_warrior_prot.desired_buffs = function()

    local desired_buffs = {
        "Blessing of Kings",
        "Blessing of Might",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Mark of the Wild",
        "Thorns",
    }

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;

end

config_warrior_prot.other = function()

end;
