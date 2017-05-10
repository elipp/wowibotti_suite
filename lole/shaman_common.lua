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

	["Mana Spring Totem"] = "Mana Spring",
--	["Mana Tide Totem"] = nil,
--	["Poison Cleansing Totem"] = nil,
--	["Disease Cleansing Totem"] = nil,

	["Wrath of Air Totem"] = "Wrath of Air Totem",
--  ["Windfury Totem"] = nil,
	["Grace of Air Totem"] = "Grace of Air"

};


local function recast_totem_if_noexists_or_OOR(arg_totem)
	local _, totemName, startTime, duration = GetTotemInfo(totem_name_type_map[arg_totem]);

	if totemName == "Mana Tide Totem" then
		return false
	end

	if startTime == 0 and duration == 0 then
		L_CastSpellByName(arg_totem);
		return true;
	end

	-- see if totem in range (doesn't work for tremor totem, mana tide totem, windfury totem etc)

	local cur_target = UnitName("target");

	TargetUnit(arg_totem);
	if IsSpellInRange("Healing Wave", "target") == 0 then
		L_CastSpellByName(arg_totem);
		ClearTarget();
		return true;
	end
 -- too bad shamans don't have a 30yd heal

	ClearTarget()

	local bname = totem_name_buffname_map[arg_totem];
	if bname then
		if not has_buff("player", bname) then
			L_CastSpellByName(arg_totem);
			return true;
		end
	end

	return false;
end

function refresh_totems(TOTEMS)
	for slot,name in pairs(TOTEMS) do
		if recast_totem_if_noexists_or_OOR(name) then return true; end
	end
	return false;
end
