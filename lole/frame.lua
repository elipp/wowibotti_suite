lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("ADDON_LOADED");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:RegisterEvent("PLAYER_DEAD");
lole_frame:RegisterEvent("UPDATE_BATTLEFIELD_STATUS")
lole_frame:RegisterEvent("PARTY_INVITE_REQUEST")

local every_nth_frame = 4
local frame_modulo = 0

lole_frame:SetScript("OnUpdate", function()
	if get_blast_state() and frame_modulo == 0 then
		do_CC_jobs()
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

		blast_check_settext(false)

		if not IsRaidLeader() then
			lbuffcheck_clean_button:Disable()
			lbuffcheck_button:Disable()
		end

		lole_frame:UnregisterEvent("ADDON_LOADED");

	elseif event == "PLAYER_DEAD" then
		clear_target();

	elseif event == "PLAYER_REGEN_DISABLED" then
		if IsRaidLeader() then
		--	broadcast_follow_target(NOTARGET); -- REMEMBER TO REMOVE THIS!!
		end
	elseif event == "PLAYER_REGEN_ENABLED" then
		if IsRaidLeader() then
			broadcast_blast_state(0);
		end

	elseif event == "UPDATE_BATTLEFIELD_STATUS" then
		-- lol

	elseif event == "PARTY_INVITE_REQUEST" then
	--	if GetNumRaidMembers() > 0 then return end

		local guildies = get_online_guild_members()

		if guildies[prefix] then
			self:RegisterEvent("PARTY_MEMBERS_CHANGED");
			AcceptGroup()
		end

	elseif event == "PARTY_MEMBERS_CHANGED" then
		StaticPopup_Hide("PARTY_INVITE")
		self:UnregisterEvent("PARTY_MEMBERS_CHANGED")

		if IsRaidLeader() then
			lole_subcommands.raid()
		end
	end


end

lole_frame:SetScript("OnEvent", LOLE_EventHandler);

lole_frame:SetHeight(290)
lole_frame:SetWidth(250)
lole_frame:SetPoint("RIGHT", -25, 0)


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

local blast_disabled_string = " BLAST disabled"
local blast_enabled_string = " BLAST enabled!"

blast_checkbutton.tooltip = "Whether BLAST is on or off.";

blast_checkbutton:SetScript("OnClick",
  function()
	local arg = blast_checkbutton:GetChecked() and "1" or "0"; -- this is equivalent to C's ternary operator ('?')
	lole_subcommands.blast(arg)
  end
);

function blast_check_settext(flag)
	if (flag == true) then
		getglobal(blast_checkbutton:GetName() .. "Text"):SetText(blast_enabled_string)
	elseif (flag == false) then
		getglobal(blast_checkbutton:GetName() .. "Text"):SetText(blast_disabled_string)
	end
end

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

local config_dropdown = CreateFrame("Frame", "config_dropdown", lole_frame, "UIDropDownMenuTemplate");
config_dropdown:SetPoint("BOTTOMLEFT", 42, 10);
config_dropdown:SetScale(0.88)

UIDropDownMenu_SetWidth(100, config_dropdown)

local config_text = config_dropdown:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
config_text:SetPoint("LEFT", -23, 3);
config_text:SetText("Config:")

--local r,g,b,a = config_text:GetTextColor();
--echo(r .. " " .. g .. " " ..  b .. " " .. a)


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

local function create_simple_button(name, parent, x, y, text, width, height, onclick, scale) -- scale optional
	local button = CreateFrame("Button", name, parent, "UIPanelButtonTemplate");

	-- SetPoint (and SetWidth/Height) behaves a bit differently (i think) depending on whether SetScale was called before or after
	if scale then
		button:SetScale(scale);
	else
		button:SetScale(0.75);
	end

	button:SetPoint("TOPLEFT", x, y);
	button:SetWidth(width);
	button:SetHeight(height);
	button:SetText(text);
	button:SetScript("OnClick", onclick);

	return button
end

local follow_button =
create_simple_button("follow_button", lole_frame, 22, -100, "Follow me!", 85, 27, function() lole_subcommands.followme() end);

local stopfollow_button =
create_simple_button("stopfollow_button", lole_frame, 22, -130, "Stopfollow", 85, 27, function() lole_subcommands.stopfollow() end);

local function update_target_onclick()

	if not IsRaidLeader() then
		lole_error("Couldn't update BLAST target (you are not the raid leader!)");
		return;
	end

	if not UnitExists("target") then
		lole_error("Couldn't update BLAST target (no valid mob selected!)");
		return;
	end

	local target_GUID = UnitGUID("target");

	if not target_GUID then
		clear_target();
		broadcast_target_GUID(NOTARGET);
		return
	end

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
end

local update_target_button =
create_simple_button("update_target_button", lole_frame, 120, -100, "Update target", 115, 27, update_target_onclick);

local function clear_target_onclick()

	if not IsRaidLeader() then
		lole_error("Couldn't clear BLAST target (you are not the raid leader!)");
		return;
	end

	clear_target();
	broadcast_target_GUID(NOTARGET);
end

local clear_target_button =
create_simple_button("clear_target_button", lole_frame, 250, -100, "Clear", 62, 27, clear_target_onclick);

local cooldowns_button =
create_simple_button("cooldowns_button", lole_frame, 120, -130, "Cooldowns", 115, 27, function() broadcast_cooldowns() end);

local drink_button =
create_simple_button("drink_button", lole_frame, 250, -130, "Drink", 62, 27, function() broadcast_drink() end);

local lbuffcheck_clean_button =
create_simple_button("lbuffcheck_clean_button", lole_frame, 120, -160, "Buff (clean)", 115, 27, function() lole_subcommands.lbuffcheck("clean") end);

local lbuffcheck_button =
create_simple_button("lbuffcheck_button", lole_frame, 250, -160, "Buff", 62, 27, function() lole_subcommands.lbuffcheck() end);

local ctm_host = { title = "CTM mode:", title_fontstr = nil, buttons = {}, num_buttons = 0, first_pos_x = 223, first_pos_y = -205, increment = 18 }

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
	button:SetPoint("TOPLEFT", self.first_pos_x, self.first_pos_y - self.increment*self.num_buttons);
	button:SetScript("OnClick", ctm_host.exclusive_onclick)
	button:SetScale(0.75);

	-- this is really bad..
	if self.num_buttons == 1 then
		button:SetChecked(true)
		self.checkedID = 1
	end

end

ctm_host.title_fontstr = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall");
ctm_host.title_fontstr:SetPoint("TOPLEFT", 162, -152);
ctm_host.title_fontstr:SetText(ctm_host.title)

-- first one is default :P

ctm_host:add_button("Local");
ctm_host:add_button("Target");
ctm_host:add_button("Everyone");
--ctm_host:add_button("Healers");
--ctm_host:add_button("Casters");
--ctm_host:add_button("Melee");

local mode_attrib_title_fontstr = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall");
mode_attrib_title_fontstr:SetPoint("TOPLEFT", 160, -225);
mode_attrib_title_fontstr:SetText("Mode attribs:")

local playermode_checkbutton = CreateFrame("CheckButton", "playermode_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
playermode_checkbutton:SetPoint("TOPLEFT", 205, -300);
playermode_checkbutton.tooltip = "Set to enabled if you're playing this character.";
playermode_checkbutton:SetScale(0.8)

--local mattrib_font = playermode_checkbutton:CreateFontString(nil, "OVERLAY")
--mattrib_font:SetFont("Fonts\\FRIZQT__.TTF", 9);

getglobal(playermode_checkbutton:GetName() .. "Text"):SetFont("Fonts\\FRIZQT__.TTF", 9)
getglobal(playermode_checkbutton:GetName() .. "Text"):SetText("|cFFFFD100Player mode")

playermode_checkbutton:SetScript("OnClick",
  function()
	local arg = playermode_checkbutton:GetChecked() and "1" or "0";
	lole_subcommands.set("playermode", arg);
  end
);

local aoemode_checkbutton = CreateFrame("CheckButton", "aoemode_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
aoemode_checkbutton:SetPoint("TOPLEFT", 205, -320);
aoemode_checkbutton.tooltip = "AOE spells only";
aoemode_checkbutton:SetScale(0.8)

getglobal(aoemode_checkbutton:GetName() .. "Text"):SetFont("Fonts\\FRIZQT__.TTF", 9)
getglobal(aoemode_checkbutton:GetName() .. "Text"):SetText("|cFFFFD100AOE mode")

aoemode_checkbutton:SetScript("OnClick",
  function()
	local arg = aoemode_checkbutton:GetChecked() and "1" or "0";
	lole_subcommands.set("aoemode", arg);
  end
);

local CC_state = {}
local num_CC_targets = 0
local CC_base_y = -140

local function delete_CC_entry(CC_entry)
	local CC_host = CC_entry:GetParent()

	if CC_host.ID > num_CC_targets then
		lole_error("attempting to delete CC entry " .. CC_host.ID  .. " (index too damn high!)")
		return false
	end

	local hostID = CC_host.ID

	CC_host:Hide()
	disable_cc_target(CC_host.char, CC_host.spell, CC_host.marker);

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
	sheep = 118,

	Cyclone = 33786,
	cyclone = 33786,

	["Entangling Roots"] = 26989,
	Roots = 26989,
	roots = 26989,
	root = 26989,

	Banish = 18647,
	banish = 18647,
	ban = 18647,

	Fear = 6215,
	fear = 6215,

	["Shackle Undead"] = 10955,
	Shackle = 10955,
	shackle = 10955,

	["Turn Evil"] = 10326,
	Turn = 10326,
	turn = 10326,
}

local CC_spellnames = { -- in a CastSpellByName-able format
	[118] = "Polymorph",
	[33786] = "Cyclone",
	[26989] = "Entangling Roots",
	[18647] = "Banish",
	[6215] = "Fear",
	[10955] = "Shackle Undead",
	[10326] = "Turn Evil",
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

local function new_CC(char, spellID, marker)
	-- echo("Asking " .. trim_string(char) .. " to do " .. trim_string(spell) .. " on " .. trim_string(marker) .. "!")

	if not UnitExists(char) then
		lole_error("Character " .. char .. " doesn't appear to exist!")
		return false
	end

	if not spellID then
		lole_error("Unknown CC spell!");
		return false
	end

	local spell = CC_spellnames[spellID]; -- get the spellname in a CastSpellByName-able format

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
	caster_char_text:SetText("|cFF" .. get_class_color(UnitClass(char)) .. char)

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

		local spellID = CC_spells[spell]
		new_CC(char, spellID, marker)

		local spell = CC_spellnames[spellID];
		enable_cc_target(char, spell, marker)
	end,

};

local add_cc_button =
create_simple_button("add_cc_button", lole_frame, 22, -175, "Add CC...", 85, 27, function() StaticPopup_Show("ADD_CC_DIALOG") end);
