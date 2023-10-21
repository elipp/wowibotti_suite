local event_frame = CreateFrame("Frame");
local events = {};

-- Functions
function events:PLAYER_TARGET_CHANGED()
    CURRENT_TARGET_GUID = UnitGUID("target");
end

function events:UNIT_SPELLCAST_SUCCEEDED(self, spell_name)
    LAST_SUCCESSFUL_SPELL.name = spell_name
    LAST_SUCCESSFUL_SPELL.cast_time = GetTime()
end

function events:TRADE_TARGET_ITEM_CHANGED(trade_slot_index)
    -- Auto-accept incoming trades if they are triggered by guildmates
    local trader_playername = UnitName("NPC")
    local trader_guildname = GetGuildInfo(trader_playername)
    if trader_guildname == "Kamat Paskana" then
        AcceptTrade()
    end
end

event_frame:SetScript("OnEvent", function(self, event, ...)
    events[event](self, ...)
end)

-- Register event functions
for event, _ in pairs(events) do
    event_frame:RegisterEvent(event)
end
