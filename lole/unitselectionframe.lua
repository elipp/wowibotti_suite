local us_frame = CreateFrame("Frame");

local backdrop = {
	bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
	edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
	  -- true to repeat the background texture to fill the frame, false to scale it
	tile = false,
	  -- size (width or height) of the square repeating background tiles (in pixels)
	tileSize = 32,
	  -- thickness of edge segments and square size of edge corners (in pixels)
	edgeSize = 12,
	  -- distance from the edges of the frame to those of the background texture (in pixels)
	insets = {
		left = 0,
		right = 0,
		top = 0,
		bottom = 0
	}
}

us_frame:SetHeight(150)
us_frame:SetWidth(700)
us_frame:SetPoint("BOTTOM", 0, 100)

us_frame:SetBackdrop(backdrop)

us_frame:EnableMouse(true)
us_frame:SetMovable(false)

local sex_strings = {
[1] = 'UNKNOWN',
[2] = 'MALE',
[3] = 'FEMALE'
}

local function get_portrait_filename()
	local _,race = UnitRace("target")
	race = string.upper(race)

	local sex = sex_strings[UnitSex("target")]
	return "Interface\\CHARACTERFRAME\\TEMPORARYPORTRAIT-"..sex.."-"..race..".BLP"

end

local function create_portrait(parent)

	local pframe = CreateFrame("Frame", nil, parent)

	pframe:SetHeight(40)
	pframe:SetWidth(40)
	pframe:SetPoint("CENTER")

	local texname = get_portrait_filename()
	local port = pframe:CreateTexture(nil, "ARTWORK")
	port:SetWidth(40)
	port:SetHeight(40)
	port:SetTexture(texname)
	port:SetPoint("CENTER")

	return pframe;
end

local function create_class_icon(parent)

	local iconframe = CreateFrame("Frame", nil, parent)
	iconframe:SetHeight(16)
	iconframe:SetWidth(16)
	iconframe:SetPoint("TOPRIGHT", 2, 2)

	local icon = iconframe:CreateTexture(nil, "ARTWORK")
	icon:SetTexture("Interface\\GLUES\\CHARACTERCREATE\\UI-CHARACTERCREATE-CLASSES.blp")
	icon:SetWidth(16)
	icon:SetHeight(16)
	icon:SetPoint("TOPRIGHT", 2, 2)
	local _, class = UnitClass("target")
	local coords = CLASS_ICON_TCOORDS[class]; -- get the coordinates of the class icon we want
	icon:SetTexCoord(unpack(coords)); -- cut out the region with our class icon according to coord

	return iconframe
end

local selected_units = {}
local num_units = 0

function clear_selection()
	for c, frame in pairs(selected_units) do
		frame:Hide()
	end
	selected_units = {}
	num_units = 0
end

local stride_pixels = 44

local function add_unitframe(unitframe)
	selected_units[#selected_units + 1] = unitframe
	num_units = num_units + 1
end

local function create_unit_frame(unitname)
	L_TargetUnit(unitname)

	if not UnitExists("target") then
		lole_error("unit " .. unitname .. " doesn't exist!")
		return
	end

	local unitframe = CreateFrame("Frame", nil, us_frame)
	unitframe:SetBackdrop(backdrop)

	unitframe:SetHeight(40)
	unitframe:SetWidth(40)
	unitframe:SetPoint("TOPLEFT", 30+num_units*stride_pixels, -30)
	local portframe = create_portrait(unitframe)
	local icon = create_class_icon(portframe)

	local name = unitframe:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	name:SetPoint("BOTTOM", unitframe, 0, -12)
	name:SetText(UnitName("target"))

	L_ClearTarget()

	return unitframe

end

-- local header_texture = us_frame:CreateTexture(nil, "ARTWORK")
-- header_texture:SetTexture(get_portrait_filename("player"))
-- header_texture:SetWidth(36)
-- header_texture:SetHeight(36)
-- header_texture:SetPoint("TOP", 0, 12)

function update_selection(selected_units_table)
	clear_selection()
	for i, unitname in pairs(selected_units_table) do
		add_unitframe(create_unit_frame(unitname))
	end
end

--us_frame:SetScript("OnUpdate", update_selected)
