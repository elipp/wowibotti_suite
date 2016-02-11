BUFF_TIME = 0;
LAST_BUFF_CHECK = 0;
SPAM_TABLE = {};
SELF_BUFF_SPAM_TABLE = {};
BUFFS_CHECKED = false;
LAST_LBUFFCHECK = 0;
LBUFFCHECK_ISSUED = false;

BUFF_ALIASES = {
    ["Arcane Intellect"] = "Arcane Brilliance",
--  ["Arcane Brilliance"] = "Arcane Intellect",
    
    ["Mark of the Wild"] = "Gift of the Wild",
--  ["Gift of the Wild"] = "Mark of the Wild",
    
    ["Power Word: Fortitude"] = "Prayer of Fortitude",
--  ["Prayer of Fortitude"] = "Power Word: Fortitude",
    
    ["Divine Spirit"] = "Prayer of Spirit",
--  ["Prayer of Spirit"] = "Divine Spirit",
    
    ["Blessing of Kings"] = "Greater Blessing of Kings",
--  ["Greater Blessing of Kings"] = "Blessing of Kings",
    
    ["Blessing of Wisdom"] = "Greater Blessing of Wisdom",
--  ["Greater Blessing of Wisdom"] = "Blessing of Wisdom",
    
    ["Blessing of Salvation"] = "Greater Blessing of Salvation",
--  ["Greater Blessing of Salvation"] = "Blessing of Salvation"

    ["Blessing of Might"] = "Greater Blessing of Might",
};

function get_desired_buffs(role)

    local caster_buffs = {
        "Blessing of Salvation",
        "Blessing of Kings",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Arcane Intellect",
        "Mark of the Wild"
    };

    local healer_buffs = {
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Arcane Intellect",
        "Mark of the Wild"
    };
	
	local melee_buffs = { -- from warrior_arms
		"Blessing of Kings",
        "Blessing of Might",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Mark of the Wild",
        "Thorns"
	}

    local desired_buffs;
    if role == ROLES.caster then
        desired_buffs = caster_buffs;
	elseif role == ROLES.melee then
		desired_buffs = melee_buffs;
    elseif role == ROLES.healer then
        desired_buffs = healer_buffs;
    else
        return {};
    end

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;
end

function lole_buffs()
    if lole_subcommands.get("selfbuffmode") == 1 then
        lole_selfbuffs();
    else
        if (get_current_config().buffs ~= nil) then
            local MISSING_BUFFS_COPY;
            if not BUFF_TABLE_READY then
                MISSING_BUFFS_COPY = shallowcopy(MISSING_BUFFS);
            end
			lole_subcommands.buffs(MISSING_BUFFS_COPY);
        end
    end
end

function lole_leaderbuffcheck(arg)

    if arg ~= nil and arg ~= "clean" then 
        echo("lole_leaderbuffcheck: erroneous argument!");
        return false;
    end

    if not IsRaidLeader() then
        echo("lole_leaderbuffcheck: You're not the raid leader.");
        return false;
    end

    local msgstr = "buffcheck";
    if arg == "clean" then
        msgstr = msgstr .. " " .. arg;
    end
    SendAddonMessage("lole_buffcheck", msgstr, "RAID", UnitName("player"));

end

-- verbose: either nil or boolean
function lole_buffcheck(arg, verbose)
    if verbose == nil then
        verbose = true;
    end

    if (arg ~= nil and arg ~= "clean") or type(verbose) ~= type(true) then 
        echo("lole_buffcheck: erroneous argument!");
        return false;
    end

    local buffname_timeleft_map = {}
    
    for i=1,32 do id,cancel = GetPlayerBuff(i,"HELPFUL") -- |HARMFUL|PASSIVE"); -- not needed really
        if (id > 0) then
            local name = GetPlayerBuffName(id);
            local timeleft = GetPlayerBuffTimeLeft(id);
            if cancel == 1 then
                timeleft = 1000;
            end
            buffname_timeleft_map[name] = timeleft;
        end
    end
   
    local missing_table = {};

    if arg == "clean" then
        if verbose then
            echo("lole_buffcheck: requested full rebuffage");
        end
        missing_table = get_desired_buffs(get_current_config().role);
    else
        if verbose then
            echo("lole_buffcheck: requested missing/expiring buffs");
        end
        local desired_buffs = get_desired_buffs(get_current_config().role);
        for i,bname in ipairs(desired_buffs) do
            local bname_alias = BUFF_ALIASES[bname];
            
            if buffname_timeleft_map[bname] ~= nil then
                if buffname_timeleft_map[bname] < 180 then
                    table.insert(missing_table, bname);
                end
            elseif bname_alias ~= nil and buffname_timeleft_map[bname_alias] ~= nil then
                if buffname_timeleft_map[bname_alias] < 180 then
                    table.insert(missing_table, bname); -- still use base name
                end
            else
                table.insert(missing_table, bname);
            end
        end
    end

    if get_current_config().self_buffs ~= nil then
        SELF_BUFF_SPAM_TABLE = {};
        for i,bname in ipairs(get_current_config().self_buffs) do
            if arg == "clean" then
                if buffname_timeleft_map[bname] == 1000 then
                else 
                    table.insert(SELF_BUFF_SPAM_TABLE, bname);
                end
            else
                if buffname_timeleft_map[bname] ~= nil then
                    if buffname_timeleft_map[bname] < 180 then
                        table.insert(SELF_BUFF_SPAM_TABLE, bname);
                    end
                else
                    table.insert(SELF_BUFF_SPAM_TABLE, bname);
                end
            end
        end
    end
    
    local msgstr = table.concat(missing_table, ",");
     
    SendAddonMessage("lole_buffs", msgstr, "RAID", UnitName("player"));
    LAST_BUFF_CHECK = time();
    BUFFS_CHECKED = true;

