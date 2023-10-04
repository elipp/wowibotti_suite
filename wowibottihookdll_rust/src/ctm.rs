use std::collections::VecDeque;
use std::f32::consts::PI;
use std::sync::{LockResult, Mutex, PoisonError};

use lazy_static::lazy_static;

use crate::lua::chatframe_print;
use crate::objectmanager::{ObjectManager, GUID, NO_TARGET};
use crate::patch::{
    copy_original_opcodes, read_elems_from_addr, write_addr, InstructionBuffer, Patch, PatchKind,
};
use crate::socket::set_facing;
use crate::vec3::Vec3;
use crate::{asm, Addr, LoleError, LoleResult, Offset};

lazy_static! {
    static ref QUEUE: Mutex<CtmQueue> = Mutex::new(CtmQueue {
        events: Default::default(),
        current: None,
        prev_pos: Default::default(),
        last_frame_time: std::time::Instant::now(),
    });
}

#[derive(Debug, Default)]
pub struct MovingAverage<const WINDOW_SIZE: usize> {
    data: VecDeque<f32>,
    sum: f32,
}

impl<const W: usize> MovingAverage<W> {
    fn add_sample(&mut self, value: f32) {
        println!("adding sample {value}, {}", self.data.len());
        self.data.push_back(value);
        self.sum += value;
        if self.data.len() > W {
            if let Some(front) = self.data.pop_front() {
                self.sum -= front;
            }
        }
    }
    fn calculate(&self) -> f32 {
        if self.data.is_empty() {
            return 0.0; // Avoid division by zero
        }
        self.sum / self.data.len() as f32
    }

    pub fn window_full(&self) -> bool {
        self.data.len() == W
    }
}

#[derive(Debug)]
struct CtmQueue {
    events: VecDeque<CtmEvent>,
    current: Option<CtmEvent>,
    prev_pos: Vec3,
    last_frame_time: std::time::Instant,
}

impl Ord for CtmPriority {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        // Compare the fields of your custom type
        (*self as u32).cmp(&(*other as u32))
    }
}

impl PartialOrd for CtmPriority {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

pub fn get_wow_ctm_target_pos() -> Vec3 {
    let [x, y, z] = read_elems_from_addr::<3, f32>(ctm::addrs::TARGET_POS_X);
    Vec3 { x, y, z }
}

const PROBABLY_STUCK_THRESHOLD: f32 = 4.0;

impl CtmQueue {
    fn current_highest_priority(&self) -> CtmPriority {
        self.events
            .iter()
            .map(|ev| ev.priority)
            .max()
            .unwrap_or(CtmPriority::None)
    }
    fn add_to_queue(&mut self, ev: CtmEvent) -> LoleResult<()> {
        if let Some(ref current) = self.current {
            if ev.priority < current.priority {
                return Ok(());
            }
        }
        ev.commit()?;
        self.current = Some(ev);

        Ok(())
    }
    fn poll(&mut self) -> LoleResult<()> {
        // let prev_frame_time = self.last_frame_time;
        self.last_frame_time = std::time::Instant::now();
        if let Some(current) = self.current.as_mut() {
            let om = ObjectManager::new()?;
            let new_pos = om.get_player()?.get_pos();
            // let dt = (self.last_frame_time - prev_frame_time).as_secs_f32();
            // current
            //     .yards_moved
            //     .add_sample((new_pos - self.prev_pos).length() / dt);

            // if current.probably_stuck() {
            //     if let Some(next) = self.events.pop_front() {
            //         next.commit()?;
            //         self.current = Some(next);
            //     } else {
            //         // todo!() write 0D and current coords etc.
            //     }
            // }
            self.prev_pos = new_pos;
        }

        Ok(())
    }
    pub fn advance(&mut self) {
        self.current = self.events.pop_front();
    }
}

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
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
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
pub enum CtmPostHook {
    FaceTarget(Vec3, f32),
    PullMob(GUID),
    SetTargetAndBlast(GUID),
}

#[derive(Debug)]
pub struct CtmEvent {
    pub target_pos: Vec3,
    pub priority: CtmPriority,
    pub action: CtmAction,
    pub interact_guid: Option<GUID>,
    pub hooks: Option<Vec<CtmPostHook>>,
    pub yards_moved: MovingAverage<25>,
}

impl Default for CtmEvent {
    fn default() -> Self {
        Self {
            target_pos: Vec3::default(),
            priority: CtmPriority::None,
            action: CtmAction::Move,
            interact_guid: None,
            hooks: None,
            yards_moved: MovingAverage::default(),
        }
    }
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
            ctm::addrs::TARGET_POS_X,
            &[self.target_pos.x, self.target_pos.y, self.target_pos.z],
        )?;

        let action: u8 = self.action.into();
        write_addr(ctm::addrs::ACTION, &[action])?;
        Ok(())
    }
    pub fn probably_stuck(&self) -> bool {
        let moved = self.yards_moved.calculate();
        println!("moved: {moved}");
        self.yards_moved.window_full() && moved < PROBABLY_STUCK_THRESHOLD
    }
}

pub fn add_to_queue(action: CtmEvent) -> LoleResult<()> {
    let mut ctm = QUEUE.lock()?;
    ctm.add_to_queue(action)?;
    Ok(())
}

pub fn event_in_progress() -> LoleResult<bool> {
    let ctm = QUEUE.lock()?;
    Ok(ctm.current.is_some())
}

pub fn poll() -> LoleResult<()> {
    let mut ctm = QUEUE.lock()?;
    ctm.poll()
}

unsafe extern "C" fn ctm_finished() {
    if let Ok(mut ctm_queue) = QUEUE.lock() {
        if let Some(current) = ctm_queue.current.take() {
            println!("ctm finished! removed {current:?}");
            let wow_internal_target_pos = get_wow_ctm_target_pos();
            if (current.target_pos - wow_internal_target_pos).length() > 2.0 {
                chatframe_print("warning: internal & CtmQueue target coordinates don't match");
            }
            if let Some(hooks) = current.hooks {
                if let Some(CtmPostHook::FaceTarget(pos, rot)) = hooks.iter().next() {
                    set_facing(*pos, *rot).expect("set_facing");
                }
            }
        }
    } else {
        println!("Warning: couldn't lock CTM_QUEUE mutex!")
    }
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
        pub const MOVE_MINDISTANCE: f32 = 0.5;
        pub const LOOT_CONST2: f32 = 13.4444444;
        pub const LOOT_MINDISTANCE: f32 = 3.6666666;
    }

    pub mod addrs {
        use crate::Addr;

        pub const PREV_POS_X: Addr = 0xD68A0C;
        pub const PREV_POS_Y: Addr = 0xD68A10;
        pub const PREV_POS_Z: Addr = 0xD68A14;

        pub const TARGET_POS_X: Addr = 0xD68A18;
        pub const TARGET_POS_Y: Addr = 0xD68A1C;
        pub const TARGET_POS_Z: Addr = 0xD68A20;

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
