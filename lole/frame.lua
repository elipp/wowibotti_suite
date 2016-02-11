lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("ADDON_LOADED");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:RegisterEvent("PLAYER_DEAD");

local every_nth_frame = 4
local frame_modulo = 0

lole_frame:SetScript("OnUpdate", function()
	if get_blast_state() and frame_modulo == 0 then
		lole_main();
	end
	
	if frame_modulo >= every_nth_frame then -- mod would be better, but,
		frame_modulo = 0
	else
		frame_modulo = frame_modulo + 1
	end
end);

local function LOLE_EventHandler(self, event, prefix, message, channel, sender) 
	--DEFAULT_CHAT_FRAME:AddMessage("LOLE_EventHandler: event:" .. event)
	
	if event == "ADDON_LOADED" then
		if prefix ~= "lole" then return end

		if LOLE_CLASS_CONFIG_NAME_SAVED ~= nil then
			lole_subcommands.setconfig(LOLE_CLASS_CONFIG_NAME_SAVED, LOLE_CLASS_CONFIG_ATTRIBS_SAVED);
		else
			lole_subcommands.setconfig("default");
		end
		
		clear_target()
		
		lole_frame:UnregisterEvent("ADDON_LOADED");
		
	elseif event == "PLAYER_DEAD" then
		clear_target();
	
	elseif event == "PLAYER_REGEN_DISABLED" then
		if IsRaidLeader() then
			broadcast_follow_target(NOTARGET);
		end
	end
	
	
end

lole_frame:SetScript("OnEvent", LOLE_EventHandler);

lole_frame:SetHeight(300)
lole_frame:SetWidth(250)
lole_frame:SetPoint("RIGHT")



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

lole_frame:SetBackdrop(backdrop)
lole_frame:SetMovable(true)

lole_frame:EnableMouse(true)
lole_frame:SetMovable(true)
lole_frame:RegisterForDrag("LeftButton")
lole_frame:SetScript("OnDragStart", function(self, button)
	self:StartMoving()
end)
lole_frame:SetScript("OnDragStop", function(self, button)
	self:StopMovingOrSizing()
	-- get container position and save it here if you want
end)

local header_texture = lole_frame:CreateTexture(nil, "ARTWORK")
header_texture:SetTexture("Interface\\DialogFrame\\UI-DialogBox-Header")
header_texture:SetWidth(315)
header_texture:SetHeight(64)
header_texture:SetPoint("TOP", 0, 12)

local header_text = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
header_text:SetPoint("TOP", header_texture, "TOP", 0, -14)
header_text:SetText("LOLEXD")

