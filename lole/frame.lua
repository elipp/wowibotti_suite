lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("ADDON_LOADED");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:RegisterEvent("PLAYER_DEAD");
lole_frame:RegisterEvent("UPDATE_BATTLEFIELD_STATUS")

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
	elseif event == "UPDATE_BATTLEFIELD_STATUS" then
		
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

update_target_button:SetPoint("TOPLEFT", 128, -100);
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
		-- mob GUIDs always start with 0xF130
		if string.sub(target_GUID, 1, 6) == "0xF130" and (not UnitIsDead("target")) and UnitReaction("target", "player") < 5 then
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

local function get_available_CC()
	local CC_table = nil
	
	for i=1,4,1 do 
		local exists = GetPartyMember(i)
		if exists then
			if not CC_table then CC_table = {} end
			
			local id = "party" .. tostring(i)
			local name = UnitName(id)
			local _, class, _ = UnitClass(id);
			
			if class == "MAGE" then
				CC_table[name] = { "Polymorph" }
			elseif class == "WARLOCK" then
				CC_table[name] = { "Fear", "Banish" }
			elseif class == "DRUID" then
				CC_table[name] = { "Cyclone", "Hibernate", "Entangling Roots" }
			end
		end
	end
	
	return CC_table;
end

local CC_state = {}
local num_CC_targets = 0
local CC_base_y = -140

local function delete_CC_entry(CC_entry)
	local CC_host = CC_entry:GetParent()

	if CC_host.ID > num_CC_targets then
		lole_error("attempting to delete CC entry " .. CC_host.ID  .. " (index too damn high!)")
		return false
	end
	
	-- if it's the last one that's being deleted, it's easy.
	
	if CC_host.ID  == num_CC_targets then
		table.remove(CC_state)
		num_CC_targets = num_CC_targets - 1
		CC_host:Hide() -- there's no way to really destroy a Frame in wow LUA
		disable_cc_target(CC_host.char, CC_host.marker);
		return
	end

	--local num_relocate = num_CC_targets - CC_host.ID;
	--echo("need to relocate " .. num_relocate .. " CC entries.")
	
	local hostID = CC_host.ID
	
	CC_host:Hide()
	table.remove(CC_state, hostID);
	num_CC_targets = num_CC_targets - 1
	
	for i = hostID, num_CC_targets do 
		CC_state[i]:SetPoint("TOPLEFT", 18, CC_base_y - 20*i)
		CC_state[i].ID = i;
	end
		
end

local CC_spells = {
	Polymorph = 118,
	Sheep = 118,
	Cyclone = 33786,
	["Entangling Roots"] = 26989,
	Roots = 26989,
	Banish = 18647,
	Fear = 6215,
	Shackle = 10955,
	["Shackle Undead"] = 10955
}

local CC_frame_backdrop = {
	--bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",  
	edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
	  -- true to repeat the background texture to fill the frame, false to scale it
	tile = false,
	  -- size (width or height) of the square repeating background tiles (in pixels)
	tileSize = 32,
	  -- thickness of edge segments and square size of edge corners (in pixels)
	edgeSize = 8,
	  -- distance from the edges of the frame to those of the background texture (in pixels)
	insets = {
		left = 0,
		right = 0,
		top = 0,
		bottom = 0
	}
}

