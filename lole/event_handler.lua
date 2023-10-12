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

event_frame:SetScript("OnEvent", function(self, event, ...)
    events[event](self, ...)
end)

-- Register event functions
for event, _ in pairs(events) do
    event_frame:RegisterEvent(event)
end
