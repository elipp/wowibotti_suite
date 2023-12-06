use std::cell::Cell;
use std::collections::VecDeque;
use std::f32::consts::PI;
use std::sync::Mutex;
use std::time::{Duration, Instant};

use crate::addrs::offsets;
use crate::lua::SETFACING_STATE;
use crate::objectmanager::{GUIDFmt, ObjectManager, GUID, NO_TARGET};
use crate::patch::{
    copy_original_opcodes, read_elems_from_addr, write_addr, InstructionBuffer, Patch, PatchKind,
};
use crate::socket::{movement_flags, set_facing};
use crate::vec3::{Vec3, TWO_PI};
use crate::{assembly, Addr, LoleError, LoleResult, Offset};
use crate::{chatframe_print, dostring};

use lazy_static::lazy_static;
use rand::Rng;

lazy_static! {
    // turns out that thread_local! { RefCell }, while possible, is kinda tedious
    // QUEUE.with(|cell| { let mut borrow = cell.borrow_mut(); // etc. })
    static ref QUEUE: Mutex<CtmQueue> = Mutex::new(CtmQueue {
        events: Default::default(),
        current: None,
        prev_pos: Default::default(),
        last_frame_time: Instant::now(),
        yards_moved: Default::default(),
    });
}

#[derive(Clone, Copy)]
struct InterpStatus {
    start_angle: f32,
    end_angle: f32,
    dt: f32,
    t: f32,
}

thread_local! {
    pub static TRYING_TO_FOLLOW: Cell<Option<GUID>> = Cell::new(None);
    pub static CTM_EVENT_ID: Cell<u64> = Cell::new(0);

    static ANGLE_INTERP_STATUS: Cell<Option<InterpStatus>> = Cell::new(None);
}

#[derive(Debug, Default)]
pub struct MovingAverage<const WINDOW_SIZE: usize> {
    data: VecDeque<f32>,
    sum: f32,
}