local function new_CC(char, spell, marker)
	-- echo("Asking " .. trim_string(char) .. " to do " .. trim_string(spell) .. " on " .. trim_string(marker) .. "!")

	if not UnitExists(char) then
		lole_error("Character " .. char .. " doesn't appear to exist!")
		return false
	end
	
	local spellID = CC_spells[spell]
	
	if not spellID then 
		lole_error("Unknown spell " .. spell .. "!");
		return false
	end
	
	local name, rank, icon, castTime, minRange, maxRange = GetSpellInfo(spellID)

	if not get_marker_index(marker) then
		lole_error("Invalid marker name " .. marker .. "!")
		return false
	end
		
	num_CC_targets = num_CC_targets + 1
		
	local CC_host = CreateFrame("Frame", nil, lole_frame);
	
	CC_host.char = char
	CC_host.spell = spell
	CC_host.marker = marker
	
	CC_host:SetWidth(100)
	CC_host:SetHeight(22)
	
	CC_host:SetBackdrop(CC_frame_backdrop)
	
	local y = CC_base_y - (num_CC_targets*20)
	
	CC_host:SetPoint("TOPLEFT", 18, y)
	
	CC_host.ID = num_CC_targets
	
	local icon_frame = CreateFrame("Frame", nil, CC_host)
	icon_frame:SetWidth(16)
	icon_frame:SetHeight(16)

	local icon_texture = icon_frame:CreateTexture(nil,"BACKGROUND")
	icon_texture:SetTexture(icon)
	icon_texture:SetAllPoints(icon_frame)
	
	icon_frame.texture = icon_texture

	local y = -140 - (num_CC_targets*20)
	
	icon_frame:SetPoint("TOPLEFT", 60, -3)
	icon_frame:Show()
	
	local caster_char_text = CC_host:CreateFontString(nil, "OVERLAY")
	caster_char_text:SetFont("Fonts\\FRIZQT__.TTF", 9);
	
	caster_char_text:SetPoint("TOPLEFT", 3, -6);
	caster_char_text:SetText("|cFF" .. class_color(UnitClass(char)) .. char)
	
	local marker_frame = CreateFrame("Frame", nil, CC_host)
	marker_frame:SetWidth(16)
	marker_frame:SetHeight(16)
	
	local marker_texture = marker_frame:CreateTexture(nil, "BACKGROUND");
	marker_texture:SetTexture("Interface\\TARGETINGFRAME\\UI-RaidTargetingIcon_" .. get_marker_index(marker))
	marker_texture:SetAllPoints(marker_frame)
	
	marker_frame.texture = marker_texture
	
	marker_frame:SetPoint("TOPLEFT", 80, -3);
	marker_frame:Show()
	
	local delete_button = CreateFrame("Button", nil, CC_host)
	
	delete_button:SetWidth(12)
	delete_button:SetHeight(12)
	
	delete_button:SetPoint("TOPLEFT", 100, -4);
	
	delete_button:SetNormalTexture("Interface\\Buttons\\UI-MinusButton-Up");
	--delete_button:SetHighlightTexture("Interface\\Buttons\\UI-MinusButton-Highlight"
	delete_button:SetPushedTexture("Interface\\Buttons\\UI-MinusButton-Down");
	
	delete_button:SetScript("OnClick", function(self) delete_CC_entry(self) end)
	
	CC_state[num_CC_targets] = CC_host;
	
end

StaticPopupDialogs["ADD_CC_DIALOG"] = {
	text = "CC syntax: <char>, <spell>, <marker>",
	button1 = "OK",
	button2 = "Cancel",

	timeout = 0,
	whileDead = 1,
	hideOnEscape = 1,
	hasEditBox = 1,
	hasWideEditBox = 1,
	
	OnShow = function()
		getglobal(this:GetName().."WideEditBox"):SetText("")
	end,
	
	OnAccept = function()
		local text = getglobal(this:GetParent():GetName().."WideEditBox"):GetText()
		local _char, _spell, _marker = strsplit(",", text);
		local char, spell, marker = trim_string(_char), trim_string(_spell), trim_string(_marker)
		new_CC(char, spell, marker)
		enable_cc_target(char, spell, marker)
	end,
  
};


-- spare these in case ;P


-- local cc_dropdowns = {}
-- local num_cc_dropdowns = 0
	
-- local cc_drop_onClick = function(name, GUID)
	-- echo("clicked on  " .. name .. " " .. GUID)
	-- UIDropDownMenu_SetSelectedID(cc_dropdowns[1], 2)
-- end

-- local function create_cc_dropdown()
	
	-- num_cc_dropdowns = num_cc_dropdowns + 1
	
	-- local y = -260 - (num_cc_dropdowns * 30)
	
	-- local cc_dropdown = CreateFrame("Frame", "cc_dropdown" .. tostring(num_cc_dropdowns), lole_frame, "UIDropDownMenuTemplate");
	-- cc_dropdown:SetScale(0.55);
	-- cc_dropdown:SetPoint("TOPLEFT", 10, y);
	-- UIDropDownMenu_SetWidth(100, cc_dropdown)
	
	-- local available_CC = get_available_CC();

	-- local cc_drop_initialize = function()
	
		-- local info = {}
		-- local n = 1;
	   
	   	-- info.text = "Select char";
		-- info.value = n;
	
		-- UIDropDownMenu_AddButton(info)
		-- n = n + 1
	   
		-- if not available_CC then
			-- info.text = "CC N/A"
			-- info.value = 1;
			-- info.checked = nil;
			-- UIDropDownMenu_AddButton(info)
		-- else
			-- for name, spells in pairs(available_CC) do
			
				-- info.text = name;
				-- info.value = n;
				-- info.arg1 = name;
				-- info.arg2 = UnitGUID(name);
				-- info.checked = nil
				-- info.func = cc_drop_onClick;
				-- UIDropDownMenu_AddButton(info)
				-- n = n + 1
				
			-- end
		-- end
	-- end
	
	-- UIDropDownMenu_Initialize(cc_dropdown, cc_drop_initialize)
	-- UIDropDownMenu_SetSelectedID(cc_dropdown, 1)

	-- local cc_spell_dropdown_initialize = function()
		
		-- local selected_id = UIDropDownMenu_GetSelectedValue(cc_dropdown);
		-- local m = 1;
		
		-- echo(tostring(selected_id))
		
		-- if not selected_id then
			-- echo("pillu.")
			-- info.text = "NO CHAR YET"
			-- info.value = 1;
			-- info.checked = true
			-- UIDropDownMenu_AddButton(info);
			-- m = m + 1
			-- return
		-- else 
			-- local selected_name = UIDropDownMenu_GetSelectedName(cc_dropdown);
			-- for i, spell in pairs(available_CC[selected_name]) do
				-- info.text = spell
				-- info.value = m;
				-- info.checked = nil
				-- UIDropDownMenu_AddButton(info);
				-- m = m + 1
			-- end
		-- end
	-- end

	-- local cc_spell_dropdown = CreateFrame("Frame", "cc_spell_dropdown" .. tostring(num_cc_dropdowns), cc_dropdown, "UIDropDownMenuTemplate");
	-- cc_spell_dropdown:SetPoint("TOPLEFT", 120, 0);
	-- UIDropDownMenu_SetWidth(100, cc_spell_dropdown)

	-- UIDropDownMenu_Initialize(cc_spell_dropdown, cc_spell_dropdown_initialize)
	-- UIDropDownMenu_SetSelectedID(cc_spell_dropdown, 1)

	-- cc_dropdowns[num_cc_dropdowns] = cc_dropdown

-- end


local add_cc_button = CreateFrame("Button", "add_cc_button", lole_frame, "UIPanelButtonTemplate")
add_cc_button:SetScale(0.75)

add_cc_button:SetPoint("TOPLEFT", 22, -175);
add_cc_button:SetHeight(27)
add_cc_button:SetWidth(85)

add_cc_button:SetText("Add CC...");

add_cc_button:SetScript("OnClick", function()
	StaticPopup_Show("ADD_CC_DIALOG")
end)

