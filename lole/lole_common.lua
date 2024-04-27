-- globals

vec3 = {}
vec3.__index = vec3

function vec3:create(x, y, z)
   local v = {}             -- our new object
   setmetatable(v,vec3)
   v.x = x      -- initialize our object
   v.y = y
   v.z = z
   return v
end

function vec3:rotated2d(angle)
  return vec3:create(self.x * math.cos(angle) - self.y * math.sin(angle), self.x * math.sin(angle) + self.y * math.cos(angle), 0)
end

function vec3:length()
  return math.sqrt(self.x*self.x + self.y*self.y + self.z*self.z)
end

function vec3:subtract(other)
  return vec3:create(self.x - other.x, self.y - other.y, self.z - other.z)
end

function vec3:add(other)
  return vec3:create(self.x + other.x, self.y + other.y, self.z + other.z)
end

function vec3:distance(other)
  return self:subtract(other):length()
end

function vec3:unit()
  local len = self:length()
  return vec3:create(self.x / len, self.y / len, self.z / len)
end

function vec3:unit_scaled(s)
  return self:unit():scale(s)
end

function vec3:scale2d(s)
  return vec3:create(self.x * s, self.y * s, self.z)
end

function vec3:direction_angle()
  local u = self:unit()
  return math.atan2(u.y, u.x);
end

function vec3:scale(s)
  return vec3:create(s * self.x, s * self.y, s * self.z)
end

function vec3:sublength(s)
  return self:subtract(s):length()
end

function get_arg_table(...)
  -- NOTE: THIS CONVERTS EVERYTHING TO STRINGS!
  local atab = {};
  if select('#', ...) < 1 then
    return atab;
  end

  for i = 1, select('#', ...) do
      local arg = select(i, ...);
      table.insert(atab, tostring(arg));
  end
  return atab
end

function concatenate_args(separator, ...)
  -- NOTE: this assumes that all the elements are actually strings
  local atab = get_arg_table(...)
  if tablelength(atab) < 1 then
    return nil
  else
    return table.concat(atab, separator);
  end
end

local ESSENCE_CLICK_TIME = 0

function run_to_essenceportal_and_click(name)

  if playermode() then return true end

  if GetTime() - ESSENCE_CLICK_TIME < 1.5 then
    return true
  end

  local GUID, success = interact_with_spellnpc(name)
  if not GUID then return false end

  if not success then
    local x, y, z = get_unit_position(GUID)
    echo("spellnpc interaction: walking to object " .. GUID .. ' at (' .. tostring(x) .. ", " .. tostring(y) .. ", " .. tostring(z) .. ")")
    walk_to(x, y, z, CtmPrio.ClearHold)
  else
    echo("successfully clicked " .. name .. "!")
    ESSENCE_CLICK_TIME = GetTime()
  end
end

local function LEGION_FLAME_AVOID()
  if avoid_npc_with_name("Legion Flame", 8) then
    local dpos = TOC_middle:subtract(get_unit_position("player"))
    if dpos:length() < 10 then
      dpos = dpos:unit_scaled(15)
    end
    local walk_pos = TOC_middle:add(dpos:rotated2d(6.28/6))
    walk_to(walk_pos.x, walk_pos.y, walk_pos.z, CtmPrio.Follow)
  end
end


local green_warn_sent = 0
local red_warn_sent = 0

local function putricide_stuff()

  if UnitName("player") == "Kuratorn" then
    if not UnitAffectingCombat("Kuratorn") then
      red_warn_sent = 0
      green_warn_sent = 0
      return
    end

    L_ClearTarget()
    L_TargetUnit("Gas Cloud")
    if UnitExists("target") then
      if red_warn_sent == 0 then
        SendChatMessage("RED slimu is targetting " .. tostring(UnitName("targettarget")) .. ". RUN!!!", "GUILD")
        red_warn_sent = 1
        green_warn_sent = 0
        return
      end
    end

    L_ClearTarget()
    L_TargetUnit("Volatile Ooze")
    if UnitExists("target") then
      if green_warn_sent == 0 then
        SendChatMessage("GREEN slimu is targetting " .. tostring(UnitName("targettarget")) .. "!", "GUILD")
        green_warn_sent = 1
        red_warn_sent = 0
        return
      end
    end

    L_TargetUnit("focus")
  end
end

local ooze_avoid_level = 0

local function putricide_ooze_avoid()
  if has_debuff("player", "Gaseous Bloat") then
    local ppos = vec3:create(get_unit_position("player"))
    local opos = vec3:create(get_unit_position("target"))
    if ooze_avoid_level == 0 then
      local wp0 = vec3:create(4404.2, 3224.8, 389.4)
      local dwp = wp0:sublength(ppos)

      if dwp > 2 then
        walk_to(wp0.x, wp0.y, wp0.z, CtmPrio.NoOverride)
      end

      local dl = opos:sublength(ppos)
      if (dl < 15) then
        ooze_avoid_level = 1
      end

    elseif ooze_avoid_level == 1 then
      local wp1 = vec3:create(4403.6, 3199.8, 389.4)
      walk_to(wp1.x, wp1.y, wp1.z, CtmPrio.NoOverride)

      local dl = wp1:sublength(ppos)
      if (dl < 3) then
        ooze_avoid_level = 2
      end

    elseif ooze_avoid_level == 2 then
      local wp2 = vec3:create(4366.8, 3165.6, 389.4)
      walk_to(wp2.x, wp2.y, wp2.z, CtmPrio.Follow)
    end
  else
    ooze_avoid_level = 0
  end
end

local function blast_cannons()
    if UnitAffectingCombat("player") then
      local n = UnitName("player")
      if (n == "Iijj" or n == "Spobodi") then -- or n == "Kuratorn") then
          if UnitInVehicle("player") then
            L_RunScript("if VehicleMenuBarPowerBar.currValue > 95 then VehicleMenuBarActionButton2:Click() else VehicleMenuBarActionButton1:Click() end")
          end
      end
    end
end

local REMOVE_THIS_FRAME = CreateFrame("frame", nil, UIParent)
REMOVE_THIS_FRAME:SetScript("OnUpdate",

function()

  blast_cannons()

  if not playermode() then

    -- if validate_target() then
    --   caster_range_check(0, 36)
    -- end

    -- this is for gunship

    if unit_castorchannel("focus") == "Staggering Stomp" then L_SpellStopCasting(); return; end

  --if UnitCastingInfo("target") == "Lightning Nova" then
    --  walk_to(-219, -235, 97, CTM_PRIO_CLEAR_HOLD) -- the coords are for emalon :D
    -- if UnitCastingInfo("target") == "Poison Nova" then
    --     walk_to(814, 92, 509, CTM_PRIO_FOLLOW) -- these are for ICK
    -- end

    --if unit_castorchannel("target") == "Blade Tempest" then
      -- if get_distance_between("player", "target") < 15 then
      --    walk_to(3238, 399, 78, CTM_PRIO_FOLLOW) -- these are for BALTHARUS
      --  end
    -- end

    -- THIS IS GOLDEN STUFF: ----------------------
    if UnitAffectingCombat("player") then
      local n = UnitName("player")
      if not (n == "Iijj" or n == "Spobodi") then -- or n == "Kuratorn") then
        hconfig("status")
      end

      putricide_ooze_avoid()

    end
    --------------------------------------------


  end

  if not playermode() then

  --  LEGION_FLAME_AVOID()
  --  boss_action("Gormok")

  end

