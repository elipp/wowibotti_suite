use std::collections::VecDeque;
use std::f32::consts::PI;

use crate::objectmanager::{ObjectManager, GUID, NO_TARGET};
use crate::patch::{copy_original_opcodes, write_addr, InstructionBuffer, Patch, PatchKind};
use crate::vec3::Vec3;
use crate::{asm, Addr, LoleError, LoleResult, Offset};

#[macro_export]
macro_rules! add_repr_and_tryfrom {
    ($repr_type:ty,
        $(#[$attrs:meta])*
        $vis:vis enum $name:ident {
            $($variant:ident = $value:literal,)*
        }
    ) => {
        $(#[$attrs])*
        #[repr($repr_type)]
        $vis enum $name {
            $($variant = $value,)*
        }
        impl TryFrom<$repr_type> for $name {
            type Error = LoleError;
            fn try_from(value: $repr_type) -> std::result::Result<Self, Self::Error> {
                match value {
                    $($value => Ok($name::$variant),)*
                    v => Err(LoleError::InvalidEnumValue(format!(concat!(stringify!($name), "::try_from({})"), v)))
                }
            }
        }

        impl From<$name> for $repr_type {
            fn from(value: $name) -> Self {
                match value {
                    $($name::$variant => $value,)*
                }
            }
        }
    };
}

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug)]
    pub enum CtmPriority {
        None = 0,
        Low = 1,
        Replace = 2,
        Exclusive = 3,
        Follow = 4,
        NoOverride = 5,
        HoldPosition = 6,
        ClearHold = 7,
    }
}

add_repr_and_tryfrom! {
    u8,
    #[derive(Debug, Clone, Copy)]
    pub enum CtmAction {
        HunterAimedShot = 0x0,
        Face = 0x2,
        Follow = 0x3,
        Move = 0x4,
        TalkNpc = 0x5,
        Loot = 0x6,
        MoveAttack = 0xA,
        Done = 0xD,
    }
}

#[derive(Debug)]
pub struct CtmEvent {
    pub target_pos: Vec3,
    pub priority: CtmPriority,
    pub action: CtmAction,
    pub interact_guid: Option<GUID>,
}

impl CtmEvent {
    fn commit(&self) -> LoleResult<()> {
        let om = ObjectManager::new()?;
        let p = om.get_player()?;
        let ppos = p.get_pos();
        let diff = ppos - self.target_pos;
        let walking_angle = (diff.y.atan2(diff.x) - ppos.y.atan2(ppos.x) - 0.5 * PI) % (2.0 * PI);

        let (const2, min_distance, interact_guid) = match self.action {
            CtmAction::Loot => (
                ctm::constants::LOOT_CONST2,
                ctm::constants::LOOT_MINDISTANCE,
                NO_TARGET, // todo!() actual GUID
            ),
            CtmAction::Move => (
                ctm::constants::MOVE_CONST2,
                ctm::constants::MOVE_MINDISTANCE,
                NO_TARGET,
            ),
            CtmAction::Done => return Err(LoleError::InvalidParam("CtmKind::Done".to_owned())),
            _ => (
                ctm::constants::MOVE_CONST2,
                ctm::constants::MOVE_MINDISTANCE,
                NO_TARGET,
            ),
        };

        write_addr(ctm::addrs::WALKING_ANGLE, &[walking_angle])?;
        write_addr(ctm::addrs::GLOBAL_CONST1, &[ctm::constants::GLOBAL_CONST1])?;
        write_addr(ctm::addrs::GLOBAL_CONST2, &[const2])?;
        write_addr(ctm::addrs::MIN_DISTANCE, &[min_distance])?;

        write_addr(
            ctm::addrs::POS_X,
            &[self.target_pos.x, self.target_pos.y, self.target_pos.z],
        )?;

        let action: u8 = self.action.into();
        write_addr(ctm::addrs::ACTION, &[action])?;
        Ok(())
    }
}

pub fn add_to_queue(action: CtmEvent) -> LoleResult<()> {
    println!("adding {action:?} to ctm_queue");
    action.commit()?;
    Ok(())
}

unsafe extern "C" fn ctm_finished() {
    println!("lulz, CTM was definitely finished")
}

pub fn prepare_ctm_finished_patch() -> Patch {
    let patch_addr = crate::addresses::CTM_finished_patchaddr;
    let original_opcodes = copy_original_opcodes(patch_addr, 12);

    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(asm::PUSHAD);
    patch_opcodes.push_call_to(ctm_finished as Addr);
    patch_opcodes.push(asm::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push(asm::PUSH);
    patch_opcodes.push_slice(&(patch_addr + original_opcodes.len() as Offset).to_le_bytes());
    patch_opcodes.push(asm::RET);

    Patch {
        name: "CTM_finished",
        patch_addr,
        patch_opcodes,
        original_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}

mod ctm {
    pub mod constants {
        pub const GLOBAL_CONST1: f32 = 13.9626340866; // this is 9/4 PI ? :D
        pub const MOVE_CONST2: f32 = 0.25;
        pub const MOVE_MINDISTANCE: f32 = 0.5; // this is 0.5, minimum distance from exact point
        pub const LOOT_CONST2: f32 = 13.4444444;
        pub const LOOT_MINDISTANCE: f32 = 3.6666666;
    }

    pub mod addrs {
        use crate::Addr;
        pub const POS_X: Addr = 0xD68A18;
        pub const POS_Y: Addr = 0xD68A1C;
        pub const POS_Z: Addr = 0xD68A20;
        pub const CONST1: Addr = 0xD68A24;
        pub const ACTION: Addr = 0xD689BC;
        pub const TIMESTAMP: Addr = 0xD689B8;
        pub const INTERACT_GUID: Addr = 0xD689C0;
        pub const MOVE_ATTACK_ZERO: Addr = 0xD689CC;

        pub const WALKING_ANGLE: Addr = 0xD689A0;
        pub const GLOBAL_CONST1: Addr = 0xD689A4;
        pub const GLOBAL_CONST2: Addr = 0xD689A8;
        pub const MIN_DISTANCE: Addr = 0xD689AC;

        pub const INCREMENT: Addr = 0xD689B8;

        pub const MYSTERY_C8: Addr = 0xD689C8;
        pub const MYSTERY_90: Addr = 0xD68A90;
        pub const MYSTERY_94: Addr = 0xD68A94;
    }
}