blast_checkbutton = CreateFrame("CheckButton", "blast_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
blast_checkbutton:SetPoint("TOPLEFT", 15, -30);


blast_disabled_string = " BLAST disabled"
blast_enabled_string = " BLAST enabled!"


function blast_check_settext(text)
	getglobal(blast_checkbutton:GetName() .. "Text"):SetText(text)
end

blast_check_settext(blast_disabled_string)

blast_checkbutton.tooltip = "Whether BLAST is on or off.";

blast_checkbutton:SetScript("OnClick", 
  function()
	local arg = blast_checkbutton:GetChecked() and "1" or "0"; -- this is the lua equivalent of '?' in C :D
	lole_subcommands.blast(arg)
  end
);


local static_target_text = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
static_target_text:SetPoint("TOPLEFT", 18, -55);
static_target_text:SetText("Current blast target:")

local target_text = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
target_text:SetPoint("TOPLEFT", 123, -55);
target_text:SetText("")

local target_GUID_text = lole_frame:CreateFontString(nil, "OVERLAY")
target_GUID_text:SetPoint("TOPLEFT", 123, -65);
target_GUID_text:SetFont("Fonts\\FRIZQT__.TTF", 9);

function update_target_text(name, GUID)
	target_text:SetText(name)
	target_GUID_text:SetText(GUID)
end

local close_button = CreateFrame("Button", "close_button", lole_frame, "UIPanelCloseButton");
close_button:SetPoint("TOPRIGHT", 0, 0);

close_button:SetScript("OnClick", function()
	lole_frame:Hide();
end)

function main_frame_show()
	lole_frame:Show();
end

--local edit = CreateFrame("EditBox", "edit1", lole_frame);
--edit:SetPoint("TOPLEFT", 10, -30);
--edit:SetHeight(15)
--edit:SetWidth(80)
--edit:SetText("");
--edit:SetAutoFocus(false)

--edit:SetCursorPosition(0);
--edit:SetBackdrop(backdrop);
--edit:SetTextInsets(3, 3, 0, 0)
--edit:SetBackdropBorderColor(1.0, 0.0, 0.0, 1.0);

--edit:SetFont("Fonts\\FRIZQT__.TTF", 10);

--edit:SetScript("OnEscapePressed", function(self) self:ClearFocus() end)

local config_dropdown = CreateFrame("Frame", "config_dropdown", lole_frame, "UIDropDownMenuTemplate");
config_dropdown:SetPoint("BOTTOMLEFT", 60, 10);

UIDropDownMenu_SetWidth(100, config_dropdown)

local config_text = config_dropdown:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
config_text:SetPoint("LEFT", -23, 3);
config_text:SetText("Config:")


local function config_drop_onClick(name)
	lole_subcommands.setconfig(string.sub(name, 11)); -- these have the color string in front of them, length 10 
end
 
local drop_formatted_configs = {}
local drop_formatted_config_indices = {}

local i = 1;

for k, v in pairsByKey(get_available_configs()) do
	drop_formatted_configs[i] = "|cFF" .. v.color .. k
	drop_formatted_config_indices[k] = i -- XD
	echo(k)
	i = i + 1;
end

function set_visible_dropdown_config(configname)
	UIDropDownMenu_SetSelectedID(config_dropdown, drop_formatted_config_indices[configname])
end


local function config_drop_initialize()
		
	local info = {}
	
	for n = 1, #drop_formatted_configs do
		info.text = drop_formatted_configs[n];
		info.value = n;
		info.arg1 = info.text;
		
		if n == drop_formatted_config_indices[get_current_config()] then
			info.checked = 1
		else
			info.checked = nil;
		end
		
		info.func = config_drop_onClick;
		UIDropDownMenu_AddButton(info)
	end
end
 
UIDropDownMenu_Initialize(config_dropdown, config_drop_initialize)


local follow_button = CreateFrame("Button", "follow_button", lole_frame, "UIPanelButtonTemplate")

follow_button:SetPoint("TOPLEFT", 22, -100);
follow_button:SetHeight(27)
follow_button:SetWidth(85)

follow_button:SetText("Follow me!");

follow_button:SetScale(0.75)

follow_button:SetScript("OnClick", function()
	lole_subcommands.followme()
end)

local stopfollow_button = CreateFrame("Button", "stopfollow_button", lole_frame, "UIPanelButtonTemplate")

stopfollow_button:SetPoint("TOPLEFT", 22, -130);
stopfollow_button:SetHeight(27)
stopfollow_button:SetWidth(85)

stopfollow_button:SetScale(0.75)

stopfollow_button:SetText("Stopfollow");

stopfollow_button:SetScript("OnClick", function()
	lole_subcommands.stopfollow()
end)

local update_target_button = CreateFrame("Button", "stopfollow_button", lole_frame, "UIPanelButtonTemplate")

update_target_button:SetPoint("TOPLEFT", 120, -100);
update_target_button:SetHeight(27)
update_target_button:SetWidth(158)

update_target_button:SetScale(0.75)

update_target_button:SetText("Update BLAST target");

update_target_button:SetScript("OnClick", function()
	
	if not IsRaidLeader() then
		lole_error("Couldn't update BLAST target (you are not the raid leader!)");
		return;
	end
	
	if not UnitExists("target") then 
		lole_error("Couldn't update BLAST target (no valid mob selected!)");
		return;
	end
	
	local target_GUID = UnitGUID("target");
	
	if target_GUID ~= BLAST_TARGET_GUID then
		if string.sub(target_GUID, 3, 6) == "F130" and (not UnitIsDead("target")) and UnitReaction("target", "player") < 5 then
			set_target(UnitGUID("target"))
			broadcast_target_GUID(UnitGUID("target"));
			return;
		else
			lole_error("Invalid target! Select an attackable NPC mob.")
			return;
		end
	end
end)


local ctm_host = { title = "CTM mode:", title_fontstr = nil, buttons = {}, num_buttons = 0, first_pos_x = 150, first_pos_y = -102, increment = -18 }

CTM_MODES = { 
	LOCAL = 1, TARGET = 2, EVERYONE = 3, HEALERS = 4, CASTERS = 5, MELEE = 6 
}


function get_CTM_mode()
	return ctm_host.checkedID;
end

-- TODO: look into whether this really needs to be exclusive (!!)


ctm_host.exclusive_onclick = function(self, button, down)
	
	for k, b in pairs(ctm_host.buttons) do
		if (b:GetID() ~= self:GetID()) then
			b:SetChecked(nil);
		else
			b:SetChecked(true);
			ctm_host.checkedID = b:GetID();
		end
	end
	
end


function ctm_host:add_button(button_text)
	self.num_buttons = self.num_buttons + 1;
	self.buttons[self.num_buttons] = CreateFrame("CheckButton", "ctm_radio" .. tostring(self.num_buttons), lole_frame, "UIRadioButtonTemplate")
	
	local button = self.buttons[self.num_buttons];

	getglobal(button:GetName() .. "Text"):SetText(button_text);
	button:SetID(self.num_buttons);
	button:SetPoint("TOPLEFT", self.first_pos_x, self.first_pos_y + self.increment*self.num_buttons);
	button:SetScript("OnClick", ctm_host.exclusive_onclick)
	
	-- this is really bad..	
	if self.num_buttons == 1 then
		button:SetChecked(true)
		self.checkedID = 1
	end
	
end

ctm_host.title_fontstr = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormal");
ctm_host.title_fontstr:SetPoint("TOPLEFT", ctm_host.first_pos_x-15, ctm_host.first_pos_y);
ctm_host.title_fontstr:SetText(ctm_host.title)

-- first one is default :P

ctm_host:add_button("Local");
ctm_host:add_button("Target");
ctm_host:add_button("Everyone");
ctm_host:add_button("Healers");
ctm_host:add_button("Casters");
ctm_host:add_button("Melee");



-- local cc_dropdown = CreateFrame("Frame", "cc_dropdown", lole_frame, "UIDropDownMenuTemplate");
-- cc_dropdown:SetPoint("TOPLEFT", 10, -200);

-- cc_dropdown:SetScale(0.75);

-- UIDropDownMenu_SetWidth(100, cc_dropdown)

-- local function cc_drop_onClick(name, GUID)
	-- echo("mu ballir " .. name .. " " .. GUID)
-- end
 
-- local function cc_drop_initialize()
	
	-- local info = {}
	-- local n = 1;
   
	-- for i=1,4,1 do 
		-- local exists = GetPartyMember(i)
	 
		-- if exists then
			-- local id = "party" .. tostring(i)
			-- local _, class, _ = UnitClass(id);
			
			-- if class == "MAGE" or
			   -- class == "WARLOCK" or
			   -- class == "DRUID" or
			   -- class == "ROGUE" then
				-- -- no other classes can CC, since we don't have a hunter or a rogue
				-- info.text = UnitName(id);
				-- info.value = n;
				-- info.arg1 = info.text;
				-- info.arg2 = UnitGUID(id);
				-- info.checked = nil
				-- info.func = cc_drop_onClick;
				-- UIDropDownMenu_AddButton(info)
				-- n = n + 1
			-- end
		-- end
	-- end
	
-- end
 
-- --UIDropDownMenu_Initialize(cc_dropdown, cc_drop_initialize)
-- UIDropDownMenu_SetSelectedID(cc_dropdown, nil)





