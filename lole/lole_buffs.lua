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

    ["Shadow Protection"] = "Prayer of Shadow Protection",

    ["Blessing of Kings"] = "Greater Blessing of Kings",
--  ["Greater Blessing of Kings"] = "Blessing of Kings",

    ["Blessing of Wisdom"] = "Greater Blessing of Wisdom",
--  ["Greater Blessing of Wisdom"] = "Blessing of Wisdom",

    ["Blessing of Salvation"] = "Greater Blessing of Salvation",
--  ["Greater Blessing of Salvation"] = "Blessing of Salvation"

    ["Blessing of Might"] = "Greater Blessing of Might",
    
    ["Blessing of Light"] = "Greater Blessing of Light",
};

function get_desired_buffs(role)

    local common_buffs = {
        "Power Word: Fortitude",
        "Divine Spirit",
        "Mark of the Wild",
        --"Amplify Magic",
        "Shadow Protection",
    };

    local caster_buffs = {
        "Blessing of Salvation",
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Arcane Intellect",
    };

    local healer_buffs = {
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Blessing of Salvation",
        "Arcane Intellect",
    };

	local warrior_tank_buffs = {
		"Blessing of Kings",
        "Blessing of Might",
        "Blessing of Light",
        "Thorns"
	}

    local paladin_tank_buffs = {
        "Blessing of Kings",
        "Blessing of Wisdom",
        "Blessing of Light",
        "Arcane Intellect",
        "Thorns"
    }

    local melee_buffs = {
        "Blessing of Salvation",
        "Blessing of Kings",
        "Blessing of Might",
    }

    local mana_melee_buffs = {
        "Blessing of Salvation",
        "Blessing of Kings",
        "Blessing of Might",
        "Arcane Intellect"
    }

    local desired_buffs;
    if role == ROLES.caster then
        desired_buffs = caster_buffs;
    elseif role == ROLES.healer then
        desired_buffs = healer_buffs;
    elseif role == ROLES.warrior_tank then
        desired_buffs = warrior_tank_buffs;
    elseif role == ROLES.paladin_tank then
        desired_buffs = paladin_tank_buffs;
    elseif role == ROLES.melee then
        desired_buffs = melee_buffs;
    elseif role == ROLES.mana_melee then
        desired_buffs = mana_melee_buffs;
    else
        return {};
    end

    paladins = get_chars_of_class("Paladin");
    if #paladins == 1 then
        table.remove(desired_buffs, 3);
        table.remove(desired_buffs, 2);
    elseif #paladins == 2 then
        table.remove(desired_buffs, 3);
    end

    for key, buff in pairs(common_buffs) do
        table.insert(desired_buffs, buff);
    end

    return desired_buffs;
end

function lole_buffs()
    if lole_subcommands.get("selfbuffmode") == 1 then
        lole_selfbuffs();
    else
        local missing_buffs_copy;
        if not BUFF_TABLE_READY then
            missing_buffs_copy = shallowcopy(MISSING_BUFFS);
        end
		lole_subcommands.buffs(missing_buffs_copy);
    end
end

function lole_leaderbuffcheck(arg)

    if arg and arg ~= "clean" then 
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
    SendAddonMessage("lole_buffcheck", msgstr, "RAID");

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

    SendAddonMessage("lole_buffs", msgstr, "RAID");
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

    local groups = {[1] = {}};
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
            if not groups[raid_info[3]] then
                groups[raid_info[3]] = {[raid_info[1]] = true};
            else
                groups[raid_info[3]][raid_info[1]] = true;
            end
            i = i + 1;
        end
    end

    local grouped_requests = {};
    for buff, chars in pairs(buffs) do
        grouped_requests[buff] = {};
        for grp, tbl in pairs(groups) do
            table.insert(grouped_requests[buff], grp, {});
        end
        for char in pairs(chars) do
            for grp, tbl in pairs(groups) do
                if groups[grp][char] then
                    table.insert(grouped_requests[buff][grp], char);
                    break;
                end
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
        [5] = "Greater Blessing of Light",
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
                    if class == nil then
                        -- character has left the party
                    elseif buff_given[class] then
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
        lole_subcommands.set("buffmode", 0);
    end
end

function get_chars_of_class(class)

    local chars = {};

    if GetNumRaidMembers() == 0 then
        if UnitClass("player") == class then
            local name = UnitName("player");
            table.insert(chars, name);
        end
        local num_party_members = GetNumPartyMembers();
        for i = 1, num_party_members do
            if UnitClass("party" .. i) == class then
                local name = UnitName("party" .. i);
                table.insert(chars, name);
            end
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            local instance = GetRealZoneText();
            -- Don't account for chars outside groups 1 and 2 when in Kara or ZA.
            if (instance == "Karazhan" or instance == "Zul'Aman") and raid_info[3] > 2 then
            elseif raid_info[5] == class then
                table.insert(chars, raid_info[1]);
            end
            i = i + 1;
        end
    end

    return chars;

end

function need_to_buff()

    local self_name = UnitName("player");
    local self_class = UnitClass("player");
    local colleagues = {};

    if self_class == "Paladin" or get_current_config().name == "priest_holy_ds" then
        return true;
    elseif self_class == "Druid" then
        colleagues = {"Kusip", "Gawk", "Teline"};
    elseif self_class == "Mage" then
        colleagues = {"Dissona", "Consona"};
    elseif self_class == "Priest" then
        colleagues = {"Kasio", "Bogomips", "Pussu", "Mam"}; -- The one with Divine Spirit should be last.
    else
        return false;
    end

    local present_colleagues = get_chars_of_class(self_class);    
    for i, char in ipairs(colleagues) do
        if table.contains(present_colleagues, char) then
            if char == self_name then
                return true;
            else
                return false;
            end
        end
    end

    return true

end
