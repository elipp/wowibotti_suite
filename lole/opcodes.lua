-- opcodes for DelIgnore :P
local 

LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID, 
LOLE_OPCODE_BLAST,   -- this is basically deprecated now
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to/towards the target
LOLE_OPCODE_CASTER_FACE,
LOLE_OPCODE_CTM_BROADCAST,
LOLE_OPCODE_COOLDOWNS,
LOLE_OPCODE_CC,
LOLE_OPCODE_DUNGEON_SCRIPT,
LOLE_OPCODE_TARGET_CHARM

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04", "LOP_05", "LOP_06", "LOP_07", "LOP_08", "LOP_09", "LOP_0A"

local LOLE_DEBUG_OPCODE_DUMP = "LOP_81";


function send_opcode_addonmsg(opcode, message)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "RAID");
end

function send_opcode_addonmsg_to(opcode, message, to)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "WHISPER", to)
end

local function nop() return end

function broadcast_target_GUID(GUID_str)
	send_opcode_addonmsg(LOLE_OPCODE_TARGET_GUID, cipher_GUID(GUID_str));
end

function broadcast_follow_target(target_GUID)
	send_opcode_addonmsg(LOLE_OPCODE_FOLLOW, cipher_GUID(target_GUID))
end

function broadcast_blast_state(state)
	send_opcode_addonmsg(LOLE_OPCODE_BLAST, state);
end

local function target_unit_with_GUID(GUID_str_ciphered)

	if lole_subcommands.get("playermode") == 1 then return; end

	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
	
	if GUID_deciphered == NOTARGET then
		clear_target()
		return
	end
	
   	if (BLAST_TARGET_GUID ~= GUID_deciphered) then
    	set_target(GUID_deciphered)
		return
    end

end

local function follow_unit_with_GUID(GUID_str_ciphered)
	--if IsRaidLeader() then return end
	
	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
	DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. GUID_deciphered);
	
end

local function set_blast(mode)
 --  DelIgnore(LOLE_OPCODE_BLAST .. ":" .. mode); 
	if mode == 1 or mode == "1" then 
		set_blast_state(true)
		blast_checkbutton:SetChecked(true)
		blast_check_settext(blast_enabled_string);
	else 
		set_blast_state(nil)
		blast_checkbutton:SetChecked(false)
		blast_check_settext(blast_disabled_string);
		clear_target();
	end
end


function send_CTM_broadcast(mode, arg)
	if mode == CTM_MODES.LOCAL then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. arg);	
				
	elseif mode == CTM_MODES.TARGET then
		local target_GUID = UnitGUID("target")
		if not target_GUID then return end
		
		echo("sending CTM to target " .. target_GUID)
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. target_GUID .. "," .. arg)
	
		-- kinda redundant.
	elseif mode == CTM_MODES.EVERYONE then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)
	
	elseif mode == CTM_MODES.HEALERS then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)
	
	elseif mode == CTM_MODES.CASTERS then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)

	elseif mode == CTM_MODES.MELEE then
		send_opcode_addonmsg(LOLE_OPCODE_CTM_BROADCAST, tostring(mode) .. "," .. "0x0" .. "," .. arg)
	else
		lole_error("lole_ctm: invalid mode: " .. tostring(mode));
		return false;
	end
	
end

function set_target(target_GUID)
	--echo("calling set_target")
  	DelIgnore(LOLE_OPCODE_TARGET_GUID .. ":" .. target_GUID); -- this does a C TargetUnit call :P
    BLAST_TARGET_GUID = target_GUID;
	FocusUnit("target")
	update_target_text(UnitName("target"), UnitGUID("target"));
	
end

function clear_target()
	--echo("calling clear_target!");
	BLAST_TARGET_GUID = NOTARGET;
	ClearFocus();
	ClearTarget();
	update_target_text("-none-", "");

end

local function act_on_CTM_broadcast(args)

	local modestr,GUID,x,y,z = unpack(tokenize_string(args, ","))

	if lole_subcommands.get("playermode") == 1 then
		return;
	end
	
	local coords = x .. "," .. y .. "," .. z
	
	local mode = tonumber(modestr)

	if mode == CTM_MODES.LOCAL then
		if GUID == UnitGUID("player") then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.TARGET then
		if GUID == UnitGUID("player") then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	
	elseif mode == CTM_MODES.EVERYONE then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords)
	
	elseif mode == CTM_MODES.HEALERS then
		if get_current_config().role == ROLES.HEALER then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.CASTERS then
		if get_current_config().role == ROLES.CASTER then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.MELEE then
		if get_current_config().role == ROLES.MELEE then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	else 
		lole_error("act_on_CTM_broadcast: invalid mode " .. modestr);
	end
	
	
end

function caster_range_check(minrange)
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_CASTER_RANGE_CHECK .. ":" .. tostring(minrange));
	end
end

function caster_face_target()
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_CASTER_FACE);
	end
end

local function stopfollow()
	if lole_subcommands.get("playermode") == 0 then
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. NOTARGET);
	end
end

local function blow_cooldowns()
	lole_subcommands.lole_cooldowns()
end

local function set_cc_target(arg)
	-- this assings the CC request into a static var or something like that :D
	local _enabled, _spell, _marker = strsplit(",", arg);
	local enabled, spell, marker = tonumber(_enabled), trim_string(_spell), trim_string(_marker)
	
	if (enabled == 1) then
		set_CC_job(spell, marker)
	elseif (enabled == 0) then
		unset_CC_job(marker)
	else
		lole_error("set_cc_target: invalid enabled argument!");
		return false
	end
	
	return false;
end

function enable_cc_target(name, spell, marker)
	send_opcode_addonmsg_to(LOLE_OPCODE_CC, "1" .. "," .. spell .. "," .. marker, name)
end

function disable_cc_target(name, spell, marker)
	send_opcode_addonmsg_to(LOLE_OPCODE_CC, "0" .. "," .. spell .. "," .. marker, name)
end

function target_unit_with_charm(marker)
	DelIgnore(LOLE_OPCODE_TARGET_CHARM .. ":" .. marker);
end

local function load_dungeon_script(script)

end

lole_opcode_funcs = {
	[LOLE_OPCODE_NOP] = nop,
	[LOLE_OPCODE_TARGET_GUID] = target_unit_with_GUID,
	[LOLE_OPCODE_BLAST] = set_blast,
	[LOLE_OPCODE_CASTER_RANGE_CHECK] = caster_range_check,
	[LOLE_OPCODE_FOLLOW] = follow_unit_with_GUID,
	[LOLE_OPCODE_CASTER_FACE] = caster_face_target,
	[LOLE_OPCODE_CTM_BROADCAST] = act_on_CTM_broadcast,
	[LOLE_OPCODE_COOLDOWNS] = blow_cooldowns,
	[LOLE_OPCODE_CC] = set_cc_target,
	[LOLE_OPCODE_DUNGEON_SCRIPT] = load_dungeon_script
}

function lole_debug_dump_wowobjects()
	DelIgnore(LOLE_DEBUG_OPCODE_DUMP);
	echo("|cFF00FF96Dumped WowObjects to <DESKTOPDIR>\\wodump.log (if you're injected!) ;)")
	return true;
end
