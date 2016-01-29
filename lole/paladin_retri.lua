config_paladin_retri = {}
config_paladin_retri.name = "paladin_retri";

config_paladin_retri.role = ROLES.MELEE;

config_paladin_retri.MODE_ATTRIBS = {
    ["combatbuffmode"] = 0,
	["buffmode"] = 0,
    ["playermode"] = 0
};

config_paladin_retri.SELF_BUFFS = {"Sanctity Aura"}
config_paladin_retri.COLOR = CLASS_COLORS["paladin"];

config_paladin_retri.combat = function()
	
	if cleanse_party("Arcane Shock") then return; end
	
	if not validate_target() then return; end
	
	melee_close_in()
	
	if not has_debuff("target", "Judgement of the Crusader") then
		if not has_buff("player", "Seal of the Crusader") then
			cast_if_nocd("Seal of the Crusader");
		end
		cast_if_nocd("Judgement");
		return;
	end

	if not has_buff("player", "Seal of Blood") then
		cast_if_nocd("Seal of Blood");
		return;
	end

	if cast_if_nocd("Crusader Strike") then return end

	if cast_if_nocd("Judgement") then return end


end

config_paladin_retri.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local buffs = {
            ["Greater Blessing of Salvation"] = MISSING_BUFFS_COPY["Blessing of Salvation"],
            ["Greater Blessing of Wisdom"] = MISSING_BUFFS_COPY["Blessing of Wisdom"],
            ["Greater Blessing of Might"] = MISSING_BUFFS_COPY["Blessing of Might"]
        };

        local num_paladins = get_num_paladins();
        
        if num_paladins < 2 then
            buffs["Greater Blessing of Kings"] = MISSING_BUFFS_COPY["Blessing of Kings"];
        end

	local num_requests = get_num_buff_requests(buffs);		

        if num_requests > 0 then
            SPAM_TABLE = get_paladin_spam_table(buffs, num_requests);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_paladin_retri.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_paladin_retri.other = function()

end;
