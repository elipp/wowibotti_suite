local function auto_stancedance() 
	local name = UnitCastingInfo("target");
	if (name == "Bellowing Roar") then
		echo("Voi vittu, castaa bellowing roarii!!");
		-- CastSpellByName("Berserker Stance");
		-- CastSpellByName("Berserker Rage");
		-- CastSpellByName("Defensive Stance");
	end

end

config_warrior_arms = {}
config_warrior_arms.name = "warrior_arms";

config_warrior_arms.SELF_BUFFS = { "Battle Shout" };
config_warrior_arms.MODE_ATTRIBS = {     
	["playermode"] = 0
}

local sw_frame;
local swing_starttime = 0;
local swing_duration = 0;
	
--sw_frame = CreateFrame("Frame");
--sw_frame:RegisterEvent("COMBAT_LOG_EVENT_UNFILTERED")

local COMBATLOG_FILTER_ME = bit.bor(
			COMBATLOG_OBJECT_AFFILIATION_MINE or 0x00000001,
			COMBATLOG_OBJECT_REACTION_FRIENDLY or 0x00000010,
			COMBATLOG_OBJECT_CONTROL_PLAYER or 0x00000100,
			COMBATLOG_OBJECT_TYPE_PLAYER or 0x00000400
)

	
local function swing_eventhandler(self, event, ...)
	if (event == "COMBAT_LOG_EVENT_UNFILTERED") then
		local timestamp, type, sourceGUID, sourceName, sourceFlags, destGUID, destName, destFlags, spellId, spellName, spellSchool, damageDealt = ...;
		--echo(tostring(timestamp) .. " " .. tostring(type) .. " " .. sourceName .. " " .. tostring(sourceFlags) .. " " .. destName .. tostring(spellId))
		if (type == "SWING_DAMAGE" or type == "SWING_MISSED") and (bit.band(sourceFlags, COMBATLOG_FILTER_ME) == COMBATLOG_FILTER_ME) then
			swing_starttime = GetTime();
			swing_duration = UnitAttackSpeed("player");
		end
		
	end
end

--sw_frame:SetScript("OnEvent", swing_eventhandler)

local function slam()
	
	local dt = GetTime() - swing_starttime;
	local __, __, __, __, __, __, castTime, __, __ = GetSpellInfo("Slam");

	local optimal_slam = swing_duration - (castTime/1000.0);

	echo("dt: " .. tostring(dt) .. ", optimal_slam = " .. tostring(optimal_slam));

	-- this margin is just arbitrary, not tested! also, on feenix the slam mechanic doesn't seem to be working as it should be..
	
	if dt > optimal_slam + 0.2 and dt < optimal_slam + 0.3 then
		if cast_if_nocd("Slam") then return end;
	end
	
end

config_warrior_arms.combat = function()
		
	if not validate_target() then return end
	
	melee_close_in() -- this is a hooked function that makes the warrior walk behind/toward the target.
	-- CTM_MOVE_AND_ATTACK is performed, so no need to mess around with StartAttack()
	
	if UnitIsDead("target") then ClearTarget() end; -- this is probably not good, sometimes follow is lost?
	
	if (not UnitAffectingCombat("player")) then
		RunMacroText("/cast [nostance:1] Battle Stance"); -- charge doesnt work
		RunMacroText("/cast Charge");
		return;
	end


	
	RunMacroText("/cast [nostance:3] Berserker Stance"); -- overall, its probably better to be in zerg stance :D

	
	if UnitCastingInfo("target") then
		RunMacroText("/cast [nostance:3] Berserker Stance");
		cast_if_nocd("Pummel");
	end

	if not has_debuff("target", "Blood Frenzy") then
		RunMacroText("/cast [nostance:1] Battle Stance");
		RunMacroText("/cast Rend");
	end
	
	if cast_if_nocd("Mortal Strike") then return; end
	if cast_if_nocd("Whirlwind") then return; end
	
	--if UnitRage("player") > 60 then
		--if GetTime() - swing_starttime < 1.5 then
		--	if cast_if_nocd("Slam") then return end;
		--end
	--end
	
	if not has_buff("player", "Battle Shout") then
		CastSpellByName("Battle Shout");
		return;
	end
		
	if not has_debuff("target", "Thunder Clap") then
		if cast_if_nocd("Thunder Clap") then return; end
	end

	if not has_debuff("target", "Demoralizing Shout") then
		CastSpellByName("Demoralizing Shout");
		return;
	end

	
end

config_warrior_arms.cooldowns = function() 

end


config_warrior_arms.buffs = function()

end

config_warrior_arms.desired_buffs = function()

    local desired_buffs = {
        "Blessing of Kings",
        "Blessing of Might",
        "Power Word: Fortitude",
        "Divine Spirit",
        "Mark of the Wild",
        "Thorns",
    }

    if get_num_paladins() < 2 then
        table.remove(desired_buffs, 2);
    end

    return desired_buffs;

end

config_warrior_arms.other = function()

end;




