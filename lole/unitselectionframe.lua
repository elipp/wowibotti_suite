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

us_frame:SetHeight(0.20*GetScreenHeight())
us_frame:SetWidth(0.7*GetScreenWidth())
us_frame:SetPoint("BOTTOM", 0, 10)

us_frame:SetBackdrop(backdrop)
us_frame:Hide()

us_frame:EnableMouse(true)
us_frame:SetMovable(false)

local close_button = CreateFrame("Button", "close_button", us_frame, "UIPanelCloseButton");
close_button:SetPoint("TOPRIGHT", 0, 0);

close_button:SetScript("OnClick", function()
	us_frame:Hide();
end)

function selection_frame_show()
	us_frame:Show()
	UIParent:Hide()
end

function selection_frame_hide()
	us_frame:Hide()
	UIParent:Show()
end

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
	for i, uf in pairs(selected_units) do
		uf.unitframe:Hide()
	end
	selected_units = {}
	num_units = 0
end

local stride_pixels = 52

local function add_unitframe(unitname, unitframe)
	local uf = {}
	uf.unitname = unitname
	uf.unitframe = unitframe
	unitframe:SetPoint("TOPLEFT", 30+num_units*stride_pixels, -30)
	selected_units[#selected_units + 1] = uf
	num_units = #selected_units
end

local function refresh_unitframe_positions()
	for i, uf in pairs(selected_units) do
		uf.unitframe:SetPoint("TOPLEFT", 30 + (i-1)*stride_pixels, -30)
	end
end

local function deselect_unit(unitname)

	local index = nil
	for i, uf in pairs(selected_units) do
		if uf.unitname == unitname then
			index = i
			break
		end
	end

	if not index then return end

	selected_units[index].unitframe:Hide()

	for i = index,num_units-1 do
		selected_units[i] = selected_units[i+1]
	end

	table.remove(selected_units) -- this is like a "pop" operation
	num_units = #selected_units

	refresh_unitframe_positions()
	set_selection(get_selected_units_commaseparated())

end

local function create_unit_frame(unitname)
	L_TargetUnit(unitname)

	if not UnitExists("target") then
		lole_error("unit " .. unitname .. " doesn't exist!")
		return
	end

	local unitframe = CreateFrame("Button", nil, us_frame)
	unitframe:SetBackdrop(backdrop)

	unitframe:SetHeight(40)
	unitframe:SetWidth(40)
	local portframe = create_portrait(unitframe)
	local icon = create_class_icon(portframe)

	-- SetPoint is done later (even though it's kind of weird), in add_unitframe

	local name = unitframe:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	name:SetPoint("BOTTOM", unitframe, 0, -12)
	name:SetText(UnitName("target"))
	unitframe.unitname = UnitName("target")

	unitframe:SetScript("OnClick", function(self)
		if IsControlKeyDown() then
			deselect_unit(self.unitname)
		else
			set_selection(self.unitname)
		end
	end)

	ClearTarget()

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
		add_unitframe(unitname, create_unit_frame(unitname))
	end
end

function get_selected_units()
	local u = {}
	for i,uf in pairs(selected_units) do
		u[#u + 1] = uf.unitname
	end

	return u
end

function get_selected_units_commaseparated()
	local s = ""
	for i,uf in pairs(selected_units) do
		s = s .. uf.unitname .. ","
	end
	return s:sub(1, -2) -- deletes the last comma apparently

end

--us_frame:SetScript("OnUpdate", update_selected)
