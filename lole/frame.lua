lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("ADDON_LOADED");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:RegisterEvent("PLAYER_DEAD");

lole_frame:SetScript("OnUpdate", function()
	if LOLE_BLAST_STATE then
		RunMacroText("/lole");
	end
end);

local function LOLE_EventHandler(self, event, prefix, message, channel, sender) 
	--DEFAULT_CHAT_FRAME:AddMessage("LOLE_EventHandler: event:" .. event)
	
	if event == "ADDON_LOADED" then
		if prefix ~= "lole" then return end

		if LOLE_CLASS_CONFIG_NAME ~= nil then
			lole_subcommands.setconfig(LOLE_CLASS_CONFIG_NAME, LOLE_CLASS_CONFIG_ATTRIBS);
		else
			lole_subcommands.setconfig("default");
		end
		
		lole_frame:UnregisterEvent("ADDON_LOADED");
		
	elseif event == "PLAYER_DEAD" then
		lole_clear_target();
	
	elseif event == "PLAYER_REGEN_DISABLED" then
		if IsRaidLeader() then
			send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, NOTARGET);
		end
	end
	
end


lole_frame:SetScript("OnEvent", LOLE_EventHandler);

lole_frame:SetHeight(300)
lole_frame:SetWidth(230)
lole_frame:SetPoint("RIGHT")
local texture = lole_frame:CreateTexture()
texture:SetAllPoints() 
texture:SetTexture(0.0, 0.0, 0.0, 0.5) 
lole_frame.background = texture 


blast_checkbutton = CreateFrame("CheckButton", "blast_checkbutton", lole_frame, "ChatConfigCheckButtonTemplate");
blast_checkbutton:SetPoint("TOPLEFT", 8, -8);

getglobal(blast_checkbutton:GetName() .. "Text"):SetText("BLAST?")

blast_checkbutton.tooltip = "Whether BLAST is on or off.";

blast_checkbutton:SetScript("OnClick", 
  function()
	LOLE_BLAST_STATE = blast_checkbutton:GetChecked()
	--echo(tostring(LOLE_BLAST_STATE))
  end
);

--local backdrop = {
 --bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",  
  --edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
  -- true to repeat the background texture to fill the frame, false to scale it
  --tile = false,
  -- size (width or height) of the square repeating background tiles (in pixels)
 -- tileSize = 32,
  -- thickness of edge segments and square size of edge corners (in pixels)
 --- edgeSize = 7,
  -- distance from the edges of the frame to those of the background texture (in pixels)
 -- insets = {
  --  left = 50,
  --  right = 50,
   -- top = 2,
  --  bottom = 2
  --}
--}
 

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
config_dropdown:SetPoint("BOTTOMLEFT", 10, 10);
config_dropdown:Show();


local function drop_onClick(name)
	lole_subcommands.setconfig(name);
end
 
local drop_formatted_configs = {}
local drop_formatted_config_indices = {}

local i = 1;

for k, v in pairsByKey(available_configs) do
	drop_formatted_configs[i] = k
	drop_formatted_config_indices[k] = i -- XD
	i = i + 1;
end

function set_visible_dropdown_config(configname)
	UIDropDownMenu_SetSelectedID(config_dropdown, drop_formatted_config_indices[configname])
end


local function drop_initialize()
   local info = {}
   for n = 1, #drop_formatted_configs do
     info.text = drop_formatted_configs[n];
	 info.value = n;
	 info.arg1 = info.text;
	 info.checked = nil
     info.func = drop_onClick;
     UIDropDownMenu_AddButton(info)
   end
 end
 
UIDropDownMenu_Initialize(config_dropdown, drop_initialize)