end

function get_num_buff_requests(buffs)

	-- buffs is a "buffname" -> "table of character names with this particular buff request" map

	local requests = {}
	local request_amount = 0;

    for buff, chars in pairs(buffs) do
        for char in pairs(chars) do
            if not requests[char] then
                requests[char] = true;
                request_amount = request_amount + 1;
            end
        end
    end
	
	return request_amount;

end

function get_spam_table(buffs, group_buff_map)
    
    local groups = {[1] = {}, [2] = {}};
    if GetNumRaidMembers() == 0 then
        for buff, chars in pairs(buffs) do
            for char in pairs(chars) do
                groups[1][char] = true;
            end
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if raid_info[3] == 1 then
                groups[1][raid_info[1]] = true;
            elseif raid_info[3] == 2 then
                groups[2][raid_info[1]] = true;
            end
            i = i + 1;
        end
    end

    local grouped_requests = {};
    for buff, chars in pairs(buffs) do
        grouped_requests[buff] = {[1] = {}, [2] = {}};
        for char in pairs(chars) do
            if groups[1][char] then
                table.insert(grouped_requests[buff][1], char);
            elseif groups[2][char] then
                table.insert(grouped_requests[buff][2], char);
            end
        end
    end

    local spam_table = {};
    for buff, groups in pairs(grouped_requests) do
        for group, chars in pairs(groups) do
            if #chars > 2 and group_buff_map[buff] then
                table.insert(spam_table, {[chars[1]] = group_buff_map[buff]});
            else
                for key in pairs(chars) do
                    table.insert(spam_table, {[chars[key]] = buff});
                end
            end
        end
    end

    return spam_table;

end

function get_paladin_spam_table(buffs, num_requests)

    local buff_order = {
        [1] = "Greater Blessing of Salvation",
        [2] = "Greater Blessing of Kings",
        [3] = "Greater Blessing of Wisdom",
        [4] = "Greater Blessing of Might",
    };

    local spam_table = {};
    local buffed_characters = {};
    local buffed_classes = {};
    if num_requests > 4 then
        for key, buff in ipairs(buff_order) do
            local buff_given = {};
            local chars = buffs[buff];
            if chars then
                for character in pairs(chars) do
                    local class = UnitClass(character);
                    if buff_given[class] then
                        buffed_characters[character] = true;
                    elseif not buffed_classes[class] then
                        table.insert(spam_table, {[character] = buff});
                        buffed_characters[character] = true;
                        buff_given[class] = true;
                        buffed_classes[class] = true;
                    end
                end
            end
        end
    end

    for key, buff in ipairs(buff_order) do
        local chars = buffs[buff];
        if chars then
            local temp = {strsplit(" ", buff)};
            table.remove(temp, 1);
            local short_buff = table.concat(temp, " ");
            for character in pairs(chars) do
                if not buffed_characters[character] then
                    table.insert(spam_table, {[character] = short_buff});
                    buffed_characters[character] = true;
                end
            end
        end
    end

    return spam_table;

end

function buffs()

    if SPAM_TABLE[1] ~= nil then
        if (GetTime() - BUFF_TIME) < 1.8 then
            return false;
        else
            local char, buff = next(SPAM_TABLE[1]);
            CastSpellByName(buff, char);
            BUFF_TIME = GetTime();
            table.remove(SPAM_TABLE, 1);
        end
    elseif SELF_BUFF_SPAM_TABLE[1] ~= nil then
        buff_self();
    else
        MISSING_BUFFS = {};
        lole_subcommands.set("playermode", 0);
        LBUFFCHECK_ISSUED = false;
    end

end

function buff_self()

    if (GetTime() - BUFF_TIME) < 1.8 then
        return false;
    else
        CastSpellByName(SELF_BUFF_SPAM_TABLE[1]);
        BUFF_TIME = GetTime();
        table.remove(SELF_BUFF_SPAM_TABLE, 1);
    end

end

function lole_selfbuffs()
    if SELF_BUFF_SPAM_TABLE[1] ~= nil then
        buff_self();
    else
        lole_subcommands.set("playermode", 0);
    end
end

function get_num_paladins()

    local num_paladins = 0;

    if GetNumRaidMembers() == 0 then
        if UnitClass("player") == "Paladin" then
            num_paladins = num_paladins + 1;
        end
        local num_party_members = GetNumPartyMembers();
        for i = 1, num_party_members do
            if UnitClass("party" .. i) == "Paladin" then
                num_paladins = num_paladins + 1;
            end
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if raid_info[3] == 1 or raid_info[3] == 2 then
                if raid_info[5] == "Paladin" then
                    num_paladins = num_paladins + 1;
                end
            end
            i = i + 1;
        end
    end

    return num_paladins;

end
