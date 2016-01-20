-- opcodes for DelIgnore :P
LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID, 
LOLE_OPCODE_BLAST, 
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to the target
LOLE_OPCODE_CASTER_FACE,
LOLE_OPCODE_CTM_BROADCAST,
LOLE_OPCODE_COOLDOWNS,
LOLE_OPCODE_CC

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04", "LOP_05", "LOP_06", "LOP_07", "LOP_08"


function send_opcode_addonmsg(opcode, message)
	SendAddonMessage("lole_opcode", opcode .. ":" .. message, "PARTY");
end


local function nop() return end

function broadcast_target_GUID(GUID_str)
	send_opcode_addonmsg(LOLE_OPCODE_TARGET_GUID, cipher_GUID(GUID_str));
end

local function target_unit_with_GUID(GUID_str_ciphered)

	if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] == 1 then return; end

	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
	
	if GUID_deciphered == NOTARGET then
		lole_clear_target()
		return
	end
	
   	if (BLAST_TARGET_GUID ~= GUID_deciphered) then
    	lole_set_target(GUID_deciphered)
		return
    end

end

local function follow_unit_with_GUID(GUID_str_ciphered)
	if IsRaidLeader() then return end
	
	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
	DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. GUID_deciphered);
	
end


local function set_blast(mode)
-- don't ignore even when playermode is on, since it's required for buffs
    DelIgnore(LOLE_OPCODE_BLAST .. ":" .. mode); 
end

local function act_on_CTM_broadcast(args)
	if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] == 0 then
		DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. args);
	end
end

local function caster_range_check(minrange)
	if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] == 0 then
		DelIgnore(LOLE_OPCODE_CASTER_RANGE_CHECK .. ":" .. tostring(minrange));
	end
end

local function caster_face_target()
	if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] == 0 then
		DelIgnore(LOLE_OPCODE_CASTER_FACE);
	end
end

local function stopfollow()
	if LOLE_CLASS_CONFIG.MODE_ATTRIBS["playermode"] == 0 then
		DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. NOTARGET);
	end
end

local function blow_cooldowns()
	lole_subcommands["lole_cooldowns"]()
end


OPCODE_FUNCS = {
	[LOLE_OPCODE_NOP] = nop,
	[LOLE_OPCODE_TARGET_GUID] = target_unit_with_GUID,
	[LOLE_OPCODE_BLAST] = set_blast,
	[LOLE_OPCODE_CASTER_RANGE_CHECK] = caster_range_check,
	[LOLE_OPCODE_FOLLOW] = follow_unit_with_GUID,
	[LOLE_OPCODE_CASTER_FACE] = caster_face_target,
	[LOLE_OPCODE_CTM_BROADCAST] = act_on_CTM_broadcast,
	[LOLE_OPCODE_COOLDOWNS] = blow_cooldowns
}
