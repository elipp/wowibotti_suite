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

-- Power type → bar color (r, g, b)
local POWER_COLORS = {
    [0] = { 0.00, 0.44, 0.87 }, -- Mana     (blue)
    [1] = { 1.00, 0.00, 0.00 }, -- Rage     (red)
    [2] = { 1.00, 0.82, 0.00 }, -- Focus    (yellow)
    [3] = { 1.00, 0.65, 0.00 }, -- Energy   (orange-yellow)
    [4] = { 0.00, 1.00, 1.00 }, -- Happiness (cyan, hunter pet)
    [6] = { 0.00, 0.82, 1.00 }, -- Runic Power (light blue)
    [7] = { 0.50, 0.50, 0.90 }, -- Soul Shards (purple-ish)
    [8] = { 1.00, 0.61, 0.00 }, -- Eclipse   (amber)
    [9] = { 1.00, 0.31, 0.17 }, -- Holy Power (gold-ish)
}
local POWER_COLOR_DEFAULT = { 0.70, 0.70, 0.70 }

-- Frame dimensions
local PORTRAIT_SIZE  = 36
local BAR_HEIGHT     = 4
local BAR_GAP        = 1.5
local BARS_TOTAL     = (BAR_HEIGHT * 2) + BAR_GAP  -- health + power bar
local FRAME_WIDTH    = PORTRAIT_SIZE + 1
local FRAME_HEIGHT   = PORTRAIT_SIZE + 1

function UnitSelectionFrame.new()
    local self = setmetatable({}, UnitSelectionFrame)

    self.selected_units = {}
    self.num_units = 0
    self.prev_hash = 0

    self:_build_frame()
    self:_build_close_button()

    return self
end

function UnitSelectionFrame:_build_frame()
    self.frame = CreateFrame("Frame", "loleUnitSelectionFrame", WorldFrame)
    self.frame:SetHeight(0.20 * GetScreenHeight())
    self.frame:SetWidth(0.70 * GetScreenWidth())
    self.frame:SetPoint("BOTTOM", 0, 10)
    self.frame:SetBackdrop(BACKDROP)
    self.frame:SetFrameStrata("LOW")
    self.frame:EnableMouse(true)
    self.frame:SetMovable(false)
    self.frame:Hide()
end

function UnitSelectionFrame:_build_close_button()
    local btn = CreateFrame("Button", nil, self.frame, "UIPanelCloseButton")
    btn:SetPoint("TOPRIGHT", 0, 0)
    btn:SetScript("OnClick", function() self:hide() end)
end

local function hideBlizzUI()
    -- ChatFrame1:Hide()
    -- MainMenuBar:Hide()
    FocusFrame:SetParent(selection_ui.frame)
    UIParent:Hide()
end

local function unhideBlizzUI()
    -- ChatFrame1:Show()
    -- MainMenuBar:Show()
    FocusFrame:SetParent(UIParent)
    UIParent:Show()

end

function UnitSelectionFrame:show()
    -- hideBlizzUI is called in selection_ui:OnUpdate
    hideBlizzUI()
    self.frame:Show()
end

function UnitSelectionFrame:hide()
    unhideBlizzUI()
    self.frame:Hide()
end

function UnitSelectionFrame:clear_selection()
    for _, unit in ipairs(self.selected_units) do
        unit.frame:Hide()
    end
    self.selected_units = {}
    self.num_units = 0
end

local unit_frame_pool = {}

function UnitSelectionFrame:update_selected_units(units_table)
    self:clear_selection()
    for _, unit in ipairs(units_table) do
        if unit_frame_pool[unit.name] == nil then
            unit_frame_pool[unit.name] = self:_create_unit_frame(unit.name)
        end
        self:_add_unit_frame(unit, unit_frame_pool[unit.name])
    end
end

function UnitSelectionFrame:units_without(unit_name)
    local res = {}
    for i, unit in ipairs(self.selected_units) do
        if unit.name ~= unit_name then
            table.insert(res, unit)
        end
    end
    return res
end

function UnitSelectionFrame:find_unit(unit_name)
    for i, unit in ipairs(self.selected_units) do
        if unit.name == unit_name then
            return {unit}
        end
    end
    return {}
end