end
)

LAST_SPELL_ERROR = nil
LAST_SPELL_ERROR_TIME = 0
LAST_SPELL_ERROR_TEXT = ""

TOC_middle = vec3:create(562, 137, 395) -- not "local" because lole.lua needs this

REMOVE_THIS_FRAME:RegisterEvent("MINIMAP_PING")
REMOVE_THIS_FRAME:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)

  if event == "MINIMAP_PING" then
    -- THIS IS FOR TOC ONLY!!

    local from = prefix
    if (from ~= "player") then
      return
    end


    local x = message
    local y = channel

    -- left side is 561, 186; center 562, 137
    -- and minimap left is -0.25

    local ppos = vec3:create(get_unit_position(UnitName("player")))
    local world_pos = vec3:create(ppos.x + (200 * y), ppos.y + (-200 * x), ppos.z)

    SendAddonMessage("lole_avoid_coords", tostring(world_pos.x) .. "," .. tostring(world_pos.y) .. "," .. tostring(world_pos.z), "RAID")

  end

end)





NOTARGET = "0x0000000000000000";

BLAST_TARGET_GUID = "0x0000000000000000";
MISSING_BUFFS = {};
OVERRIDE_COMMAND = nil;

HEALERS = {"Bacc"}; -- for keeping order mostly
DEFAULT_HEALER_TARGETS = {
  Bacc = {heals={"raid"}, hots={"Raimo"}}
}
ASSIGNMENT_DOMAINS = {"heals", "hots", "ignores"};
HEALS_IN_PROGRESS = {};
HEAL_FINISH_INFO = {};
HEAL_ATTEMPTS = 0;
MAX_HEAL_ATTEMPTS = 5;
UNREACHABLE_TARGETS = {};
CH_BOUNCE_1 = nil;
CH_BOUNCE_2 = nil;
POH_TARGETS = {};

-- Healers select targets based on the function shown here:
-- http://www.wolframalpha.com/input/?i=plot+of+y%3D(x%2F0.5)%5E(15000%2F10000)+and+y%3D(x%2F0.5)%5E(15000%2F12500)+and+y%3D(x%2F0.5)%5E(15000%2F15000)++for+x%3D0+to1+
-- This value defines the point where HP deficit percentages are considered equal when healers compare targets.
-- The idea is to favor targets with low max HP at deficit percentages higher than this point.
-- At lower deficit percentages, targets with high max HP are favored.
-- Adjust this value to shift healers' targeting priorities.
POINT_DEFICITS_EQUAL = 0.5;

HEAL_ESTIMATES = {
    ["Flash of Light"] = 2800,
    ["Holy Light"] = 8600,
    ["Lesser Healing Wave"] = 4400,
    ["Healing Wave"] = 9200,
    ["Chain Heal"] = 4600,
    ["Regrowth"] = 6400, -- a few ticks included
    ["Healing Touch"] = 10600,
    ["Nourish"] = 5000,
    ["Flash Heal"] = 4600,
    ["Greater Heal"] = 10000,
    ["Prayer of Healing"] = 4600,
    ["Binding Heal"] = 4000,
}

INSTANT_HEALS = {
    ["Holy Shock"] = true,
    ["Earth Shield"] = true,
    ["Riptide"] = true,
    ["Power Word: Shield"] = true,
    ["Circle of Healing"] = true,
    ["Renew"] = true,
    ["Prayer of Mending"] = true,
    ["Lifebloom"] = true,
    ["Rejuvenation"] = true,
    ["Swiftmend"] = true,
    ["Wild Growth"] = true,
}

ROLES = { healer = 1, caster = 2, tank = 3, mana_tank = 4, melee = 5, mana_melee = 6 }

-- http://wowwiki.wikia.com/wiki/Class_colors

local CLASS_COLORS = {
    death_knight = "C41F3B",
    druid = "FF7D0A",
    hunter = "ABD473",
    mage = "69CCF0",
    paladin = "F58CBA",
    priest = "FFFFFF",
    rogue = "FFF569",
    shaman = "0070DE",
    warlock = "9482C9",
    warrior = "C79C6E",
}

function get_class_color(class)
	local r = CLASS_COLORS[string.lower(class)]
	if not r then
		return "(ERR)"
	else
		return r
	end
end

local raid_target_indices = {

["star"] = 1,
["circle"] = 2,
["diamond"] = 3,
["triangle"] = 4,
["crescent"] = 5,
["moon"] = 5,
["square"] = 6,
["cross"] = 7,
["skull"] = 8,

}

function get_marker_index(marker)
	return raid_target_indices[string.lower(marker)]
end

local CC_spells = {
  -- full ranks
	polymorph = 118,
	sheep = 118,
	cyclone = 33786,
	roots = 26989,
	root = 26989,
	banish = 18647,
	ban = 18647,
	fear = 6215,
	shackle = 10955,
	turn = 10326,
  sleep = 18658,

}


local CC_spellnames = { -- in a L_CastSpellByName-able format
	[118] = "Polymorph",
	[33786] = "Cyclone",
	[26989] = "Entangling Roots",
	[18647] = "Banish",
	[6215] = "Fear",
	[10955] = "Shackle Undead",
	[10326] = "Turn Evil",
  [18658] = "Hibernate"
}

local AOE_spellIDs = {
  ["Flamestrike"] = 42926,
  ["Blizzard"] = 42940,
  ["Volley(Rank 2)"] = 14294,
  ["Volley"] = 27022,
  ["Hurricane"] = 48467,
  ["DND"] = 49938,
}

function get_AOE_spellID(name)
  return AOE_spellIDs[name]
end

function get_CC_spellID(name)
	return CC_spells[string.lower(name)];
end

function get_CC_spellname(spellID)
	return CC_spellnames[tonumber(spellID)];
end

function echo_noprefix(...)
    local t = concatenate_args(", ", ...)
    DEFAULT_CHAT_FRAME:AddMessage(t)
end

function echo(...)
    local t = concatenate_args(", ", ...)
    DEFAULT_CHAT_FRAME:AddMessage("lole: " .. t)
