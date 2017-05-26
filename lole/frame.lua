local lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("ADDON_LOADED")
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED") -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED") -- and this when combat is over
lole_frame:RegisterEvent("PLAYER_DEAD")
--lole_frame:RegisterEvent("UPDATE_BATTLEFIELD_STATUS")
lole_frame:RegisterEvent("PARTY_INVITE_REQUEST")
lole_frame:RegisterEvent("RESURRECT_REQUEST")
lole_frame:RegisterEvent("CONFIRM_SUMMON")
--lole_frame:RegisterEvent("LOOT_OPENED")
lole_frame:RegisterEvent("CVAR_UPDATE")
lole_frame:RegisterEvent("PLAYER_ENTERING_WORLD")
lole_frame:RegisterEvent("PLAYER_LOGOUT")
lole_frame:RegisterEvent("TRADE_SHOW")
lole_frame:RegisterEvent("LFG_PROPOSAL_SHOW")
lole_frame:RegisterEvent("LFG_ROLE_CHECK_SHOW")


function lole_frame_register(EVENTNAME)
	lole_frame:RegisterEvent(EVENTNAME)
end

function lole_frame_unregister(EVENTNAME)
	lole_frame:UnRegisterEvent(EVENTNAME)
end

LOOT_OPENED_REASON = nil

local every_4th_frame = 0
local every_30th_frame = 0

local time_since_dcheck = 0

local function check_durability()

	if (time() - time_since_dcheck > 300) then
		if get_durability_status() == false then
			SendChatMessage("RIP i hawe progen kears!!", "GUILD")
		end
		time_since_dcheck = time()
	end

end

local function set_button_states()
	-- is this really necessary? :P
	if not IsRaidLeader() then -- it just wasn't reliable enough to do this in ADDON_LOADED
		if lbuffcheck_clean_button:IsEnabled() == 1 then lbuffcheck_clean_button:Disable() end
		if lbuffcheck_button:IsEnabled() == 1 then lbuffcheck_button:Disable() end
		if add_cc_button:IsEnabled() == 1 then add_cc_button:Disable() end
	else
		if lbuffcheck_clean_button:IsEnabled() == 0 then lbuffcheck_clean_button:Enable() end
		if lbuffcheck_button:IsEnabled() == 0 then lbuffcheck_button:Enable() end
		if add_cc_button:IsEnabled() == 0 then add_cc_button:Enable() end
	end

end

lole_frame:SetHeight(380)
lole_frame:SetWidth(290)
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

