local main_frame = CreateFrame("main_frame");

local buff_check_frame = CreateFrame("Frame");
buff_check_frame:RegisterEvent("PLAYER_ALIVE");
buff_check_frame:RegisterEvent("PLAYER_ENTERING_WORLD");
buff_check_frame:SetScript("OnEvent", on_buff_check_event);

local msg_frame = CreateFrame("Frame");
msg_frame:RegisterEvent("CHAT_MSG_ADDON");
msg_frame:SetScript("OnEvent", OnMsgEvent);

local lole_frame = CreateFrame("Frame");
lole_frame:RegisterEvent("PLAYER_REGEN_DISABLED"); -- this is fired when player enters combat
lole_frame:RegisterEvent("PLAYER_REGEN_ENABLED"); -- and this when combat is over
lole_frame:SetScript("OnEvent", LOLE_EventHandler);

local button = CreateFrame("Button", "MyButton", main_frame, "UIPanelButtonTemplate")
	button:SetPoint("CENTER", mainframe, "CENTER", 0, 0)
	button:SetWidth(120)
	button:SetHeight(20)
	
	button:SetScript("OnMouseDown", function(self, button)
  if button == "LeftButton" and not self.isMoving then
   self:StartMoving();
   self.isMoving = true;
  end
end)
	
	--button:SetText("test")
	--button:SetNormalFontObject("GameFontNormal")
	
	--local ntex = button:CreateTexture()
	--ntex:SetTexture("Interface/Buttons/UI-Panel-Button-Up")
	--ntex:SetTexCoord(0, 0.625, 0, 0.6875)
	--ntex:SetAllPoints()	
	--button:SetNormalTexture(ntex)
	
	--local htex = button:CreateTexture()
	--htex:SetTexture("Interface/Buttons/UI-Panel-Button-Highlight")
	--htex:SetTexCoord(0, 0.625, 0, 0.6875)
	--htex:SetAllPoints()
	--button:SetHighlightTexture(htex)
	
	--local ptex = button:CreateTexture()
	--ptex:SetTexture("Interface/Buttons/UI-Panel-Button-Down")
	--ptex:SetTexCoord(0, 0.625, 0, 0.6875)
	--ptex:SetAllPoints()
	--button:SetPushedTexture(ptex)
	
button:RegisterForClicks("AnyUp", "AnyDown")

local function btn_onMouseDown() 
	button:SetText(tostring(time()))
end

--button:SetScript("OnMouseDown", btn_onMouseDown)