impl<const W: usize> MovingAverage<W> {
    fn add_sample(&mut self, value: f32) {
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

    pub fn reset(&mut self) {
        self.sum = 0.0;
        self.data.clear();
    }
}

#[derive(Debug)]
struct CtmQueue {
    events: VecDeque<CtmEvent>,
    current: Option<(CtmEvent, Instant)>,
    prev_pos: Vec3,
    last_frame_time: Instant,
    yards_moved: MovingAverage<30>,
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
    let [x, y, z] = read_elems_from_addr::<3, f32>(offsets::ctm::TARGET_POS_X);
    Vec3 { x, y, z }
}

pub fn short_angle_dist(start: f32, end: f32) -> f32 {
    let da = (end - start) % TWO_PI;
    2.0 * da % TWO_PI - da
}

pub fn angle_lerp(start: f32, end: f32, t: f32) -> f32 {
    (start + short_angle_dist(start, end) * t) % TWO_PI
}

const PROBABLY_STUCK_THRESHOLD: f32 = 4.0;
const CTM_COOLDOWN: Duration = Duration::from_millis(150);
const ALMOST_IDENTICAL_TARGET_POS_THRESHOLD: f32 = 0.2;

const INTERP_DT_DEFAULT: f32 = 0.035; // lower means smoother turns :D
const INTERP_NONE: f32 = 1.0;

fn handle_walking_angle_interp(player_r: f32, diff_rot: f32) -> LoleResult<()> {
    if let Some(InterpStatus {
        start_angle,
        end_angle,
        dt,
        t,
    }) = ANGLE_INTERP_STATUS.take()
    {
        if t >= 1.0 {
            set_facing(diff_rot, movement_flags::FORWARD)?;
            ANGLE_INTERP_STATUS.set(None);
        } else {
            if short_angle_dist(player_r, diff_rot).abs() > 0.01 {
                set_facing(
                    angle_lerp(start_angle, end_angle, t),
                    movement_flags::FORWARD,
                )?;
            }
            ANGLE_INTERP_STATUS.set(Some(InterpStatus {
                start_angle,
                end_angle: diff_rot,
                dt,
                t: t + dt,
            }));
        }
    } else {
        if short_angle_dist(player_r, diff_rot).abs() > 0.01 {
            ANGLE_INTERP_STATUS.set(Some(InterpStatus {
                start_angle: player_r,
                end_angle: diff_rot,
                dt: INTERP_DT_DEFAULT,
                t: 0.0,
            }));
        }
    }
    Ok(())
}

impl CtmQueue {
    fn add_to_queue(&mut self, ev: CtmEvent) -> LoleResult<()> {
        if let Some((current, start_time)) = self.current.as_mut() {
            if ev.priority < current.priority {
                return Ok(());
            } else if (current.priority, ev.priority) == (CtmPriority::Path, CtmPriority::Path) {
                let dist = (ev.target_pos - current.target_pos).length();
                if dist > ALMOST_IDENTICAL_TARGET_POS_THRESHOLD {
                    self.events.push_back(ev);
                }
            } else if start_time.elapsed() > CTM_COOLDOWN {
                // ev.priority > current.priority
                self.replace_current(ev)?;
            }
        } else {
            self.replace_current(ev)?;
        }
        Ok(())
    }
    fn replace_current(&mut self, ev: CtmEvent) -> LoleResult<()> {
        match ev.backend {
            CtmBackend::ClickToMove => {
                ev.commit_to_memory()?;
            }
            CtmBackend::Playback => {
                ev.start_walking_towards()?;
            }
        }
        self.events.clear();
        self.current = Some((ev, Instant::now()));
        Ok(())
    }
    fn abort_current(&mut self) -> LoleResult<()> {
        // 00612A53  |.  891D C089D600 MOV DWORD PTR DS:[0D689C0],EBX
        // 00612A59  |.  D91D 9889D600 FSTP DWORD PTR DS:[0D68998]              ; FLOAT 0.0
        // 00612A5F  |.  891D C489D600 MOV DWORD PTR DS:[0D689C4],EBX
        // 00612A65  |.  891D C889D600 MOV DWORD PTR DS:[0D689C8],EBX
        // 00612A6B  |.  891D 9C89D600 MOV DWORD PTR DS:[0D6899C],EBX
        // 00612A71  |.  C705 BC89D600 MOV DWORD PTR DS:[0D689BC],0D

        // No idea what these represent, but ^this is what "ctm_finished" does in case action == 0xD

        // write_addr(0xD689C0, &[0i32; 3])?; // C0 to C8
        // write_addr(0xD68998, &[0i32; 2])?; // 98 and 9C
        // write_addr::<i32>(offsets::ctm::ACTION, &[CtmAction::Done as i32])?;

        let om = ObjectManager::new()?;
        let player = om.get_player()?;
        let target_pos = player.yards_in_front_of(0.1);
        self.replace_current(CtmEvent {
            target_pos,
            priority: CtmPriority::NoOverride,
            action: CtmAction::Move,
            ..Default::default()
        })
    }
    fn poll(&mut self) -> LoleResult<()> {
        let prev_frame_time = self.last_frame_time;
        self.last_frame_time = Instant::now();

        let om = ObjectManager::new()?;
        let player = om.get_player()?;
        let pp = player.get_pos();

        let dt = prev_frame_time.elapsed().as_secs_f32().max(0.001);
        self.yards_moved
            .add_sample((pp - self.prev_pos).length() / dt);

        self.prev_pos = pp;

        if let Some(guid) = TRYING_TO_FOLLOW.get() {
            if let Some(t) = om.get_object_by_guid(guid) {
                let tp = t.get_pos();
                if (pp - tp).length() < 10.0 {
                    TRYING_TO_FOLLOW.set(None);
                    // doing FollowUnit() here causes a ctm_finished, causing a deadlock on the QUEUE mutex.
                    // is avoidable using try_lock();
                    dostring!("MoveForwardStop(); FollowUnit(\"{}\")", t.get_name());
                    self.current = None;
                } else {
                    self.replace_current(CtmEvent {
                        target_pos: tp,
                        priority: CtmPriority::Follow,
                        action: CtmAction::Move,
                        angle_interp_dt: INTERP_NONE,
                        ..Default::default()
                    })?;
                }
            } else {
                println!(
                    "follow: object with guid {} doesn't exist, stopping trying to follow",
                    GUIDFmt(guid)
                );
                TRYING_TO_FOLLOW.set(None);
                self.current = None;
            }
        } else if let Some((current, start_time)) = self.current.as_mut() {
            let diff = current.target_pos - pp;
            let diff_rot = diff.unit().to_rot_value();
            let [_, _, _, r] = player.get_xyzr();

            handle_walking_angle_interp(r, diff_rot)?;

            let yards_moved = self.yards_moved.calculate();
            let probably_stuck = start_time.elapsed() > Duration::from_millis(300)
                && yards_moved < PROBABLY_STUCK_THRESHOLD;

            if probably_stuck || ((pp - current.target_pos).zero_z().length() < 0.9) {
                if probably_stuck {
                    println!(
                        "we're probably stuck, skipping to next CtmEvent ({})",
                        current.id
                    );
                }
                self.advance()?;
            } else if current.priority == CtmPriority::Path {
                if rand::thread_rng().gen::<f32>() < 0.003 {
                    dostring!("JumpOrAscendStart(); AscendStop()")?;
                }
            }
        } else {
            self.advance()?;
        }

        Ok(())
    }
    pub fn advance(&mut self) -> LoleResult<()> {
        let prev_current = self.current.take();
        if let Some(next) = self.events.pop_front() {
            self.yards_moved.reset();
            next.start_walking_towards()?;
            self.current = Some((next, Instant::now()));
        } else {
            if prev_current.is_some() {
                dostring!("MoveForwardStop()")?;
            }
        }
        Ok(())
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
        Path = 1,
        Move = 2,
        Replace = 3,
        Exclusive = 4,
        Follow = 5,
        NoOverride = 6,
        HoldPosition = 7,
        ClearHold = 8,
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
    FaceTarget(f32),
    FollowUnit(String),
    PullMob(GUID),
    SetTargetAndBlast(GUID),
}

#[derive(Debug)]
pub enum CtmBackend {
    ClickToMove,
    Playback,
}

#[derive(Debug)]
pub struct CtmEvent {
    pub id: u64,
    pub target_pos: Vec3,
    pub priority: CtmPriority,
    pub action: CtmAction,
    pub interact_guid: Option<GUID>,
    pub hooks: Option<Vec<CtmPostHook>>,
    pub distance_margin: Option<f32>,
    pub angle_interp_dt: f32,
    pub backend: CtmBackend,
}

impl Default for CtmEvent {
    fn default() -> Self {
        let id = CTM_EVENT_ID.get();
        CTM_EVENT_ID.set(id + 1);
        Self {
            id,
            target_pos: Vec3::default(),
            priority: CtmPriority::None,
            action: CtmAction::Move,
            interact_guid: None,
            hooks: None,
            distance_margin: None,
            angle_interp_dt: INTERP_DT_DEFAULT,
            backend: CtmBackend::Playback,
        }
    }
}

mod constants {
    pub const GLOBAL_CONST1: f32 = 13.9626340866; // this is 9/4 PI ? :D
    pub const MOVE_CONST2: f32 = 0.25;
    pub const MOVE_MINDISTANCE: f32 = 0.5;
    pub const LOOT_CONST2: f32 = 13.4444444;
    pub const LOOT_MINDISTANCE: f32 = 3.6666666;
}

impl CtmEvent {
    fn update_target_pos(&mut self, new_pos: Vec3, distance_margin: Option<f32>) {
        self.target_pos = new_pos;
        self.distance_margin = distance_margin;
    }
    fn commit_to_memory(&self) -> LoleResult<()> {
        let om = ObjectManager::new()?;
        let p = om.get_player()?;
        let ppos = p.get_pos();
        let diff = ppos - self.target_pos;
        let walking_angle = (diff.y.atan2(diff.x) - ppos.y.atan2(ppos.x) - 0.5 * PI) % TWO_PI;

        let (const2, min_distance, _interact_guid) = match self.action {
            CtmAction::Loot => (
                constants::LOOT_CONST2,
                constants::LOOT_MINDISTANCE,
                NO_TARGET, // todo!() actual GUID
            ),
            CtmAction::Move => (
                constants::MOVE_CONST2,
                constants::MOVE_MINDISTANCE,
                NO_TARGET,
            ),
            CtmAction::Done => return Err(LoleError::InvalidParam("CtmKind::Done".to_owned())),
            _ => (
                constants::MOVE_CONST2,
                constants::MOVE_MINDISTANCE,
                NO_TARGET,
            ),
        };
        let min_distance = self.distance_margin.unwrap_or(min_distance);

        write_addr(offsets::ctm::WALKING_ANGLE, &[walking_angle])?;
        write_addr(offsets::ctm::GLOBAL_CONST1, &[constants::GLOBAL_CONST1])?;
        write_addr(offsets::ctm::GLOBAL_CONST2, &[const2])?;
        write_addr(offsets::ctm::MIN_DISTANCE, &[min_distance])?;

        let final_target_point = self.target_pos; // + 2.5 * (self.target_pos - ppos).unit(); // set target "too far", such that `ctm_finished` is not triggered

        write_addr(
            offsets::ctm::TARGET_POS_X,
            &[
                final_target_point.x,
                final_target_point.y,
                final_target_point.z,
            ],
        )?;

        let action: u8 = self.action.into();
        write_addr(offsets::ctm::ACTION, &[action])?;
        SETFACING_STATE.set((walking_angle, std::time::Instant::now()));
        Ok(())
    }
    fn start_walking_towards(&self) -> LoleResult<()> {
        let om = ObjectManager::new()?;
        let p = om.get_player()?;
        let ppos = p.get_pos();
        let [_, _, _, r] = p.get_xyzr();
        let diff = self.target_pos - ppos;
        let walking_angle = diff.unit().to_rot_value();
        handle_walking_angle_interp(r, walking_angle)?;
        dostring!("MoveForwardStart()")?;
        Ok(())
    }
}

pub fn add_to_queue(action: CtmEvent) -> LoleResult<()> {
    let mut ctm = QUEUE.lock()?;
    ctm.add_to_queue(action)?;
    Ok(())
}

pub fn abort_current_event() -> LoleResult<()> {
    let mut ctm = QUEUE.lock()?;
    ctm.abort_current()
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
    // if let Ok(mut ctm_queue) = QUEUE.try_lock() {
    //     if let Some((_current, _)) = ctm_queue.current.take() {
    //         if !ctm_queue.events.is_empty() {
    //             if let Err(_) = dostring!("MoveForwardStart()") {
    //                 println!("dostring(MoveForwardStart()) failed");
    //             }
    //         } else {
    //             if let Err(_) = dostring!("MoveForwardStop()") {
    //                 println!("dostring(MoveForwardStop()) failed");
    //             }
    //         }
    //         // this shit is broken
    //         // if let Some(hooks) = old_current.hooks {
    //         //     let res = match hooks.first() {
    //         //         Some(CtmPostHook::FaceTarget(rot)) => set_facing(*rot),
    //         //         Some(CtmPostHook::FollowUnit(name)) => dostring!("FollowUnit(\"{}\")", name),
    //         //         _ => Ok(()),
    //         //     };
    //         //     if res.is_err() {
    //         //         println!("warning: {res:?}");
    //         //     }
    //         // }
    //     } else {
    //         if let Err(_) = dostring!("MoveForwardStop()") {
    //             println!("dostring(MoveForwardStop()) failed");
    //         }
    //     }
    // } else {
    //     println!("warning: couldn't lock ctm::QUEUE mutex");
    // }
}

pub fn prepare_ctm_finished_patch() -> Patch {
    const CTM_FINISHED_PATCHADDR: Addr = 0x612A53;
    let patch_addr = CTM_FINISHED_PATCHADDR;
    let original_opcodes = copy_original_opcodes(patch_addr, 12);

    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD);
    patch_opcodes.push_call_to(ctm_finished as Addr);
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push(assembly::PUSH_IMM);
    patch_opcodes.push_slice(&(patch_addr + original_opcodes.len() as Offset).to_le_bytes());
    patch_opcodes.push(assembly::RET);

    Patch {
        name: "CTM_finished",
        patch_addr,
        patch_opcodes,
        original_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
