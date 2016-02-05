config_druid_balance = {}
config_druid_balance.name = "druid_balance";

config_druid_balance.role = ROLES.CASTER;

-- config_druid_balance.MODE_ATTRIBS = {
    -- ["combatbuffmode"] = 0,
    -- ["buffmode"] = 0,
    -- ["playermode"] = 0,
-- };

config_druid_balance.MODE_ATTRIBS = {}

config_druid_balance.MODE_ATTRIBS.buffmode = 0;
config_druid_balance.MODE_ATTRIBS.combatbuffmode = 0;
config_druid_balance.MODE_ATTRIBS.playermode = 0;


config_druid_balance.SELF_BUFFS = {"Moonkin Form"}; 
config_druid_balance.COLOR = CLASS_COLORS["druid"];

config_druid_balance.combat = function()

-- seems to not work otherwise O_O fucking lua...
	if not config_druid_balance.MODE_ATTRIBS["playermode"] then 
		config_druid_balance.MODE_ATTRIBS["playermode"] = 0;
	end

	caster_range_check(35);
	caster_face_target();
	
	if UnitCastingInfo("player") then return; end

	if not UnitExists("target") then return; end

	
	if not has_debuff("target", "Insect Swarm") then 
		CastSpellByName("Insect Swarm"); 
	-- elseif not target_has_debuff("target", "Moonfire") then CastSpellByName("Moonfire");
	else 
		CastSpellByName("Starfire"); 
	end

	
	 --special shit for curator
	--CastSpellByName("Wrath");
	
end

config_druid_balance.cooldowns = function() 
	UseInventoryItem(13);
	UseInventoryItem(14);
	cast_if_nocd("Barkskin");
end

config_druid_balance.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local GROUP_BUFF_MAP = { 
            ["Mark of the Wild"] = "Gift of the Wild",
        };

        local buffs = {
            ["Mark of the Wild"] = MISSING_BUFFS_COPY["Mark of the Wild"],
            ["Thorns"] = MISSING_BUFFS_COPY["Thorns"],
        };
        
        if buffs["Mark of the Wild"] ~= nil then
            SELF_BUFF_SPAM_TABLE[1] = "Moonkin Form";
        end

        local num_requests = get_num_buff_requests(buffs);
        
        if num_requests > 0 then
            SPAM_TABLE = get_spam_table(buffs, GROUP_BUFF_MAP);
            BUFF_TABLE_READY = true;
        end
    end

    buffs();

end

config_druid_balance.desired_buffs = function()

    local desired_buffs = get_desired_buffs("dps");
    return desired_buffs;

end

config_druid_balance.other = function()

end;