function UnitSelectionFrame:get_selected_unit_names()
    local units = {}
    for _, unit in ipairs(self.selected_units) do
        units[#units + 1] = unit.name
    end
    return units
end

function UnitSelectionFrame:get_selected_unit_names_comma_separated()
    return self:get_selected_unit_names().concat(parts, ",")
end

function UnitSelectionFrame:_add_unit_frame(unit, frame)
    unit.frame = frame
    unit.frame:SetPoint("TOPLEFT", 30 + self.num_units * STRIDE_PIXELS, -30)
    unit.frame:Show()
    self.selected_units[#self.selected_units + 1] = unit
    self.num_units = #self.selected_units
end

function UnitSelectionFrame:_get_portrait_filename(name)
    local _, race = UnitRace(name)
    local sex = SEX_STRINGS[UnitSex(name)]
    return "Interface\\CHARACTERFRAME\\TEMPORARYPORTRAIT-"
           .. sex .. "-" .. string.upper(race) .. ".BLP"
end

function UnitSelectionFrame:_create_portrait(parent, name)
    local SIZE_ADJ = -4
    local SIZE = PORTRAIT_SIZE + SIZE_ADJ
    local portrait_frame = CreateFrame("Frame", nil, parent)
    portrait_frame:SetSize(SIZE, SIZE)
    -- Offset downward by the bar area so bars sit above the portrait
    portrait_frame:SetPoint("TOPLEFT", -SIZE_ADJ, -SIZE_ADJ - 8)
    portrait_frame:SetFrameStrata("LOW")

    local tex = portrait_frame:CreateTexture(nil, "ARTWORK")
    tex:SetSize(SIZE, SIZE)
    tex:SetTexture(self:_get_portrait_filename(name))
    tex:SetPoint("CENTER")

    return portrait_frame
end

function UnitSelectionFrame:_create_class_icon(parent, name)
    local icon_frame = CreateFrame("Frame", nil, parent)
    icon_frame:SetSize(16, 16)
    icon_frame:SetPoint("TOPRIGHT", 4, 4)
    icon_frame:SetFrameStrata("MEDIUM")

    local icon = icon_frame:CreateTexture(nil, "ARTWORK")
    icon:SetSize(16, 16)
    icon:SetTexture("Interface\\GLUES\\CHARACTERCREATE\\UI-CHARACTERCREATE-CLASSES.blp")
    icon:SetPoint("TOPRIGHT", 0, 0)

    local _, class = UnitClass(name)
    icon:SetTexCoord(unpack(CLASS_ICON_TCOORDS[class]))

    return icon_frame
end

-- Creates a thin status bar with a dark background track
function UnitSelectionFrame:_create_status_bar(parent, yOffset, r, g, b)
    -- Dark track behind the bar
    local track = parent:CreateTexture(nil, "BACKGROUND")
    track:SetSize(FRAME_WIDTH, BAR_HEIGHT)
    track:SetPoint("BOTTOMLEFT", 0, yOffset)
    track:SetTexture("Interface\\ChatFrame\\ChatFrameBackground")
    track:SetVertexColor(0.1, 0.1, 0.1, 0.9)

    local bar = CreateFrame("StatusBar", nil, parent)
    bar:SetSize(FRAME_WIDTH, BAR_HEIGHT)
    bar:SetPoint("BOTTOMLEFT", 0, yOffset)
    bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
    bar:SetStatusBarColor(r, g, b)
    bar:SetMinMaxValues(0, 1)
    bar:SetValue(1)
    bar:SetFrameStrata("HIGH")

    return bar
end

-- Builds both health and power bars and wires up an OnUpdate to keep them live
function UnitSelectionFrame:_create_unit_bars(parent, unit_name)
    local yOffset = 0
    -- Health bar (green) — topmost
    local hp_bar = self:_create_status_bar(
        parent,
        yOffset,
        0.0, 0.8, 0.1
    )

    -- Power bar — just below health bar
    local power_type = UnitPowerType(unit_name) or 0
    local pc = POWER_COLORS[power_type] or POWER_COLOR_DEFAULT
    local pw_bar = self:_create_status_bar(
        parent,
        yOffset-(BAR_HEIGHT + BAR_GAP),
        pc[1], pc[2], pc[3]
    )

    -- OnUpdate: refresh values from the unit token
    -- UnitHealth/UnitMana etc. accept the unit name directly as a token in 3.3.5a
    parent:SetScript("OnUpdate", function()
        local hp    = UnitHealth(unit_name)
        local hpmax = UnitHealthMax(unit_name)
        if hpmax and hpmax > 0 then
            hp_bar:SetMinMaxValues(0, hpmax)
            hp_bar:SetValue(hp)
        end

        local pw    = UnitPower(unit_name, power_type)
        local pwmax = UnitPowerMax(unit_name, power_type)
        if pwmax and pwmax > 0 then
            pw_bar:SetMinMaxValues(0, pwmax)
            pw_bar:SetValue(pw)
        else
            pw_bar:SetValue(0)
        end
    end)

    return hp_bar, pw_bar
end

local function find(t, key, value)
    for _, inner in ipairs(t) do
        if inner[key] == value then
            return { inner }
        end
    end
    return {}
end

function UnitSelectionFrame:_create_unit_frame(unit_name)
    local unit_frame = CreateFrame("Button", nil, self.frame)
    unit_frame:SetBackdrop(BACKDROP)
    -- Taller frame: portrait + bar area + label space
    unit_frame:SetSize(FRAME_WIDTH, FRAME_HEIGHT)

    -- Bars are anchored to the top of the unit_frame
    self:_create_unit_bars(unit_frame, unit_name)

    -- Portrait is pushed down below the bars
    local portrait_frame = self:_create_portrait(unit_frame, unit_name)
    self:_create_class_icon(portrait_frame, unit_name)

    local label = unit_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("BOTTOM", unit_frame, 0, -16)
    label:SetText(unit_name)

    unit_frame:SetScript("OnMouseDown", function(_unit_frame, button)
        local new_units = IsControlKeyDown() and self:units_without(unit_name) or self:find_unit(unit_name)
        lole_wc3mode.update_selected_units(new_units)
        self:update_selected_units(new_units)
    end)

    return unit_frame
end

selection_ui = UnitSelectionFrame.new()

selection_ui.frame:SetScript("OnUpdate", function(self)
    local units, hash = lole_wc3mode.get_selected_units()
    if hash ~= selection_ui.prev_hash then
        selection_ui.prev_hash = hash
        selection_ui:update_selected_units(units)
    end
end)
