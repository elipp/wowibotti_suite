-- opcodes for DelIgnore :P
LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID, 
LOLE_OPCODE_BLAST,   -- this is basically deprecated now
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to the target
LOLE_OPCODE_CASTER_FACE,
LOLE_OPCODE_CTM_BROADCAST,
LOLE_OPCODE_COOLDOWNS,
LOLE_OPCODE_CC

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04", "LOP_05", "LOP_06", "LOP_07", "LOP_08"

LOLE_DEBUG_OPCODE_DUMP = "LOP_81";


function send_opcode_addonmsg(opcode, message)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "PARTY");
end


local function nop() return end

function broadcast_target_GUID(GUID_str)
	send_opcode_addonmsg(LOLE_OPCODE_TARGET_GUID, cipher_GUID(GUID_str));
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
		LOLE_BLAST_STATE = true;
		blast_checkbutton:SetChecked(true)
		blast_check_settext(blast_enabled_string);
	else 
		LOLE_BLAST_STATE = nil;
		blast_checkbutton:SetChecked(false)
		blast_check_settext(blast_disabled_string);
		clear_target();
	end
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
		if LOLE_CLASS_CONFIG.role == ROLES.HEALER then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.CASTERS then
		if LOLE_CLASS_CONFIG.role == ROLES.CASTER then
			DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. coords);
		end
	elseif mode == CTM_MODES.MELEE then
		if LOLE_CLASS_CONFIG.role == ROLES.MELEE then
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
	lole_subcommands["lole_cooldowns"]()
end


lole_opcode_funcs = {
	[LOLE_OPCODE_NOP] = nop,
	[LOLE_OPCODE_TARGET_GUID] = target_unit_with_GUID,
	[LOLE_OPCODE_BLAST] = set_blast,
	[LOLE_OPCODE_CASTER_RANGE_CHECK] = caster_range_check,
	[LOLE_OPCODE_FOLLOW] = follow_unit_with_GUID,
	[LOLE_OPCODE_CASTER_FACE] = caster_face_target,
	[LOLE_OPCODE_CTM_BROADCAST] = act_on_CTM_broadcast,
	[LOLE_OPCODE_COOLDOWNS] = blow_cooldowns
}