end

function lole_error(text)
	DEFAULT_CHAT_FRAME:AddMessage("|cFFFF3300lole: error: " .. tostring(text))
end

function player_casting()
  return unit_castorchannel("player")
end


function unit_castorchannel(unit)
  return UnitCastingInfo(unit) or UnitChannelInfo(unit)
end

function shallowcopy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in pairs(orig) do
            copy[orig_key] = orig_value
        end
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function deepcopy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in next, orig, nil do
            copy[deepcopy(orig_key)] = deepcopy(orig_value)
        end
        setmetatable(copy, deepcopy(getmetatable(orig)))
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

HEALER_TARGETS = deepcopy(DEFAULT_HEALER_TARGETS);

function first_to_upper(str)
    return (str:gsub("^%l", string.upper))
end

function table.val_to_str ( v )
  if "string" == type( v ) then
    v = string.gsub( v, "\n", "\\n" )
    if string.match( string.gsub(v,"[^'\"]",""), '^"+$' ) then
      return "'" .. v .. "'"
    end
    return '"' .. string.gsub(v,'"', '\\"' ) .. '"'
  else
    return "table" == type( v ) and table.tostring( v ) or
      tostring( v )
  end
end

function table.key_to_str ( k )
  if "string" == type( k ) and string.match( k, "^[_%a][_%a%d]*$" ) then
    return k
  else
    return "[" .. table.val_to_str( k ) .. "]"
  end
end

function table.tostring( tbl )
  local result, done = {}, {}
  for k, v in ipairs( tbl ) do
    table.insert( result, table.val_to_str( v ) )
    done[ k ] = true
  end
  for k, v in pairs( tbl ) do
    if not done[ k ] then
      table.insert( result,
        table.key_to_str( k ) .. "=" .. table.val_to_str( v ) )
    end
  end
  return "{" .. table.concat( result, "," ) .. "}"
end

function table_print (tt, indent, done)
  done = done or {}
  indent = indent or 0
  if type(tt) == "table" then
    local sb = {}
    for key, value in pairs (tt) do
      table.insert(sb, string.rep (" ", indent)) -- indent it
      if type (value) == "table" and not done [value] then
        done [value] = true
        table.insert(sb, key .. " = {\n");
        table.insert(sb, table_print (value, indent + 2, done))
        table.insert(sb, string.rep (" ", indent)) -- indent it
        table.insert(sb, "}\n");
      elseif "number" == type(key) then
        table.insert(sb, string.format("\"%s\"\n", tostring(value)))
      else
        table.insert(sb, string.format(
            "%s = \"%s\"\n", tostring (key), tostring(value)))
       end
    end
    return table.concat(sb)
  else
    return tt .. "\n"
  end
end

function to_string( tbl )
    if  "nil"       == type( tbl ) then
        return tostring(nil)
    elseif  "table" == type( tbl ) then
        return table_print(tbl)
    elseif  "string" == type( tbl ) then
        return tbl
    else
        return tostring(tbl)
    end
end

function table.contains(table, element)
  for _, value in pairs(table) do
    if value == element then
      return true
    end
  end
  return false
end

