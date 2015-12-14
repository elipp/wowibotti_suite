config_paladin_retri = {}
config_paladin_retri.name = "paladin_retri";

config_paladin_retri.MODE_ATTRIBS = {
	["buffmode"] = 0,
};

config_paladin_retri.SELF_BUFFS = {"Sanctity Aura"};

config_paladin_retri.combat = function()

    if not UnitExists("target") then
        TargetNearestEnemy();
     end

    StartAttack("target");

   -- if cast_if_nocd("Consecration") then return; end -- don't know?
        
    if not has_debuff("target", "Judgement of the Crusader") then
         if not has_buff("player", "Seal of the Crusader") then
            CastSpellByName("Seal of the Crusader");
        end
        cast_if_nocd("Judgement");
        return;
     end

    if not has_buff("player", "Seal of Blood") then
        CastSpellByName("Seal of Blood");
        return;
    end
        
    if cast_if_nocd("Judgement") then return; end

    
end

config_paladin_retri.buffs = function(MISSING_BUFFS_COPY)

    if not BUFF_TABLE_READY then
        local buffs = {
            ["Greater Blessing of Kings"] = MISSING_BUFFS_COPY["Blessing of Kings"]
        };

        local num_paladins = get_num_paladins();

        if num_paladins < 2 then
            buffs["Greater Blessing of Salvation"] = MISSING_BUFFS_COPY["Blessing of Salvation"];
            buffs["Greater Blessing of Wisdom"] = MISSING_BUFFS_COPY["Blessing of Wisdom"];
            buffs["Greater Blessing of Might"] = MISSING_BUFFS_COPY["Blessing of Might"];
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

    local desired_buffs = {
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Arcane Intellect",
        "Mark of the Wild",
        "Thorns",
    }

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;

end

config_paladin_retri.other = function()

end;
