use std::ffi::c_char;

use crate::lua::{get_facing_angle_to_target, playermode, set_facing_force};
use crate::patch::{copy_original_opcodes, InstructionBuffer, Patch, PatchKind};
use crate::{add_repr_and_tryfrom, assembly, Addr};
use crate::{LoleError, LAST_FRAME_NUM, LAST_SPELL_ERR_MSG};

use crate::addrs::offsets;

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug, Copy, Clone)]
    pub enum SpellError {
        YouAreFacingTheWrongWay = 0x02,
        YouHaveNoTarget = 0x07,
        Interrupted = 0x22,
        NotInLineOfSight = 0x29,
        CantDoThatWhileMoving = 0x2D,
        OutOfRange = 0x59,
        AnotherActionIsInProgress = 0x61,
        TargetNeedsToBeInFrontOfYou = 0x7E,
    }
}

extern "stdcall" fn spell_err_msg(msg_id: i32, _msg: *const c_char) {
    if msg_id == 0x0 {
        // GetErrorText has a special handler for msg_id == 0, in which case _msg contains a garbage pointer :D
        // this happens for things like "Looting set to free-for-all.", and other non-spell related info
        return;
    }
    match playermode() {
        Ok(false) => {
            let msg = msg_id.try_into();
            match msg {
                Ok(SpellError::NotInLineOfSight | SpellError::OutOfRange) => {
                    // safety: below unwrap() ok because we matched against Ok()
                    LAST_SPELL_ERR_MSG.set(Some((msg.unwrap(), LAST_FRAME_NUM.get())));
                }
                Ok(
                    SpellError::YouAreFacingTheWrongWay | SpellError::TargetNeedsToBeInFrontOfYou,
                ) => {
                    if let Ok(Some(angle)) = get_facing_angle_to_target() {
                        if let Err(e) = set_facing_force(angle) {
                            println!("spell_err_msg: error: {e:?}");
                        }
                    }
                }
                Ok(_) => {
                    // unimplemented
                }
                Err(_) => {
                    // unknown
                }
            }
        }
        Ok(true) => {}
        Err(e) => println!("(warning: playermode() returned {e:?})"),
    }
    // } else {
    //     if let Ok(msg) = cstr_to_str!(msg) {
    //         println!("(unlisted spell error message: \"{msg}\")");
    //     } else {
    //         println!("(string conversion error: spell_err_msg: {msg_id:x})");
    //     }
    // }
}

pub fn prepare_spell_err_msg_trampoline() -> Patch {
    let original_opcodes = copy_original_opcodes(offsets::wow_cfuncs::SpellErrMsg, 9);
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD);
    patch_opcodes.push(assembly::PUSH_EDI);
    patch_opcodes.push(assembly::PUSH_EAX);
    patch_opcodes.push_call_to(spell_err_msg as Addr);
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push_default_return(offsets::wow_cfuncs::SpellErrMsg, 9);
    Patch {
        name: "SpellErrMsg",
        patch_addr: offsets::wow_cfuncs::SpellErrMsg,
        original_opcodes,
        patch_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
