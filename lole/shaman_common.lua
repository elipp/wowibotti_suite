TIME_TOTEMS_REFRESHED = 0;

local totem_type_id_map = {
	["fire"] = 1,
	["earth"] = 2,
	["water"] = 3,
	["air"] = 4
};

local totem_name_type_map = {
	["Tremor Totem"] = totem_type_id_map["earth"],
	["Stoneskin Totem"] = totem_type_id_map["earth"],
	["Strength of Earth Totem"] = totem_type_id_map["earth"],

	["Totem of Wrath"] = totem_type_id_map["fire"],
	["Frost Resistance Totem"] = totem_type_id_map["fire"],
	["Searing Totem"] = totem_type_id_map["fire"],
	["Flametongue Totem"] = totem_type_id_map["fire"],

	["Mana Spring Totem"] = totem_type_id_map["water"],
	["Mana Tide Totem"] = totem_type_id_map["water"],
	["Poison Cleansing Totem"] = totem_type_id_map["water"],
	["Disease Cleansing Totem"] = totem_type_id_map["water"],

	["Wrath of Air Totem"] = totem_type_id_map["air"],
	["Windfury Totem"] = totem_type_id_map["air"],
	["Grace of Air Totem"] = totem_type_id_map["air"],
	["Nature Resistance Totem"] = totem_type_id_map["air"]
};

local totem_name_buffname_map = {
--	["Tremor Totem"] = nil,
	["Stoneskin Totem"] = "Stoneskin",
	["Strength of Earth Totem"] = "Strength of Earth",

	["Totem of Wrath"] = "Totem of Wrath",
	["Frost Resistance Totem"] = "Frost Resistance",
	["Flametongue Totem"] = "Flametongue Totem",

	["Mana Spring Totem"] = "Mana Spring",
--	["Mana Tide Totem"] = nil,
--	["Poison Cleansing Totem"] = nil,
--	["Disease Cleansing Totem"] = nil,

	["Wrath of Air Totem"] = "Wrath of Air Totem",
--  ["Windfury Totem"] = nil,
	["Grace of Air Totem"] = "Grace of Air"

};


local function should_recast_totem(arg_totem)
	local _, totemName, startTime, duration = GetTotemInfo(totem_name_type_map[arg_totem]);

	if totemName == "Mana Tide Totem" then
		return false
	end

	if not starts_with(totemName, arg_totem) then
		return true
	end

	if startTime == 0 and duration == 0 then
		return true;
	end

	-- see if totem in range (doesn't work for tremor totem, mana tide totem, windfury totem etc)

	local cur_target = UnitName("target");

	L_TargetUnit(arg_totem);
	if IsSpellInRange("Healing Wave", "target") == 0 then
		L_ClearTarget();
		return true;
	end
 -- too bad shamans don't have a 30yd heal

	L_ClearTarget()

	--[[local bname = totem_name_buffname_map[arg_totem];
	if bname then
		if not has_buff("player", bname) then
			echo(3)
			return true;
		end
	end--]]

	return false;
end

function refresh_totems(TOTEMS, TOTEM_BAR)
	--[[if (GetTime() - TIME_TOTEMS_REFRESHED) < 5 then
        return false;
    end--]]

	local totems_to_recast = {}
	local num_to_recast = 0
	for slot,name in pairs(TOTEMS) do
		if should_recast_totem(name) then 
			totems_to_recast[name] = true
			num_to_recast = num_to_recast + 1
		end
		if num_to_recast > 2 then
			L_CastSpellByName(TOTEM_BAR)
			TIME_TOTEMS_REFRESHED = GetTime()
			return true
		end
	end

	for totem, _ in pairs(totems_to_recast) do
		L_CastSpellByName(totem)
		TIME_TOTEMS_REFRESHED = GetTime()
		return true
	end

	return false
end
