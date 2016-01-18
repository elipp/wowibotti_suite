-- opcodes for DelIgnore :P
LOLE_OPCODE_NOP,
LOLE_OPCODE_TARGET_GUID, 
LOLE_OPCODE_BLAST, 
LOLE_OPCODE_CASTER_RANGE_CHECK,
LOLE_OPCODE_FOLLOW,  -- this also includes walking to the target
LOLE_OPCODE_CASTER_FACE,
LOLE_OPCODE_CTM_BROADCAST

= "LOP_00", "LOP_01", "LOP_02", "LOP_03", "LOP_04", "LOP_05", "LOP_06";


function broadcast_target_GUID(GUID_str)
	local ciphered = cipher_GUID(GUID_str);
	SendAddonMessage("lole_target", tostring(ciphered), "PARTY");
end

function target_unit_with_GUID(GUID_str_ciphered)

	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
   	if (BLAST_TARGET_GUID ~= GUID_deciphered) then
    	DelIgnore(LOLE_OPCODE_TARGET_GUID .. ":" .. GUID_deciphered); 
    	BLAST_TARGET_GUID = GUID_deciphered;
		FocusUnit("target")
    end

end

function follow_unit_with_GUID(GUID_str_ciphered)
	local GUID_deciphered = decipher_GUID(GUID_str_ciphered);
    DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. GUID_deciphered);
end

function set_blast(mode)
    DelIgnore(LOLE_OPCODE_BLAST .. ":" .. mode); 
end

function act_on_CTM_broadcast(args)
    DelIgnore(LOLE_OPCODE_CTM_BROADCAST .. ":" .. args);
end

function caster_range_check(minrange)
	DelIgnore(LOLE_OPCODE_CASTER_RANGE_CHECK .. ":" .. tostring(minrange));
end

function caster_face_target()
	DelIgnore(LOLE_OPCODE_CASTER_FACE);
end

function stopfollow()
	DelIgnore(LOLE_OPCODE_FOLLOW .. ":" .. "0x0000000000000000");
end

function melee_close_in()
	ClosePetStables(); -- hooked XD
end