function table.remove_duplicates(table)
    local hash = {}
    local set = {}
    for _,v in ipairs(table) do
       if (not hash[v]) then
           set[#set+1] = v
           hash[v] = true
       end
    end
    return set;
end

function table.get_num_elements(table)
    local num = 0;
    for key, val in pairs(table) do
        num = num + 1;
    end
    return num;
end

function table.rid(tbl, value)
    local new_tbl = {};
    for i, val in ipairs(tbl) do
        if val ~= value then
            table.insert(new_tbl, val);
        end
    end
    return new_tbl;
end

function starts_with(str, start)
   return str:sub(1, #start) == start
end

function ends_with(str, ending)
   return ending == "" or str:sub(-#ending) == ending
end

function get_available_class_configs()
	return get_list_of_keys(available_configs)
end

function get_available_class_configs_pretty()
	local key_tab, n = {}, 1;

	for name, _ in pairs_by_key(get_available_configs()) do
		key_tab[n] = get_config_name_with_color(name);
		n = n + 1;
	end

	return table.concat(key_tab, ", ");

end


function get_mode_attribs()
	return get_list_of_keys(lole_subcommands.get())
end


function get_available_subcommands()
	return get_list_of_keys(lole_subcommands);
end

function get_config_name_with_color(arg_config)
	if arg_config == "default" then
		return "|r|rdefault";
	else
		local avconf = get_available_configs()
		return "|cFF" .. avconf[arg_config].color .. avconf[arg_config].name .. "|r";
	end

end

function track_heal_attempts(name)
    if not name then return end
    local msgid, _ = get_last_spell_error()
    if msgid ~= nil then
        local fail_msg = SPELL_ERROR_TEXTS[msgid];
        HEAL_ATTEMPTS = HEAL_ATTEMPTS + 1;
        if HEAL_ATTEMPTS == MAX_HEAL_ATTEMPTS then
            HEAL_ATTEMPTS = 0;
            if UNREACHABLE_TARGETS[name] + 5 < GetTime() then
                print(string.format("%s to the penalty box for 5 sec: %s", name, fail_msg))
            end
            UNREACHABLE_TARGETS[name] = GetTime() + 5;
        end
    end
end

function cast_if_nocd(spellname, rank)
	if GetSpellCooldown(spellname) == 0 then
		L_CastSpellByName(spellname);
        if INSTANT_HEALS[spellname] or HEAL_ESTIMATES[spellname] or (rank and HEAL_ESTIMATES[spellname.."("..rank..")"]) then
            track_heal_attempts(UnitName("target"));
        end
		return true;
	end

	return false;
end

function off_cd(spellname)
    return GetSpellCooldown(spellname) == 0
end

function cast_spell(spellname)
	local name, rank, icon, cost, isFunnel, powerType, castTime, minRange, maxRange = GetSpellInfo(spellname);
	cast_state = { true, GetTime(), castTime, UnitName("target") };
	cast_if_nocd(spellname, rank);
end

function cast_heal(spellname, target, range)

    if range == nil then range = 35; end
    if target then L_TargetUnit(target); end
    if spellname ~= "Prayer of Healing" then
        healer_range_check(range);
    end
    if INSTANT_HEALS[spellname] then
        return cast_if_nocd(spellname);
    else
        stopfollow()
        cast_spell(spellname);
    end
end

function get_HP_deficits(party_only, with_heals)
	local HP_deficits = {};
    local heals_in_progress = shallowcopy(HEALS_IN_PROGRESS);

    local num_raid_members;
    if party_only then
        num_raid_members = 0;
    else
        num_raid_members = GetNumRaidMembers();
    end

	if num_raid_members == 0 then
	    HP_deficits[UnitName("player")] = UnitHealthMax("player") - UnitHealth("player");
	    for i=1,4,1 do local exists = GetPartyMember(i)
            local name = UnitName("party" .. i);
            if exists and UnitIsConnected(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) and UNREACHABLE_TARGETS[name] < GetTime() then
                local hp = UnitHealth(name);
                if with_heals then
                    for healer, info in pairs(heals_in_progress[name]) do
                        if info[2] > GetTime()*1000 then
                            hp = hp + info[1];
                        end
                    end
                end
            	HP_deficits[name] = UnitHealthMax(name) - hp;
            end
	    end
	else
		for i=1,num_raid_members,1 do
			local name = UnitName("raid" .. i);
			if UnitExists(name) and UnitIsConnected(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) and UNREACHABLE_TARGETS[name] < GetTime() then
                local hp = UnitHealth(name);
                if with_heals then
                    for healer, info in pairs(heals_in_progress[name]) do
                        if info[2] > GetTime()*1000 then
                            hp = hp + info[1];
                        end
                    end
                end
                HP_deficits[name] = UnitHealthMax(name) - hp;
			end
		end
	end

	return HP_deficits;

end

function get_lowest_hp(hp_deficits)

	local lowest = nil;
	local lowest_deficit = 0;

	for name,hp_deficit in pairs(hp_deficits) do

		if not lowest then
			lowest = name
			lowest_deficit = hp_deficit
		else
			if hp_deficit > hp_deficits[lowest] then
				lowest = name
				lowest_deficit = hp_deficit
			end
		end
	end

	return lowest, lowest_deficit;

end

function get_HP_table_and_maxmaxHP(party_only)
    local HP_table = {};
    local num_raid_members = 0;

    if party_only then
        num_raid_members = 0;
    else
        num_raid_members = GetNumRaidMembers();
    end

    local maxmaxHP = 0;
    if num_raid_members == 0 then
        HP_table[UnitName("player")] = {UnitHealth("player"), UnitHealthMax("player")};
        for i=1,4,1 do local exists = GetPartyMember(i)
            local name = UnitName("party" .. i);
            if exists and UnitIsConnected(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) and UNREACHABLE_TARGETS[name] < GetTime() and is_valid_pair(UnitName("player"), name) then
                HP_table[name] = {UnitHealth(name), UnitHealthMax(name)};
                if HP_table[name][2] > maxmaxHP then
                    maxmaxHP = HP_table[name][2];
                end
            end
        end
    else
        for i=1,num_raid_members,1 do
            local name = UnitName("raid" .. tonumber(i));
            local unr = UNREACHABLE_TARGETS[name]
            if UnitExists(name) and UnitIsConnected(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) and (unr ~= nil) and unr < GetTime() and is_valid_pair(UnitName("player"), name) then
                HP_table[name] = {UnitHealth(name), UnitHealthMax(name)};
                if HP_table[name][2] > maxmaxHP then
                    maxmaxHP = HP_table[name][2];
                end
            end
        end
    end

    return HP_table, maxmaxHP;

end

local CS_CASTING, CS_TIMESTAMP, CS_CASTTIME, CS_TARGET = 1, 2, 3, 4;
local NOT_CASTING = { false, 0.0, 0.0, "none" };

local cast_state = NOT_CASTING;

function casting_legit_heal()

	if UnitCastingInfo("player") then return true; end

	if cast_state[CS_CASTING] then
		if (GetTime() - cast_state[CS_TIMESTAMP])*1000 > (cast_state[CS_CASTTIME]+100) then
			cast_state = NOT_CASTING;
			return false;

		elseif not UnitCastingInfo("player") then
			cast_state = NOT_CASTING;
			return false;

		elseif health_percentage("target") > 90 then
			stopfollow();
			cast_state = NOT_CASTING; -- useful when the UnitHealth info lag causes the char to overheal (or any cause)
			return false;

		else return true; end
	end

	return false;

end

function cleanse_party(debuffname)
	for i=1,5,1 do
    local exists = true;
    local name = "party" .. i;
    if i == 5 then
        name = "player"
    else
        exists = GetPartyMember(i)
    end
    if exists and has_debuff(name, debuffname) then
	    L_TargetUnit(name);
      L_CastSpellByName("Cleanse");
			L_CastSpellByName("Dispel Magic")
      return true;
		end
	end
	return false;
end

function get_player_with_debuff(debuffname)
    num_raid_members = GetNumRaidMembers();
    if num_raid_members == 0 then
        for i=1,5,1 do
            local exists = true;
            local name = "party" .. i;
            if i == 5 then
                name = "player"
            else
                exists = GetPartyMember(i)
            end
            if exists and has_debuff(name, debuffname) then
                return name;
            end
        end
    else
        for i=1,num_raid_members,1 do
            local exists = true;
            local name = UnitName("raid" .. tonumber(i));
            if UnitExists(name) and has_debuff(name, debuffname) then
                return name;
            end
        end
    end
end

local function CAST_CLEANSE_ALL()
  L_CastSpellByName("Cleanse");
  L_CastSpellByName("Dispel Magic")
  L_CastSpellByName("Cleanse Spirit")
  L_CastSpellByName("Remove Curse")
end

function cleanse_raid(debuffname)
    num_raid_members = GetNumRaidMembers();
    if num_raid_members == 0 then
        for i=1,5,1 do
            local exists = true;
            local name = "party" .. i;
            if i == 5 then
                name = "player"
            else
                exists = GetPartyMember(i)
            end
            if exists and has_debuff(name, debuffname) then
                L_TargetUnit(name);
                CAST_CLEANSE_ALL()
                return true;
            end
        end
    else
        for i=1,num_raid_members,1 do
            local exists = true;
            local name = UnitName("raid" .. tonumber(i));
            if UnitExists(name) and has_debuff(name, debuffname) then
                L_TargetUnit(name);
                CAST_CLEANSE_ALL()
                return true;
            end
        end
    end
    return false;
end

function decurse_party(debuffname)
    for i=1,5,1 do
        local exists = true;
        local name = "party" .. i;
        if i == 5 then
            name = "player"
        else
            exists = GetPartyMember(i)
        end
        if exists and has_debuff(name, debuffname) then
            L_TargetUnit(name);
            L_CastSpellByName("Remove Curse");
            L_CastSpellByName("Remove Lesser Curse")
            return true;
        end
    end
    return false;
end


function table_empty(t)
  if t == nil then return true end
  return next(t) == nil
end

function update_table(target, source)
  -- only update values, don't destroy keys that already exist
  for k,v in pairs(source) do
    target[k] = v
  end
end

function match_any(tab, string)
  for k,v in pairs(tab) do
    if string == v then
      return true
    end
  end

  return nil
end

function get_raid_debuffs_by_type(debuff_types)

  local matching_debuffs = {}

  -- Wotlk:
  -- local _ua = UnitAura
  -- local unit_debuff = function(unit, idx)
  --   return _ua(unit, idx, "HARMFUL")
  -- end

  -- TBC:
  local unit_debuff = UnitDebuff
  
  local RM = GetRealNumRaidMembers()

  local dtypes = tokenize_string(debuff_types, ", ")

  for i=1,RM,1 do
    local rname = "raid" .. tostring(i)
    local debuffs = {}

    local a = 1
    local name,rank,_,_,type = unit_debuff(rname, a)
    while name do
      if match_any(dtypes, type) then
        table.insert(debuffs, name)
      end
      a = a + 1
      name,rank,_,_,type = unit_debuff(rname, a)
    end

    if not table_empty(debuffs) then
      matching_debuffs[UnitName(rname)] = debuffs
    end
  end

  return matching_debuffs

end

function cast_dispel(debuff_table, spellname)
-- TODO: cleansing totem is a very powerful spell tho...
  local charname = table_getkey_any(debuff_table)
  if not charname then return nil end
    -- we dont really care about the actual debuff, just cleanse, don't ask questions :D
  L_TargetUnit(charname)
  healer_range_check(40) -- most dispel spells have a 40 yd range?
  L_CastSpellByName(spellname)

  return true

end

function table_getkey_any(t)
  if not t or table_empty(t) then return nil end
  for k,_ in pairs(t) do return k end -- yeah this is so great :DDD
end



function has_buff(targetname, buff_name)

  local UB = UnitBuff

  local i = 1
	local name, _, _, count, _, duration, timeleft = UB(targetname, i)
	while name do
    if string.find(name, buff_name) then
      return true, timeleft, count
		end
    i = i + 1
    name, _, _, count, _, duration, timeleft = UB(targetname, i)
	end

  return nil

end


function has_debuff(targetname, debuff_name)

  local UD = UnitDebuff
  local i = 1
  local name, _, _, count, _, duration, expirationTime = UD(targetname,i)
	while name do
		if string.find(name, debuff_name) then
      local timeleft = expirationTime - GetTime()
      return true, timeleft, count, duration
		end
    i = i + 1
    name, _, _, count, _, duration, expirationTime = UD(targetname,i)
	end

	return nil

end

function has_debuff_by_self(targetname, debuff_name)
  local has, timeleft, count, duration = has_debuff(targetname, debuff_name)

  if has and duration then -- duration is nil for debuffs cast by others
    return true, timeleft, count
  end

  return nil
end


function get_self_debuff_expiration(targetname, debuff_name)
  local h, timeleft, count = has_debuff_by_self(targetname, debuff_name)
  if h then return timeleft end
  return nil
end

function debuff_left_percentage(targetname, debuff_name)
    local h, timeleft, count, duration = has_debuff(targetname, debuff_name)
    if h and timeleft and duration then
        return timeleft / duration * 100.0
    end
    return nil;
end

function get_num_debuff_stacks(targetname, debuff_name)

  local h, timeleft, count = has_debuff(targetname, debuff_name)

  if h then return count, timeleft end
	-- not debuffed with debuff_name
	return 0, 999;

end


local CC_jobs = {}
local num_CC_jobs = 0

function set_CC_job(spellID, marker)
	num_CC_jobs = num_CC_jobs + 1
	CC_jobs[marker] = spellID
end

function unset_CC_job(marker)
	num_CC_jobs = num_CC_jobs - 1
	CC_jobs[marker] = nil;
end

function unset_all_CC_jobs()
	CC_jobs = nil
end



function do_CC_jobs()

	for marker, spellID in pairs(CC_jobs) do
		target_unit_with_marker(marker);

		if UnitExists("target") and not UnitIsDead("target") then
      local spellname = get_CC_spellname(spellID)
			a, d = has_debuff("target", spellname)
			if not a then
				L_CastSpellByName(spellname)
				return true;
			elseif d < 3 then
				L_CastSpellByName(spellname)
				return true;
			end
		end
	end

	return false
end

function can_attack_target()
    return UnitCanAttack("player", "target") == 1 and
            not UnitIsDead("target")
end

function player_is_targeted()
    return UnitName("targettarget") == GetUnitName("player")
end

function validate_target()


	if BLAST_TARGET_GUID ~= NOTARGET and UnitExists("focus") and BLAST_TARGET_GUID == UnitGUID("focus") then
		if not UnitIsDead("focus") then
			if has_debuff("focus", "Polymorph") or has_debuff("focus", "Shackle") then
			 	return false;
			end

			L_TargetUnit("focus");
			return true;
		else
			clear_target()
			return false;
		end
	else
		return false;
	end

end

function get_distance_between(c1, c2)
  local x1,y1,z1 = get_unit_position(c1)
  if not x1 then return nil end

  local x2,y2,z2 = get_unit_position(c2)
  if not x2 then return nil end

  local x = x2-x1
  local y = y2-y1
  local z = z2-z1

  local length = math.sqrt(x*x + y*y + z*z)

  return length;

end

function get_int_from_strbool(strbool)
	local rval = -1;
	if strbool ~= nil then
		if strbool == "on" or strbool == "1" or strbool == 1 then
			rval = 1;
		elseif strbool == "off" or strbool == "0" or strbool == 0 then
			rval = 0;
		end
	end

	return rval;
end

function pairs_by_key(t, f)
	local a = {}
		for n in pairs(t) do table.insert(a, n) end
		table.sort(a, f)
		local i = 0      -- iterator variable
		local iter = function ()   -- iterator function
			i = i + 1
			if a[i] == nil then return nil
			else return a[i], t[a[i]]
			end
		end
	return iter
end

function get_list_of_keys(dict)
	if not dict or next(dict) == nil then
		return "-none-"
	end

	local key_tab, n = {}, 1;

	for k, _ in pairs_by_key(dict) do
		key_tab[n] = k;
		n = n + 1;
	end

	return table.concat(key_tab, ", "); -- alphabetical sort.

end

function tokenize_string(str, sep)
        local sep, fields = sep or ":", {}
        local pattern = string.format("([^%s]+)", sep)
        str:gsub(pattern, function(c) fields[#fields+1] = c end)
        return fields
end

function trim_string(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local guild_members = {
  Chonkki = 1,
  Bacc = 2,
  Boyee = 3,
  Setitboy = 4,
  Raimo = 5,
  Vomitonux = 6,
  Abych = 7,
  Nonankh = 8,
  Flaunor = 9,
  Ocdun = 10,
}

for name, num in pairs(guild_members) do
    HEALS_IN_PROGRESS[name] = {};
    UNREACHABLE_TARGETS[name] = 0;
end

function get_guild_members()
	return guild_members
end

function get_guild_members_list()
  local res = {}
  for name, _ in pairs(get_guild_members()) do
    table.insert(res, name)
  end
  return res
end

function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

function get_durability_status()
	for i=1,10,1 do
		local dur, max = GetInventoryItemDurability(i)
		if dur and dur == 0 then
			return false
		end
	end

	for i=16,18,1 do
		local dur, max = GetInventoryItemDurability(i)
		if dur and dur == 0 then
			return false
		end
	end

	return true
end

function player_class()
    return UnitClass("player")
end

function get_item_bag_position(itemLink)

    for bag = 0, NUM_BAG_SLOTS do
        for slot = 1, GetContainerNumSlots(bag) do
            if (GetContainerItemLink(bag, slot) == itemLink) then
                return bag, slot
            end
        end
    end

-- (else)
	return nil,nil

end

function in_party()
    if GetNumPartMembers() > 0 and GetNumRaidMembers() == 0 then
        return true;
    end
    return false;
end

function in_raid()
    if GetNumRaidMembers() > 0 then
        return true;
    end
    return false;
end

function in_party_or_raid()
    if GetNumPartyMembers() > 0 or GetNumRaidMembers() > 0 then
        return true
    end
    return false
end


function get_raid_groups()

    local groups = {[1] = {}};

    if GetNumRaidMembers() == 0 then
        local name = UnitName("player");
        table.insert(groups[1], name);
        local num_party_members = GetNumPartyMembers();
        for i = 1, num_party_members do
            local name = UnitName("party" .. i);
            table.insert(groups[1], name);
        end
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if not groups[raid_info[3]] then
                groups[raid_info[3]] = {};
            end
            table.insert(groups[raid_info[3]], raid_info[1]);
            i = i + 1;
        end
    end

    return groups;

end

function get_raid_HP_deficits_grouped(groups)

    local grouped_deficits = {}

    for grp, tbl in pairs(groups) do
        grouped_deficits[grp] = {};
        for i, name in pairs(tbl) do
            if UnitExists(name) and UnitIsConnected(name) and (not UnitIsDead(name)) and (not has_buff(name, "Spirit of Redemption")) then
                local deficit = UnitHealthMax(name) - UnitHealth(name);
                grouped_deficits[grp][name] = deficit;
            end
        end
    end

    return grouped_deficits;

end

function get_CoH_eligible_groups(groups, min_deficit, max_ineligible_chars, max_distance)

    -- Gieves [target] - {group members} pairs for each group with a healable target that
    -- has at least 3 healable targets within 18 yards of itself. If there are
    -- multiple such targets in a group, the one with the most nearby targets and
    -- and the shortest distance to the caster is chosen.

    if min_deficit == nil then min_deficit = 4000; end
    if max_ineligible_chars == nil then max_ineligible_chars = 1; end
    if max_distance == nil then max_distance = 15; end

    local heals_in_progress = shallowcopy(HEALS_IN_PROGRESS);
    local coh_groups = {};

    for grp, tbl in pairs(groups) do
        if #tbl >= (5 - max_ineligible_chars) then
            local group_candidates = {};
            local target_data;
            local ineligible_chars = {};
            local num_ineligible_chars = 5 - #tbl;
            for i, name in ipairs(tbl) do
                local hp = UnitHealth(name);
                for healer, info in pairs(heals_in_progress[name]) do
                    if info[2] > GetTime()*1000 then
                        hp = hp + info[1];
                    end
                end
                if (not UnitExists(name) or not UnitIsConnected(name) or UnitIsDead(name) or has_buff(name, "Spirit of Redemption")
                    or UnitHealthMax(name) - hp < min_deficit or UNREACHABLE_TARGETS[name] > GetTime()) then
                    ineligible_chars[name] = true;
                    num_ineligible_chars = num_ineligible_chars + 1;
                    if num_ineligible_chars > max_ineligible_chars then
                        break;
                    end
                else
                    group_candidates[name] = {name};
                    for j=i-1,1,-1 do
                        if not ineligible_chars[tbl[j]] then
                            local distance = get_distance_between(name, tbl[j]);
                            if distance and distance <= max_distance then
                                table.insert(group_candidates[name], tbl[j]);
                                table.insert(group_candidates[tbl[j]], name);
                            end
                        end
                    end
                end
            end
            if num_ineligible_chars <= max_ineligible_chars then
                for tar, grp_cand in pairs(group_candidates) do
                    if #grp_cand >= (5 - max_ineligible_chars) then
                        local dist_to_me = get_distance_between("player", tar);
                        if not target_data or #grp_cand > target_data[2] then
                            target_data = {tar, #grp_cand, dist_to_me};
                        elseif #grp_cand == target_data[2] and dist_to_me < target_data[3] then
                            target_data = {tar, #grp_cand, dist_to_me};
                        end
                    end
                end
                if target_data then
                    coh_groups[target_data[1]] = group_candidates[target_data[1]];
                end
            end
        end
    end

    return coh_groups;

end

function get_assigned_targets(healer)
    return shallowcopy(HEALER_TARGETS[healer]["heals"]);
end

function get_assigned_hottargets(healer)
    return shallowcopy(HEALER_TARGETS[healer]["hots"]);
end

function get_assigned_ignores(healer)
    return shallowcopy(HEALER_TARGETS[healer]["ignores"]);
end

function get_heal_target(HP_table, maxmaxHP, with_urgencies)

    local heals_in_progress = shallowcopy(HEALS_IN_PROGRESS);
    local most_urgent = nil;
    local highest_urgency = 0;
    local urgencies = {};
    for target, hp_data in pairs(HP_table) do
        local tar_hp = hp_data[1];
        local tar_maxhp = hp_data[2];
        for healer, info in pairs(heals_in_progress[target]) do
            if info[2] > GetTime()*1000 then
                tar_hp = tar_hp + info[1];
            end
        end
        if tar_hp < tar_maxhp then
            local urgency = (((tar_maxhp - tar_hp) / tar_maxhp) / POINT_DEFICITS_EQUAL) ^ (maxmaxHP / tar_maxhp);
            urgencies[target] = urgency;
            --echo(string.format("%f", urgency));
            if urgency > highest_urgency then
                highest_urgency = urgency;
                most_urgent = target;
            end
        end
    end

    if with_urgencies then
        return most_urgent, urgencies;
    else
        return most_urgent;
    end

end

function get_raid_heal_target(with_urgencies)
    local HP_table, maxmaxHP = get_HP_table_and_maxmaxHP();
    return get_heal_target(HP_table, maxmaxHP, with_urgencies);
end

function get_raid_heal_targets(urgencies, num_targets)

    -- Returns a table of healable raid members sorted in descending order of urgency.
    -- Limit number of elements to num_targets when passed.

    local ordered_targets = {};
    for name, urgency in pairs(urgencies) do
        local index = #ordered_targets + 1;
        for i, tar in ipairs(ordered_targets) do
            if urgency > urgencies[tar] then
                index = i;
                break;
            end
        end
        table.insert(ordered_targets, index, name);
    end

    if num_targets == nil then
        return ordered_targets;
    end

    local r_tbl = {};
    for i = 1, num_targets do
        table.insert(r_tbl, ordered_targets[i]);
    end

    return r_tbl;

end

function sorted_by_urgency(chars)

    local HP_table = {};
    local maxmaxHP = 0;
    for i, name in pairs(chars) do
        if name == "raid" or not UnitExists(name) or not UnitIsConnected(name) or UnitIsDead(name) or has_buff(name, "Spirit of Redemption") or UNREACHABLE_TARGETS[name] > GetTime() then
        else
            HP_table[name] = {UnitHealth(name), UnitHealthMax(name)};
            if HP_table[name][2] > maxmaxHP then
                maxmaxHP = HP_table[name][2];
            end
        end
    end

    local ordered_targets = {};
    local _, urgencies = get_heal_target(HP_table, maxmaxHP, true);
    for name, urgency in pairs(urgencies) do
        local index = #ordered_targets + 1;
        for i, tar in ipairs(ordered_targets) do
            if urgency > urgencies[tar] then
                index = i;
                break;
            end
        end
        table.insert(ordered_targets, index, name);
    end

    if table.contains(chars, "raid") then
        table.insert(ordered_targets, "raid");
    end

    return ordered_targets;

end

function sync_healer_targets_with_mine()

    local healer_targets = deepcopy(HEALER_TARGETS);

    local msg = "set;";
    for i, healer in ipairs(HEALERS) do
        msg = msg .. healer .. ":"
        for j, domain in ipairs(ASSIGNMENT_DOMAINS) do
            local targets = healer_targets[healer][domain];
            msg = msg .. "<" .. domain .. ">";
            for k, target in ipairs(targets) do
                msg = msg .. target
                if k ~= #targets then
                    msg = msg .. ","
                end
            end
        end
        if i ~= #HEALERS then
            msg = msg .. "."
        end
    end

    SendAddonMessage("lole_healers", msg, "RAID");

end

function get_new_healer_targets(domain, op, healer, new_targets)

    local targets = {};
    local old_targets = {};
    if domain == "ignores" then
        old_targets = get_assigned_ignores(healer);
    else
        old_targets = get_assigned_targets(healer);
    end

    if op == "set" then
        targets = new_targets;
    elseif op == "add" then
        targets = old_targets;
        for i, target in ipairs(new_targets) do
            table.insert(targets, target);
        end
    elseif op == "del" then
        for i, target in ipairs(old_targets) do
            if not table.contains(new_targets, target) then
                table.insert(targets, target);
            end
        end
    end

    targets = table.remove_duplicates(targets);

    local raid_i = nil;
    for i, target in ipairs(targets) do
        if target == "raid" then
            raid_i = i;
            break
        end
    end
    if raid_i then
        table.remove(targets, raid_i);
        if domain == "heals" then
            table.insert(targets, "raid") -- "raid" must be last
        end
    end

    return targets;

end

function get_healer_target_info()

    local info = "";
    for i, healer in ipairs(HEALERS) do
        info = info .. get_healer_assignments_msg(healer) .. "\n";
    end

    return info;

end

function get_healer_assignments_msg(healer)

    local msg = nil;
    local order = {"heals", "hots", "ignores"};
    local verbose = {
        heals = "Heals",
        hots = "HoT/ES",
        ignores = "Raid heals ignore",
    }
    local assignments = {
        heals = get_assigned_targets(healer),
        hots = get_assigned_hottargets(healer),
        ignores = get_assigned_ignores(healer),
    }
    for i, domain in ipairs(order) do
        local tmp_str = "";
        if next(assignments[domain]) then
            tmp_str = table.concat(assignments[domain], ",");
        end
        if tmp_str ~= "" then
            tmp_str = " " .. tmp_str;
        end
        if not msg then
            msg = verbose[domain] .. ":" .. tmp_str;
        else
            msg = msg .. " | " .. verbose[domain] .. ":" .. tmp_str;
        end
    end

    msg = "[" .. healer .. "] " .. msg;
    return msg;

end

function handle_healer_assignment(message)
    local msg_tbl = {strsplit(";", message)};
    local op = msg_tbl[1];
    local msg = msg_tbl[2];

    if op == "reset" then
        HEALER_TARGETS = deepcopy(DEFAULT_HEALER_TARGETS);
        echo("Healer assignments reset to default:\n" .. get_healer_target_info());
        return;
    elseif op == "wipe" then
        for i, healer in pairs(HEALERS) do
            for j, domain in pairs(ASSIGNMENT_DOMAINS) do
                HEALER_TARGETS[healer][domain] = {};
            end
        end
        echo("Wiped healer assignments:\n" .. get_healer_target_info());
        return;
    elseif op == "sync" then
        local sync_with = "raid leader";
        if msg then
            sync_with = msg;
            if msg == UnitName("player") then
                sync_healer_targets_with_mine();
            end
        elseif IsRaidLeader() then
            sync_healer_targets_with_mine();
        end
        echo("Syncing healer assignments with " .. sync_with .. "...");
        return;
    elseif op == "restore" then
        local sync_with = "raid leader";
        local is_restorer = false;
        local not_found = false;
        if msg then
            sync_with = msg;
            if msg == UnitName("player") then
                is_restorer = true;
            end
        elseif IsRaidLeader() then
            is_restorer = true;
        end
        if is_restorer then
            if LOLE_HEALER_TARGETS_SAVED then
                HEALER_TARGETS = deepcopy(LOLE_HEALER_TARGETS_SAVED);
                sync_healer_targets_with_mine();
            else
                not_found = true;
            end
        end
        echo("Restoring and syncing with " .. sync_with .. "'s saved healer assignments...");
        if not_found then
            lole_subcommands.echo("No saved healer assignments found on " .. sync_with .. ".");
        end
        return;
    end
    local per_healer = {strsplit(".", msg)};
    local new_assignments = {};
    echo("Healer assignments change:")
    for key, healer_targets in pairs(per_healer) do
        local ht = {strsplit(":", healer_targets)};
        local healer = ht[1];
        if not table.contains(HEALERS, healer) then
            echo("No such healer: " .. "'" .. healer .. "'");
            return;
        end
        new_assignments[healer] = {};
        for i, domain in pairs(ASSIGNMENT_DOMAINS) do
            new_assignments[healer][domain] = nil;
        end
        if ht[2] ~= "" then
            local assign_data = {};
            local tars_itars = table.rid({strsplit("<", ht[2])}, "");
            for i, tars in pairs(tars_itars) do
                local tmp = {strsplit(">", tars)};
                local tmp_tars = {};
                if tmp[2] then
                    tmp_tars = table.rid({strsplit(",", tmp[2])}, "");
                end
                assign_data[tmp[1]] = tmp_tars;
            end
            for domain, domain_targets in pairs(assign_data) do
                local guildies = get_guild_members();
                for i, target in ipairs(domain_targets) do
                    if not guildies[target] and not (target == "raid" and domain == "heals") then
                        echo("Not a valid target for " .. domain .. ": '" .. target .. "'");
                        return;
                    end
                end
                new_assignments[healer][domain] = get_new_healer_targets(domain, op, healer, domain_targets);
            end
        end
    end
    for i, healer in ipairs(HEALERS) do
        if new_assignments[healer] then
            for j, domain in pairs(ASSIGNMENT_DOMAINS) do
                if new_assignments[healer][domain] then
                    HEALER_TARGETS[healer][domain] = new_assignments[healer][domain];
                end
            end
            echo_noprefix("   " .. get_healer_assignments_msg(healer));
        end
    end

    LOLE_HEALER_TARGETS_SAVED = HEALER_TARGETS;

end

function get_serialized_heals()

    local serialized_heals = "";
    local heals_in_progress = shallowcopy(HEALS_IN_PROGRESS);
    local raid_members = get_raid_members();
    for i, target in ipairs(raid_members) do
        local target_heals = 0;
        if not UnitExists(target) or not UnitIsConnected(target) or UnitIsDead(target) or has_buff(target, "Spirit of Redemption") or UNREACHABLE_TARGETS[target] > GetTime() or not is_valid_pair(UnitName("player"), target) then
            target_heals = 100000; -- Dirty hack to make DLL backend not consider this target for Chain Heal.
        else
            local heals = heals_in_progress[target];
            for healer, info in pairs(heals) do
                if info[2] > GetTime()*1000 then
                    target_heals = target_heals + info[1];
                end
            end
        end
        if target_heals > 0 then
            if serialized_heals == "" then
                serialized_heals = target .. ":" .. target_heals;
            else
                serialized_heals = serialized_heals .. "," .. target .. ":" .. target_heals;
            end
        end
    end

    return serialized_heals;

end

function get_raid_members()

    local members = {};
    local raid_groups = get_raid_groups();
    for grp, tbl in pairs(raid_groups) do
        for i, member in ipairs(tbl) do
            table.insert(members, member);
        end
    end

    return members;

end

function get_group_members(grp_num)

    local raid_groups = get_raid_groups();
    return raid_groups[grp_num];

end

function get_group_number(name)

    if GetNumRaidMembers() == 0 then
        return 1;
    else
        local i = 1;
        while GetRaidRosterInfo(i) do
            local raid_info = {GetRaidRosterInfo(i)};
            if raid_info[1] == name then
                return raid_info[3];
            end
            i = i + 1;
        end
    end

    return nil;

end

function handle_CH_report(targets, healer)

    table.remove(targets, 1);
    local hfi = shallowcopy(HEAL_FINISH_INFO[healer]);
    for i, target in ipairs(targets) do
        local div = 2 ^ (i - 1);
        local heal_estimate = hfi[1] / div;
        if not hfi[2] then
            echo("hfi[2] is nil")
        end
        HEALS_IN_PROGRESS[target][healer] = {heal_estimate, hfi[2]};
    end

end

function on_gcd()
    local gcd_spells = {
        ["Death Knight"] = "Rune Strike",
        Druid = "Healing Touch",
        Hunter = "Aspect of the Hawk",
        Mage = "Fireball",
        Paladin = "Holy Light",
        Priest = "Lesser Heal",
        Rogue = "Sinister Strike",
        Shaman = "Healing Wave",
        Warlock = "Shadow Bolt",
        Warrior = "Battle Shout",
    };
    return GetSpellCooldown(gcd_spells[UnitClass("player")]) > 0;
end

local passes = 0;
function run_override()
    if not on_gcd() then
        if OVERRIDE_COMMAND == "cooldowns" then
            lole_subcommands.cooldowns();
            passes = passes + 1;
            if passes < 5 then
                return;
            else
                passes = 0;
            end
        else
            L_RunScript(OVERRIDE_COMMAND);
        end
        lole_subcommands.set("playermode", 0);
        OVERRIDE_COMMAND = nil;
    end
end

function is_valid_pair(healer, target)

	local healer_ignores = get_assigned_ignores(healer);
	if healer_ignores and table.contains(healer_ignores, target) then
		return false;
	end

	return true;

end

-- just a convenience function

function playermode()
  if lole_subcommands.get("playermode") == 1 then
    return 1
  else
    return nil
  end
end

function health_percentage(unitname)
  return UnitHealth(unitname)/UnitHealthMax(unitname) * 100
end

function mana_percentage(unitname)
  return UnitMana(unitname)/UnitManaMax(unitname) * 100
end

local function bitoper(a, b, oper)
   local r, m, s = 0, 2^52
   repeat
      s,a,b = a+b+m, a%m, b%m
      r,m = r + m*oper%(s-a-b), m/2
   until m < 1
   return r
end

BITWISE = { 
    OR=function(a,b) return bitoper(a,b,1) end,
    XOR=function(a,b) return bitoper(a,b,3) end,
    AND=function(a,b) return bitoper(a,b,4) end,
}

function all(iterable, condition)
  for k, v in pairs(iterable) do
    if not condition(k, v) then return false end
  end
  return true
end

function any(iterable, condition)
  for k, v in pairs(iterable) do
    if condition(k, v) then return true end
  end
  return false
end

local DISPEL_SPELLS = {
  Poison = { Cleanse = 4987 },
  Magic = { Cleanse = 4987, ["Dispel Magic"] = 988 },
  Disease = { Cleanse = 4987, ["Abolish Disease"] = 552 },
  -- Curse = { ":("}
}

local function get_eligible_player_dispel_spell(spell_type)
  local spells = DISPEL_SPELLS[spell_type]
  if spells ~= nil then
    for spell_name, spellid in pairs(spells) do
      if IsSpellKnown(spellid) then
        return spell_name
      end
    end
  end
end

function group_dispel()
  local guildies = get_guild_members_list()
  local guildie_name = guildies[random(1, #guildies)]
  for i=1,40 do
    local debuff_name, _rank, _icon, count, dispel_type = UnitAura(guildie_name, i, "HARMFUL")
    if debuff_name == nil then break end
    local dispel_spell = get_eligible_player_dispel_spell(dispel_type)
    if dispel_spell ~= nil then
      L_TargetUnit(guildie_name)
      return L_CastSpellByName(dispel_spell)
    end
  end
end
