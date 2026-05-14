local UnitSelectionFrame = {}
UnitSelectionFrame.__index = UnitSelectionFrame

local BACKDROP = {
    bgFile   = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = false, tileSize = 32, edgeSize = 12,
    insets = { left = 0, right = 0, top = 0, bottom = 0 }
}

local STRIDE_PIXELS = 52
local SEX_STRINGS   = { "UNKNOWN", "MALE", "FEMALE" }

function UnitSelectionFrame.new()
    local self = setmetatable({}, UnitSelectionFrame)

    self.selected_units = {}
    self.num_units      = 0

    self:_build_frame()
    self:_build_close_button()

    return self
end

function UnitSelectionFrame:_build_frame()
    self.frame = CreateFrame("Frame", nil, WorldFrame)
    self.frame:SetHeight(0.20 * GetScreenHeight())
    self.frame:SetWidth(0.70 * GetScreenWidth())
    self.frame:SetPoint("BOTTOM", 0, 10)
    self.frame:SetBackdrop(BACKDROP)
    self.frame:SetFrameStrata("HIGH")
    self.frame:EnableMouse(true)
    self.frame:SetMovable(false)
    self.frame:Hide()
end

function UnitSelectionFrame:_build_close_button()
    local btn = CreateFrame("Button", nil, self.frame, "UIPanelCloseButton")
    btn:SetPoint("TOPRIGHT", 0, 0)
    btn:SetScript("OnClick", function() self:hide() end)
end

function UnitSelectionFrame:show()
    self.frame:Show()
    UIParent:Hide()
end

function UnitSelectionFrame:hide()
    self.frame:Hide()
    UIParent:Show()
end

function UnitSelectionFrame:clear_selection()
    for _, uf in ipairs(self.selected_units) do
        uf.unit_frame:Hide()
    end
    self.selected_units = {}
    self.num_units = 0
end

local unit_frame_pool = {}

function UnitSelectionFrame:update_selection(units_table)
    self:clear_selection()
    for _, unit in ipairs(units_table) do
        if unit_frame_pool[unit.name] == nil then
            unit_frame_pool[unit.name] = self:_create_unit_frame(unit.name)
        end
        local frame = unit_frame_pool[unit.name]
        self:_add_unit_frame(unit.name, frame)
        frame:Show()
    end
end

function UnitSelectionFrame:deselect_unit(unit_name)
    local index
    for i, uf in ipairs(self.selected_units) do
        if uf.unit_name == unit_name then index = i; break end
    end
    if not index then return end

    self.selected_units[index].unit_frame:Hide()
    table.remove(self.selected_units, index)
    self.num_units = #self.selected_units

    self:_refresh_unit_frame_positions()
end

function UnitSelectionFrame:get_selected_units()
    local units = {}
    for _, uf in ipairs(self.selected_units) do
        units[#units + 1] = uf.unit_name
    end
    return units
end

function UnitSelectionFrame:get_selected_units_comma_separated()
    local parts = {}
    for _, uf in ipairs(self.selected_units) do
        parts[#parts + 1] = uf.unit_name
    end
    return table.concat(parts, ",")
end

function UnitSelectionFrame:_add_unit_frame(unit_name, unit_frame)
    local uf = { unit_name = unit_name, unit_frame = unit_frame }
    unit_frame:SetPoint("TOPLEFT", 30 + self.num_units * STRIDE_PIXELS, -30)
    self.selected_units[#self.selected_units + 1] = uf
    self.num_units = #self.selected_units
end

function UnitSelectionFrame:_refresh_unit_frame_positions()
    for i, uf in ipairs(self.selected_units) do
        uf.unit_frame:SetPoint("TOPLEFT", 30 + (i - 1) * STRIDE_PIXELS, -30)
    end
end

function UnitSelectionFrame:_get_portrait_filename(name)
    local _, race = UnitRace(name)
    local sex = SEX_STRINGS[UnitSex(name)]
    return "Interface\\CHARACTERFRAME\\TEMPORARYPORTRAIT-"
           .. sex .. "-" .. string.upper(race) .. ".BLP"
end

function UnitSelectionFrame:_create_portrait(parent, name)
    local portrait_frame = CreateFrame("Frame", nil, parent)
    portrait_frame:SetSize(40, 40)
    portrait_frame:SetPoint("CENTER")

    local tex = portrait_frame:CreateTexture(nil, "ARTWORK")
    tex:SetSize(40, 40)
    tex:SetTexture(self:_get_portrait_filename(name))
    tex:SetPoint("CENTER")

    return portrait_frame
end

function UnitSelectionFrame:_create_class_icon(parent, name)
    local icon_frame = CreateFrame("Frame", nil, parent)
    icon_frame:SetSize(16, 16)
    icon_frame:SetPoint("TOPRIGHT", 2, 2)

    local icon = icon_frame:CreateTexture(nil, "ARTWORK")
    icon:SetSize(16, 16)
    icon:SetTexture("Interface\\GLUES\\CHARACTERCREATE\\UI-CHARACTERCREATE-CLASSES.blp")
    icon:SetPoint("TOPRIGHT", 2, 2)

    local _, class = UnitClass(name)
    icon:SetTexCoord(unpack(CLASS_ICON_TCOORDS[class]))

    return icon_frame
end

function UnitSelectionFrame:_create_unit_frame(unit_name)
    local unit_frame = CreateFrame("Button", nil, self.frame)
    unit_frame:SetBackdrop(BACKDROP)
    unit_frame:SetSize(40, 40)

    local portrait_frame = self:_create_portrait(unit_frame, unit_name)
    self:_create_class_icon(portrait_frame, unit_name)

    local label = unit_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("BOTTOM", unit_frame, 0, -12)
    label:SetText(unit_name)

    unit_frame.unit_name = unit_name
    unit_frame:SetScript("OnMouseDown", function(_unit_frame, button)
        if IsControlKeyDown() then
            self:deselect_unit(_unit_frame.unit_name)
        else
            self:update_selection({ {name = _unit_frame.unit_name} })
        end
    end)

    return unit_frame
end

selection_ui = UnitSelectionFrame.new()

selection_ui.frame:SetScript("OnUpdate", function(self)
    local left, bottom, width, height = get_pixel_rect(self)
    lole_wc3mode.unitselection_frame_region(left,bottom,width,height)
end)
