use std::collections::VecDeque;
use std::f32::consts::PI;

use crate::objectmanager::{ObjectManager, GUID, NO_TARGET};
use crate::patch::write_addr;
use crate::vec3::Vec3;
use crate::{LoleError, LoleResult};

#[derive(Debug)]
pub enum CtmPriority {
    None,
    Low,
    Replace,
    Exclusive,
    Follow,
    NoOverride,
    HoldPosition,
    ClearHold,
    Unknown(i32),
}

impl From<i32> for CtmPriority {
    fn from(v: i32) -> Self {
        match v {
            0 => CtmPriority::None,
            1 => CtmPriority::Low,
            2 => CtmPriority::Replace,
            3 => CtmPriority::Exclusive,
            4 => CtmPriority::Follow,
            5 => CtmPriority::NoOverride,
            6 => CtmPriority::HoldPosition,
            7 => CtmPriority::ClearHold,
            v => CtmPriority::Unknown(v),
        }
    }
}

#[derive(Debug, Clone, Copy)]
enum CtmKind {
    HunterAimedShot,
    Face(GUID),
    Follow(GUID),
    Move,
    TalkNpc(GUID),
    Loot(GUID),
    MoveAttack,
    Done,
}

impl From<CtmKind> for u8 {
    fn from(value: CtmKind) -> Self {
        match value {
            CtmKind::HunterAimedShot => 0x0,
            CtmKind::Face(_) => 0x2,
            CtmKind::Follow(_) => 0x3,
            CtmKind::Move => 0x4,
            CtmKind::TalkNpc(_) => 0x5,
            CtmKind::Loot(_) => 0x6,
            CtmKind::MoveAttack => 0xA,
            CtmKind::Done => 0xD,
        }
    }
}

struct CtmAction {
    target: Vec3,
    priority: CtmPriority,
    kind: CtmKind,
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

impl CtmAction {
    fn commit(&self) -> LoleResult<()> {
        let om = ObjectManager::new()?;
        let p = om.get_player()?;
        let ppos = p.get_pos();
        let diff = ppos - self.target;
        let walking_angle = (diff.y.atan2(diff.x) - ppos.y.atan2(ppos.x) - 0.5 * PI) % (2.0 * PI);

        let (const2, min_distance, interact_guid) = match self.kind {
            CtmKind::Loot(guid) => (
                ctm::constants::LOOT_CONST2,
                ctm::constants::LOOT_MINDISTANCE,
                guid,
            ),
            CtmKind::Move => (
                ctm::constants::MOVE_CONST2,
                ctm::constants::MOVE_MINDISTANCE,
                NO_TARGET,
            ),
            CtmKind::Done => return Err(LoleError::InvalidParam),
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
            &[self.target.x, self.target.y, self.target.z],
        )?;

        let action: u8 = self.kind.into();
        write_addr(ctm::addrs::ACTION, &[action])?;
        Ok(())
    }
}
