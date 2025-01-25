use crate::lua::spell_errmsg_received;
use crate::patch::{copy_original_opcodes, InstructionBuffer, Patch, PatchKind};
use crate::{add_repr_and_tryfrom, assembly, dostring, Addr};
use crate::{LoleError, LAST_FRAME_NUM, LAST_SPELL_ERR_MSG};

use crate::addrs::offsets;
use lole_macros::generate_lua_enum;

#[cfg(feature = "tbc")]
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

#[cfg(feature = "wotlk")]
#[derive(Debug, Copy, Clone, Hash, PartialEq, Eq)]
#[generate_lua_enum(repr = u32, lua_path = "../lole/generated/spell_error.lua")]
pub enum SpellError {
    YouHaveNoTarget = 0xB,
    InvalidTarget = 0xC,
    Interrupted = 0x28,
    NotInLineOfSight = 0x2F,
    CantDoThatWhileMoving = 0x33,
    OutOfAmmo = 0x34,
    CantAttackWhileMounted = 0x40,
    SpellIsNotReadyYet = 0x43,
    NotEnoughMana = 0x55,
    OutOfRange = 0x61,
    TargetTooClose = 0x80,
    TargetNeedsToBeInFrontOfYou = 0x86,

    // the ones with the 0x1000 bitmask originate from the UI_ERROR_MESSAGE event handler
    YouAreFacingTheWrongWay = 0x1000,
    ThereIsNothingToAttack = 0x1001,
    YouAreTooFarAway = 0x1002,
    YouCantDoThatYet = 0x1003,
}

#[repr(C)]
struct SpellErrMsgArgs {
    u0: u32,
    u1: u32,
    msg: u32, // apparently, it's possible to just give this the SpellError type
    u2: u32,
    u3: u32,
    u4: u32,
}

fn is_aligned_to<const A: usize, T>(ptr: *const T) -> bool {
    (ptr as usize) % A == 0
}

unsafe extern "stdcall" fn spell_err_msg(msg_ptr: *const SpellErrMsgArgs) {
    if msg_ptr.is_null() || !is_aligned_to::<4, _>(msg_ptr) {
        return;
    }
    let msg = (*msg_ptr).msg;
    let as_spellerrmsg: Result<SpellError, _> = msg.try_into();
    match as_spellerrmsg {
        Ok(m) => {
            tracing::error!("{m:?} ({msg:X})");
            let _res = spell_errmsg_received(msg);
            // LAST_SPELL_ERR_MSG.with(|l| {
            //     let mut l = l.borrow_mut();
            //     l.insert(m, LAST_FRAME_NUM.get());
            //     match m {
            //         SpellError::TargetNeedsToBeInFrontOfYou => {
            //             // for some reason, checking for playermode() here causes a crash, "Fatal condition"/error #134
            //             dostring!("face_mob()");
            //         }
            //         _ => {}
            //     }
            // });
        }
        e => tracing::warn!("spell_err_msg: {e:?}"),
    }
}

#[cfg(feature = "tbc")]
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

#[cfg(feature = "wotlk")]
pub fn prepare_spell_err_msg_trampoline() -> Patch {
    let original_opcodes = copy_original_opcodes(offsets::wow_cfuncs::SpellErrMsg, 9);
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD); // NOTE: this adds 0x20 to the stack
    patch_opcodes.push_slice(&[0x8D, 0x5C, 0x24, 0x24, 0x53]); // the original SpellErrMsg takes 6 arguments, third one is relevant
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