local blast_checkbutton = CreateFrame("CheckButton", "blast_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
blast_checkbutton:SetPoint("TOPLEFT", 15, -30);
blast_checkbutton:SetHitRectInsets(0, -80, 0, 0)

blast_checkbutton:SetScript("OnClick",
  function()
		local arg = blast_checkbutton:GetChecked() and 1 or 0; -- this is equivalent to C's ternary operator ('?')
		lole_subcommands.broadcast("set", "blast", arg)
  end
);

local function blast_check_settext(text)
	getglobal(blast_checkbutton:GetName() .. "Text"):SetText(text)
end

function gui_set_blast(arg)
	if arg == 1 then
		blast_checkbutton:SetChecked(true)
		blast_check_settext(" BLAST ON!");
	else
		blast_checkbutton:SetChecked(false)
		blast_check_settext(" BLAST off!");
	end
end

local heal_blast_checkbutton = CreateFrame("CheckButton", "heal_blast_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
heal_blast_checkbutton:SetPoint("TOPLEFT", 120, -30);
heal_blast_checkbutton:SetHitRectInsets(0, -80, 0, 0)

heal_blast_checkbutton:SetScript("OnClick",
  function()
		local arg = heal_blast_checkbutton:GetChecked() and 1 or 0;
		lole_subcommands.broadcast("set", "heal_blast", arg)
  end
);

local function heal_blast_check_settext(text)
	getglobal(heal_blast_checkbutton:GetName() .. "Text"):SetText(text)
end

function gui_set_heal_blast(arg)
	if arg == 1 then
		heal_blast_checkbutton:SetChecked(true)
		heal_blast_check_settext(" HEALING ON!");
	else
		heal_blast_checkbutton:SetChecked(false)
		heal_blast_check_settext(" HEALING off!");
	end
end


local static_target_text = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
static_target_text:SetPoint("TOPLEFT", 18, -55);
static_target_text:SetText("Current blast target:")

local target_text = lole_frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
target_text:SetPoint("TOPLEFT", 126, -55);
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



local config_dropdown = CreateFrame("Frame", "config_dropdown", lole_frame, "UIDropDownMenuTemplate")
config_dropdown:ClearAllPoints()
config_dropdown:SetPoint("BOTTOMLEFT", 42, 35)
config_dropdown:Show()


local config_text = config_dropdown:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
config_text:SetPoint("LEFT", -23, 3);
config_text:SetText("Config:")

local drop_configs = {}

for k, v in pairs_by_key(get_available_configs()) do
	drop_configs[#drop_configs + 1] = { unformatted = k, formatted = "|cFF" .. v.color .. k }
end

local function config_drop_onclick(self)
	UIDropDownMenu_SetSelectedID(config_dropdown, self:GetID())
	lole_subcommands.setconfig(self.value)
end

local function config_drop_initialize(self, level)
	local info = UIDropDownMenu_CreateInfo()
	for k,v in pairs(drop_configs) do
		info = UIDropDownMenu_CreateInfo()
		info.text = v.formatted
		info.value = v.unformatted
		info.func = config_drop_onclick
		UIDropDownMenu_AddButton(info, level)
	end
end

UIDropDownMenu_Initialize(config_dropdown, config_drop_initialize)
UIDropDownMenu_SetWidth(config_dropdown, 100);
UIDropDownMenu_SetButtonWidth(config_dropdown, 124)
UIDropDownMenu_SetSelectedValue(config_dropdown, get_current_config().name)
UIDropDownMenu_JustifyText(config_dropdown, "LEFT")

function set_visible_dropdown_config(configname)
	for k,v in pairs(drop_configs) do
		if v.unformatted == configname then
			UIDropDownMenu_SetSelectedValue(config_dropdown, v.unformatted)
		end
	end
end

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
create_simple_button("stopfollow_button", lole_frame, 22, -130, "Stopfollow", 85, 27,
function() lole_subcommands.broadcast("stopfollow") end);

local function update_target_onclick()

	-- if not IsRaidLeader() then
	-- 	lole_error("Couldn't update BLAST target (you are not the raid leader!)");
	-- 	return;
	-- end

	if not UnitExists("target") then
		lole_error("Couldn't update BLAST target (no valid mob selected!)");
		return;
	end

	local target_GUID = UnitGUID("target");

	if not target_GUID then
		clear_target();
		lole_subcommands.broadcast("target", NOTARGET);
		return
	end

	if target_GUID ~= BLAST_TARGET_GUID then
		-- mob GUIDs always start with 0xF130
		if string.sub(target_GUID, 1, 6) == "0xF130" and (not UnitIsDead("target")) and UnitReaction("target", "player") < 5 then
			set_target(UnitGUID("target"))
			lole_subcommands.broadcast("target", UnitGUID("target"));
			return;
		else
			lole_error("Invalid target! Select an attackable NPC mob.")
			return;
		end
	end
end

local update_target_button =
create_simple_button("update_target_button", lole_frame, 115, -100, "Update target", 115, 27, update_target_onclick);

function update_target()
	update_target_onclick()
end

local function clear_target_onclick()
	clear_target();
	lole_subcommands.broadcast("target", NOTARGET);
end

local clear_target_button =
create_simple_button("clear_target_button", lole_frame, 240, -100, "Clear", 62, 27, clear_target_onclick);

local cooldowns_button =
create_simple_button("cooldowns_button", lole_frame, 115, -130, "Cooldowns", 115, 27,
function()
	lole_subcommands.broadcast("cooldowns");
end);

local drink_button =
create_simple_button("drink_button", lole_frame, 240, -130, "Drink", 62, 27,
function()
	lole_subcommands.broadcast("drink")
end);

local lbuffcheck_clean_button =
create_simple_button("lbuffcheck_clean_button", lole_frame, 115, -160, "Buff (clean)", 115, 27, function() lole_subcommands.lbuffcheck("clean") end);

local lbuffcheck_button =
create_simple_button("lbuffcheck_button", lole_frame, 240, -160, "Buff", 62, 27, function() lole_subcommands.lbuffcheck() end);

local reloadui_button =
create_simple_button("reloadui_button", lole_frame, 310, -100, "Reload", 68, 27, function() execute_script("RunMacroText(\"/console reloadui\")") end);

local getbiscuit_button =
create_simple_button("getbiscuit_button", lole_frame, 310, -130, "Biscuit", 68, 27, function() lole_subcommands.broadcast("getbiscuits") end);

local loot_badge_button =
create_simple_button("loot_badge_button", lole_frame, 310, -160, "Badge", 68, 27, function() lole_subcommands.broadcast("loot_badge", UnitGUID("target")) end);

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
mode_attrib_title_fontstr:SetPoint("BOTTOMRIGHT", -25, 80);
mode_attrib_title_fontstr:SetText("Mode attribs:")

local playermode_checkbutton = CreateFrame("CheckButton", "playermode_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
playermode_checkbutton:SetPoint("BOTTOMRIGHT", -67, 50);
playermode_checkbutton.tooltip = "Set to enabled if you're playing this character.";
--playermode_checkbutton:SetScale(0.8)

--local mattrib_font = playermode_checkbutton:CreateFontString(nil, "OVERLAY")
--mattrib_font:SetFont("Fonts\\FRIZQT__.TTF", 9);

getglobal(playermode_checkbutton:GetName() .. "Text"):SetFont("Fonts\\FRIZQT__.TTF", 8)
getglobal(playermode_checkbutton:GetName() .. "Text"):SetText("Player mode")

playermode_checkbutton:SetScript("OnClick",
  function()
	local arg = playermode_checkbutton:GetChecked() and "1" or "0";
	lole_subcommands.set("playermode", arg);
  end
);

-- COLOR CODE FOR E.G. GameFontNormalSmall TEMPLATE: |cFFFFD100

local aoemode_checkbutton = CreateFrame("CheckButton", "aoemode_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
aoemode_checkbutton:SetPoint("BOTTOMRIGHT", -67, 30);
aoemode_checkbutton.tooltip = "AOE spells only";
--aoemode_checkbutton:SetScale(0.8)

getglobal(aoemode_checkbutton:GetName() .. "Text"):SetFont("Fonts\\FRIZQT__.TTF", 8)
getglobal(aoemode_checkbutton:GetName() .. "Text"):SetText("AOE mode")

aoemode_checkbutton:SetScript("OnClick",
  function()
	local arg = aoemode_checkbutton:GetChecked() and "1" or "0";
	lole_subcommands.set("aoemode", arg);
  end
);

local mode_attrib_checkboxes = {
	["playermode"] = playermode_checkbutton,
	["aoemode"] = aoemode_checkbutton,
	["blast"] = blast_checkbutton,
	["heal_blast"] = heal_blast_checkbutton
}

function update_mode_attrib_checkbox_states()
	for attrib, checkbutton in pairs(mode_attrib_checkboxes) do
		if lole_subcommands.get(attrib) == 1 then
			checkbutton:SetChecked(true)
		else
			checkbutton:SetChecked(false)
		end
	end
end

------------------------------------------------
------------------ CC STUFF --------------------
------------------------------------------------

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


local CC_state = {}
-- CC_state contains active CC targets with the raid marker as key
local num_CC_targets = 0

local CC_base_y = -140

function delete_CC_entry(CC_marker)

	local CC_host = CC_state[CC_marker];
	if not CC_host then
		echo("(warning: attempt to delete CC entry for " .. CC_marker .. ", which doesn't exist. Maybe double-posted AddonMessage?)")
		return
	end

	if CC_host.ID > num_CC_targets then
		lole_error("attempting to delete CC entry " .. CC_host.ID  .. " (index too damn high!)")
		return false
	end

	local hostID = CC_host.ID

	CC_host:Hide()
	disable_cc_target(CC_host.char_name, CC_host.marker);

	--table.remove(CC_state, hostID);
	CC_state[CC_host.marker] = nil;

	num_CC_targets = num_CC_targets - 1

	for marker, entry in pairs(CC_state) do
		if entry.ID > hostID then
		 	entry.ID = entry.ID - 1;
			entry:SetPoint("TOPLEFT", 18, CC_base_y - 20*entry.ID)
		end
	end

end

function delete_all_CC_entries()
	for marker, entry in pairs(CC_state) do
		entry:Hide()
		CC_state[marker] = nil;
	end
end

function new_CC(char_name, marker, spellID)
	-- echo("Asking " .. trim_string(char) .. " to do " .. trim_string(spell) .. " on " .. trim_string(marker) .. "!")

	if CC_state[marker] then
		local cc = CC_state[marker];
		if cc.char_name:lower() == char_name:lower() and cc.spellID == spellID then
			echo("(new_CC request is identical (normal double-posted AddonMessage?), ignoring.)")
		else
			lole_error("A conflicting entry for marker \"" .. marker .. "\" seems to already exist!")
			lole_error("(info: " .. cc.char_name .. ":" .. get_CC_spellname(cc.spellID) .. "). Please delete this one before reassigning.")
		end

		return
	end

	if not UnitExists(char_name) then
		lole_error("Character " .. char_name .. " doesn't appear to exist!")
		return false
	end

	local spellname = get_CC_spellname(spellID); -- get the spellname in a L_CastSpellByName-able format

	if not spellname then
		lole_error("Unknown CC spell with ID " .. tostring(spellID));
		return false
	end

	local name, rank, icon, castTime, minRange, maxRange = GetSpellInfo(spellID)

	if not get_marker_index(marker) then
		lole_error("Invalid marker name " .. marker .. "!")
		return false
	end

	num_CC_targets = num_CC_targets + 1

	local CC_host = CreateFrame("Frame", nil, lole_frame);

	CC_host.char_name = first_to_upper(char_name)
	CC_host.spellID = spellID
	CC_host.marker = marker

	CC_host.ID = num_CC_targets

	CC_host:SetWidth(100)
	CC_host:SetHeight(22)

	CC_host:SetBackdrop(CC_frame_backdrop)

	local y = CC_base_y - (num_CC_targets*20)

	CC_host:SetPoint("TOPLEFT", 18, y)

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
	caster_char_text:SetText("|cFF" .. get_class_color(UnitClass(CC_host.char_name)) .. CC_host.char_name)

	local marker_frame = CreateFrame("Frame", nil, CC_host)
	marker_frame:SetWidth(16)
	marker_frame:SetHeight(16)

	local marker_texture = marker_frame:CreateTexture(nil, "BACKGROUND");
	marker_texture:SetTexture("Interface\\TARGETINGFRAME\\UI-RaidTargetingIcon_" .. get_marker_index(marker))
	marker_texture:SetAllPoints(marker_frame)

	marker_frame.texture = marker_texture

	marker_frame:SetPoint("TOPLEFT", 80, -3);
	marker_frame:Show()

	if IsRaidLeader() then
		local delete_button = CreateFrame("Button", nil, CC_host)

		delete_button:SetWidth(12)
		delete_button:SetHeight(12)

		delete_button:SetPoint("TOPLEFT", 100, -4);

		delete_button:SetNormalTexture("Interface\\Buttons\\UI-MinusButton-Up");
		--delete_button:SetHighlightTexture("Interface\\Buttons\\UI-MinusButton-Highlight"
		delete_button:SetPushedTexture("Interface\\Buttons\\UI-MinusButton-Down");

		delete_button:SetScript("OnClick", function(self) delete_CC_entry(self:GetParent().marker) end)
	end

	CC_state[marker] = CC_host;

end

local dialog_handle = nil -- dirty lil hack to hide the cc dialog from accept_cc
local function hide_cc_dialog()
	if dialog_handle then dialog_handle:Hide() end
	dialog_handle = nil
end


local function accept_cc(editbox_text)
	local _char, _marker, _spell = strsplit(" ", editbox_text);

	if not _char or not _marker or not _spell then
		lole_error("accept_cc: invalid arguments!")
		return
	end

	local char, marker, spell = trim_string(_char), trim_string(_marker), trim_string(_spell)

	local spellID = get_CC_spellID(spell)

	if not spellID then
		lole_error("accept_cc: unknown CC spell with name \"" .. spell .. "\"!")
		hide_cc_dialog()
		return
	end

	new_CC(char, marker, spellID)
	enable_cc_target(char, marker, spellID)

	hide_cc_dialog()
end

StaticPopupDialogs["ADD_CC_DIALOG"] = {
	text = "CC syntax: <character> <marker> <spell>",
	button1 = "OK",
	button2 = "Cancel",

	timeout = 0,
	whileDead = 1,
	hideOnEscape = 1,
	hasEditBox = 1,
	hasWideEditBox = 1,

	OnAccept = function()
		accept_cc(getglobal(this:GetParent():GetName().."WideEditBox"):GetText())
	end,

	OnShow = function()
		local edit = getglobal(this:GetName().."WideEditBox")
		edit:SetText("")
		edit:SetScript("OnEnterPressed", function() accept_cc(this:GetText()) end)
		edit:SetScript("OnEscapePressed", function() this:GetParent():Hide() end)
		dialog_handle = this
	end,

};

local add_cc_button =
create_simple_button("add_cc_button", lole_frame, 22, -175, "Add CC...", 85, 27, function() StaticPopup_Show("ADD_CC_DIALOG") end);


local inject_status_text = lole_frame:CreateFontString(nil, "OVERLAY");
inject_status_text:SetFont("Fonts\\FRIZQT__.TTF", 9);
inject_status_text:SetPoint("BOTTOMRIGHT", -22, 15);
inject_status_text:SetText("")

local last_status = true -- hack to enable initial injected status display

function update_injected_status(arg)
	if arg == last_status then return end

	if arg == true then
		inject_status_text:SetText("|cFF228B22(Injected)")
	else
		inject_status_text:SetText("|cFF800000(Not injected!)")
	end

	last_status = arg
end

local player_pos_text = lole_frame:CreateFontString(nil, "OVERLAY");
player_pos_text:SetFont("Fonts\\FRIZQT__.TTF", 8);
player_pos_text:SetPoint("BOTTOMLEFT", 12, 80);
player_pos_text:SetText("")

function update_player_pos_text(x, y, z)
	if not x then return end
	local pos = string.format("%.1f, %.1f, %.1f", x, y, z)
	player_pos_text:SetText("|cFFFFD100Player pos: |cFFFFFFFF" .. pos)
end

local target_pos_text = lole_frame:CreateFontString(nil, "OVERLAY");
target_pos_text:SetFont("Fonts\\FRIZQT__.TTF", 8);
target_pos_text:SetPoint("BOTTOMLEFT", 12, 66);
target_pos_text:SetText("")

function update_target_pos_text(x, y, z)
	local pos;
	if x then	pos = string.format("%.1f, %.1f, %.1f", x, y, z)
	else pos = "(no target)" end

	target_pos_text:SetText("|cFFFFD100Target pos: |cFFFFFFFF" .. pos)
end

local function do_combat_stuff()
	do_CC_jobs()
	lole_main()
end

local raid_zones = {
	["Gruul's Lair"] = 1,
	["Black Temple"] = 2,
	["Karazhan"] = 3
}

local function gui_set_injected_status()
		local inj = query_injected()

		if (inj == 1) then
			update_injected_status(true) -- default
		else
			update_injected_status(false)
		end

end


lole_frame:SetScript("OnUpdate", function()

	if query_injected() == 0 then return end

	if every_4th_frame == 0 then

		local r = get_current_config().general_role;

		if r == "HEALER" then
			if lole_subcommands.get("heal_blast") == 1 then do_combat_stuff() end
		else
			if lole_subcommands.get("blast") == 1 then do_combat_stuff() end
		end

		update_mode_attrib_checkbox_states()

	end

	if every_30th_frame == 0 then
		set_button_states()
		check_durability()

		gui_set_injected_status()

		update_player_pos_text(get_unit_position("player"))
		update_target_pos_text(get_unit_position("target"))

	end

	every_4th_frame = every_4th_frame >= 4 and 0 or (every_4th_frame + 1)
	every_30th_frame = every_30th_frame >= 30 and 0 or (every_30th_frame + 1)

end);

lole_frame:SetScript("OnEvent", function(self, event, prefix, message, channel, sender)
	--DEFAULT_CHAT_FRAME:AddMessage("LOLE_EventHandler: event:" .. event)

	if event == "ADDON_LOADED" then
		if prefix ~= "lole" then return end

		if LOLE_CLASS_CONFIG_NAME_SAVED ~= nil then
			lole_subcommands.setconfig(LOLE_CLASS_CONFIG_NAME_SAVED, LOLE_CLASS_CONFIG_ATTRIBS_SAVED);
		else
			lole_subcommands.setconfig("default");
		end

		update_injected_status(false)

		blast_check_settext(" BLAST off!")
		heal_blast_check_settext( "HEALING off!")

		ClosePetStables()
		-- ^ this one's hooked too; sets lua_registered = 0 in the DLL, to re-register on reloadui

		lole_frame:UnregisterEvent("ADDON_LOADED");


	elseif event == "PLAYER_DEAD" then
		--clear_target();

	elseif event == "PLAYER_REGEN_DISABLED" then
		if IsRaidLeader() then
		--	broadcast_follow_target(NOTARGET); -- REMEMBER TO REMOVE THIS!!
		end
	elseif event == "PLAYER_REGEN_ENABLED" then
		-- if IsRaidLeader() then
		-- 	broadcast_blast_state(0);
		-- end

	elseif event == "UPDATE_BATTLEFIELD_STATUS" then
		-- lol

	elseif event == "PARTY_INVITE_REQUEST" then
	--	if GetNumRaidMembers() > 0 then return end

		local guildies = get_guild_members()

		if guildies[prefix] then
			self:RegisterEvent("PARTY_MEMBERS_CHANGED");
			AcceptGroup()
			StaticPopup_Hide("PARTY_INVITE")
		else
			SendChatMessage("PARTY_INVITE_REQUEST: " .. prefix .. " doesn't appear to be a member of Uuslapio, not auto-accepting!", "GUILD")
		--	DeclineGroup()
		end


	elseif event == "PARTY_MEMBERS_CHANGED" then
		StaticPopup_Hide("PARTY_INVITE")
		self:UnregisterEvent("PARTY_MEMBERS_CHANGED")

		if IsRaidLeader() then
			lole_subcommands.raid()
		end
	elseif event == "RESURRECT_REQUEST" then
		if not UnitAffectingCombat(prefix) then
			lole_frame:RegisterEvent('PLAYER_ALIVE')
			lole_frame:RegisterEvent('PLAYER_UNGHOST', 'PLAYER_ALIVE')
			AcceptResurrect()
		else
			SendChatMessage(prefix .. " resurrected my ass but appears to be in combat. Not auto-accepting.", "GUILD")
		end
	elseif event == "PLAYER_ALIVE" then
		lole_frame:UnregisterEvent('PLAYER_ALIVE')
		lole_frame:UnregisterEvent('PLAYER_UNGHOST', 'PLAYER_ALIVE')

		StaticPopup_Hide("RESURRECT")
		StaticPopup_Hide("RESURRECT_NO_SICKNESS")
		StaticPopup_Hide("RESURRECT_NO_TIMER")

		lole_subcommands.drink()

	elseif event == "CONFIRM_SUMMON" then
		local summoner = GetSummonConfirmSummoner() -- weird ass API..
		local guildies = get_guild_members()

		if guildies[summoner] then
			ConfirmSummon()
		else
			SendChatMessage(summoner .. " attempted to summon my ass to " .. GetSummonConfirmAreaName() .. " but doesn't appear to be a member of Uuslapio, not auto-accepting!", "GUILD")
		end

	elseif event == "TRADE_SHOW" then
		local guildies = get_guild_members();
		if guildies[UnitName("npc")] then -- this is weird as fuck.. but the unit "npc" apparently represents the char that's trading with us
			self:RegisterEvent("TRADE_ACCEPT_UPDATE")
		end

	elseif event == "TRADE_ACCEPT_UPDATE" then
		if message == 1 then
			-- prefix -> our answer, message -> theirs (accept:1, decline:0).
			-- so this is that the trade partner has accepted, and we concur with AcceptTrade()
			L_AcceptTrade()
			self:UnregisterEvent("TRADE_ACCEPT_UPDATE")
		end

	elseif event == "LFG_PROPOSAL_SHOW" then
		execute_script("RunMacroText(\"/click LFDDungeonReadyDialogEnterDungeonButton\")")

elseif event == "LFG_ROLE_CHECK_SHOW" then
		execute_script("RunMacroText(\"/click LFDRoleCheckPopupAcceptButton\")")

	elseif event == "PLAYER_ENTERING_WORLD" then

	elseif event == "LOOT_OPENED" then
		if LOOT_OPENED_REASON then
			if LOOT_OPENED_REASON == "DE_GREENIEZ" then
				local num_items = GetNumLootItems()
				for i = 1, num_items do LootSlot(i) end

			elseif LOOT_OPENED_REASON == "LOOT_BADGE" then
				local num_items = GetNumLootItems()
				for i = 1, num_items do
					local icon, name = GetLootSlotInfo(i)
					if name == "Badge of Justice" then LootSlot(i) end
				end
			end
		end
		lole_frame:UnregisterEvent("LOOT_OPENED")
		LOOT_OPENED_REASON = nil
	end
end
)
