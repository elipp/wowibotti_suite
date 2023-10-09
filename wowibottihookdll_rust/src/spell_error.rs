use std::ffi::{c_char, CStr};

use crate::patch::{copy_original_opcodes, InstructionBuffer, Patch, PatchKind};
use crate::{add_repr_and_tryfrom, addrs, asm, Addr};
use crate::{cstr_to_str, LoleError, LAST_FRAME_NUM, LAST_SPELL_ERR_MSG};

add_repr_and_tryfrom! {
    i32,
    #[derive(Copy, Clone)]
    pub enum SpellError {
        YouHaveNoTarget = 0x07,
        Interrupted = 0x22,
        NotInLineOfSight = 0x29,
        CantDoThatWhileMoving = 0x2D,
        OutOfRange = 0x59,
        AnotherActionIsInProgress = 0x61,
        TargetNeedsToBeInFrontOfYou = 0x7E,
    }
}

extern "stdcall" fn spell_err_msg(msg_id: i32, msg: *const c_char) {
    if let Ok(msg) = msg_id.try_into() {
        if matches!(msg, SpellError::NotInLineOfSight | SpellError::OutOfRange) {
            LAST_SPELL_ERR_MSG.set(Some((msg, LAST_FRAME_NUM.get())));
        }
    } else {
        if let Ok(msg) = cstr_to_str!(msg) {
            println!("(unlisted spell error message: {msg})");
        } else {
            println!("(string conversion error: spell_err_msg: {msg_id:x})");
        }
    }
}

pub fn prepare_spell_err_msg_trampoline() -> Patch {
    let original_opcodes = copy_original_opcodes(addrs::wow_cfuncs::SpellErrMsg, 9);
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(asm::PUSHAD);
    patch_opcodes.push(asm::PUSH_EDI);
    patch_opcodes.push(asm::PUSH_EAX);
    patch_opcodes.push_call_to(spell_err_msg as Addr);
    patch_opcodes.push(asm::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push_default_return(addrs::wow_cfuncs::SpellErrMsg, 9);
    Patch {
        name: "SpellErrMsg",
        patch_addr: addrs::wow_cfuncs::SpellErrMsg,
        original_opcodes,
        patch_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
