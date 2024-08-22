use std::cell::Cell;
use std::f32::consts::PI;
use std::ffi::{c_char, c_void, CStr, CString};

use rand::Rng;

use crate::addrs::offsets::{self, TAINT_CALLER};
use crate::ctm::{self, CtmAction, CtmBackend, CtmEvent, CtmPriority, TRYING_TO_FOLLOW};
use crate::objectmanager::{guid_from_str, GUIDFmt, ObjectManager, WowObject};
use crate::patch::{
    copy_original_opcodes, deref, read_addr, write_addr, InstructionBuffer, Patch, PatchKind,
};
use crate::socket::facing::{self, set_facing};
use crate::socket::movement_flags::NOT_MOVING;
use crate::socket::{cast_gtaoe, movement_flags, read_os_tick_count};
use crate::vec3::{Vec3, TWO_PI};
use crate::{
    add_repr_and_tryfrom, assembly, get_current_character_name, LoleError, LoleResult,
    BROKER_STATE, LAST_SPELL_ERR_MSG, SHOULD_EJECT,
};

#[cfg(feature = "addonmessage_broker")]
use addonmessage_broker::server::{AddonMessage, Msg, MsgSender, MsgWrapper};

use crate::{define_lua_function, Addr}; // POSTGRES_ADDR, POSTGRES_DB, POSTGRES_PASS, POSTGRES_USER};

thread_local! {
    // this is modified from CtmAction::commit() and set_facing
    pub static RUN_SCRIPT_AFTER_N_FRAMES: Cell<Option<(&'static str, usize)>> = Cell::new(None);
}

#[allow(non_camel_case_types)]
pub type lua_State = *mut c_void;

#[allow(non_camel_case_types)]
type lua_CFunction = unsafe extern "C" fn(lua_State) -> i32;

#[allow(non_camel_case_types)]
type lua_Boolean = i32;

const LUA_FALSE: lua_Boolean = 0;
const LUA_TRUE: lua_Boolean = 1;

#[allow(non_camel_case_types)]
type lua_Number = f64;

#[allow(non_camel_case_types)]
type lua_Integer = i32;

const LUA_GLOBALSINDEX: i32 = -10002;
const LUA_NO_RETVALS: i32 = 0;

#[macro_export]
macro_rules! chatframe_print {
    ($fmt:expr, $($args:expr),*) => {
        // this [=[ blah blah blah ]=] syntax is a "level 1 long bracket"
        crate::dostring!(concat!("DEFAULT_CHAT_FRAME:AddMessage([=[[DLL]: ", $fmt, "]=])"), $($args),*)
    };

    ($msg:literal) => {
        crate::dostring!(concat!("DEFAULT_CHAT_FRAME:AddMessage([=[", $msg, "]=])"))
    };
}

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug)]
    pub enum LuaType {
        Nil = 0,
        Boolean = 1,
        UserData = 2,
        Number = 3,
        // Integer, // not official Lua
        String = 4,
        Table = 5,
        Function = 6,
        UserData2 = 7,
        Thread = 8,
        Proto = 9,
        Upval = 10,
    }
}

define_lua_function!(
    lua_gettop,
    (state: lua_State) -> i32
);

// kept here for reference (not allowed by the compiler atm):
// const _lua_gettop: extern "C" fn(lua_State) -> i32 = {
//     let ptr = offsets::lua_gettop as *const ();
//     unsafe { std::mem::transmute(ptr) }
//     // let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute(ptr) };
// };

define_lua_function!(
    lua_settop,
    (state: lua_State, idx: i32) -> ());

#[macro_export]
macro_rules! lua_pop {
    ($state:expr, $idx:expr) => {
        lua_settop($state, -($idx) - 1)
    };
}

define_lua_function!(
    lua_pushcclosure,
    (state: lua_State, closure: lua_CFunction, n: i32) -> ());

define_lua_function!(
    lua_pushnumber,
    (state: lua_State, number: lua_Number) -> ());

define_lua_function!(
    lua_pushboolean,
    (state: lua_State, b: lua_Boolean) -> ());

define_lua_function!(
    lua_gettable,
    (state: lua_State, idx: i32) -> ());

define_lua_function!(
    lua_getfield,
    (state: lua_State, idx: i32, k: *const c_char) -> i32);

define_lua_function!(
    lua_setfield,
    (state: lua_State, idx: i32, k: *const c_char) -> ());

define_lua_function!(
    lua_settable,
    (state: lua_State, idx: i32) -> ());

define_lua_function!(
    lua_rawseti,
    (state: lua_State, idx: i32, n: i32) -> ());

define_lua_function!(
    lua_tointeger,
    (state: lua_State, idx: i32) -> i32);

define_lua_function!(
    lua_gettype,
    (state: lua_State, idx: i32) -> i32);

define_lua_function!(
    lua_tolstring,
    (state: lua_State, idx: i32, len: *mut usize) -> *const c_char);

define_lua_function!(
    lua_next,
    (state: lua_State, idx: i32) -> i32);

define_lua_function!(
    lua_createtable,
    (state: lua_State, narr: i32, nrec: i32) -> ());

macro_rules! lua_tostring {
    ($lua:expr, $idx:literal) => {{
        let mut len: usize = 0;
        // NOTE: lua actually checks for `lua_isstring` before `lua_tostring`, this is pretty good still
        match lua_gettype($lua, $idx).try_into() {
            Ok(LuaType::String) => {
                let s = lua_tolstring($lua, $idx, &mut len);
                if !s.is_null() {
                    cstr_to_str!(s)
                } else {
                    Err(LoleError::NullPtrError)
                }
            }
            Ok(t) => Err(LoleError::LuaUnexpectedTypeError(t, LuaType::String)),
            Err(e) => Err(e)?,
        }
    }};
}

define_lua_function!(
    lua_tonumber,
    (state: lua_State, idx: i32) -> lua_Number);

define_lua_function!(
    lua_pushnil,
    (state: lua_State) -> ());

define_lua_function!(
    lua_pushstring,
    (state: lua_State, str: *const c_char) -> ());

pub fn lua_pushstring_(lua: lua_State, str: &CStr) {
    lua_pushstring(lua, str.as_ptr())
}

define_lua_function!(
    lua_pushlstring,
    (state: lua_State, str: *const c_char, len: usize) -> ());

define_lua_function!(
    lua_pushinteger,
    (state: lua_State, i: lua_Integer) -> ());

define_lua_function!(
    lua_toboolean,
    (state: lua_State, idx: i32) -> i32);

macro_rules! lua_tonumber_f32 {
    ($state:expr, $id:literal) => {
        lua_tonumber($state, $id) as f32
    };
}

define_lua_function!(
    lua_dostring,
    (script: *const c_char, _s: *const c_char, taint: u32) -> ());

#[macro_export]
macro_rules! dostring {
    ($fmt:expr, $($args:expr),*) => {{
        let formatted = format!($fmt, $($args),*);
        crate::dostring!(formatted)
    }};

    ($script:expr) => {{
        use crate::lua::{lua_dostring};
        use std::ffi::{c_char, CString};
        if let Ok(s) = CString::new($script).map_err(|_e|LoleError::StringConvError(format!("{_e:?}"))) {
            lua_dostring(s.as_ptr() as *const c_char, s.as_ptr() as *const c_char, 0);
        }
        else {
            eprintln!("(warning: dostring: CString::new failed)");
        }
    }};

}

pub fn lua_setglobal(lua: lua_State, key: &CStr) {
    lua_setfield_(lua, LUA_GLOBALSINDEX, key)
}

macro_rules! lua_register {
    ($lua:expr, $name:literal, $closure:expr) => {
        lua_pushcclosure($lua, $closure, 0);
        lua_setglobal($lua, $name)
    };
    ($lua:expr, $name:expr, $closure:expr) => {
        lua_pushcclosure($lua, $closure, 0);
        lua_setglobal($lua, $name)
    };
}

macro_rules! lua_unregister {
    ($lua:expr, $name:literal) => {
        lua_pushnil($lua);
        lua_setglobal($lua, $name);
    };

    ($lua:expr, $name:expr) => {
        lua_pushnil($lua);
        lua_setglobal($lua, $name);
    };
}

pub fn lua_getfield_(lua: lua_State, index: i32, name: &CStr) -> i32 {
    lua_getfield(lua, index, name.as_ptr() as *const c_char) //$s.len()) // the last argument is some weird wow internal
}

pub fn lua_setfield_(lua: lua_State, index: i32, name: &CStr) {
    lua_setfield(lua, index, name.as_ptr() as *const c_char)
}

macro_rules! lua_getglobal {
    ($lua:expr, $s:literal) => {
        lua_getfield_($lua, LUA_GLOBALSINDEX, $s)
    };
    ($lua:expr, $s:expr) => {
        lua_getfield_($lua, LUA_GLOBALSINDEX, $s)
    };
}

pub fn register_lop_exec() -> LoleResult<()> {
    write_addr(offsets::lua::vfp_max, &[0xEFFFFFFFu32])?;
    let lua = get_wow_lua_state()?;
    lua_register!(lua, c"lop_exec", lop_exec);
    println!("lop_exec registered!");
    // write_addr(offsets::lua::vfp_max, &[offsets::lua::vfp_original])?;
    Ok(())
}

pub fn unregister_lop_exec() -> LoleResult<()> {
    write_addr(offsets::lua::vfp_max, &[offsets::lua::vfp_original])?; // TODO: probably should look up what the original value was...
    let lua = get_wow_lua_state()?;
    lua_unregister!(lua, c"lop_exec");
    println!("lop_exec un-registered!");
    Ok(())
}

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug)]
    pub enum Opcode {
        Nop = 0,
        TargetGuid = 1,
        CasterRangeCheck = 2,
        Follow = 3,
        StopFollow = 4,
        ClickToMove = 5,
        TargetMarker = 6,
        MeleeBehind = 7,
        GetUnitPosition = 8,
        GetLastSpellErrMsg = 9,
        HealerRangeCheck = 10,
        RefreshHwEventTimestamp = 11,
        StopFollowSpread = 12,
        GetCombatParticipants = 13,
        GetCombatMobs = 14,
        SetTaint = 15,
        LootMob = 16,
        GetAoeFeasibility = 17,
        CastGtAoe = 18,
        FaceMob = 19,
        StorePath = 0x100,
        PlaybackPath = 0x101,
        SendAddonMessage = 0x200,
        Debug = 0x400,
        Dump = 0x401,
        DoString = 0x402,
        EjectDll = 0x403,
        QueryInjected = 0x404,
        DumpWowObject = 0x405,
        RegisterUiErrorMessage = 0x406,
    }
}

#[macro_export]
macro_rules! cstr_to_str {
    ($e:expr) => {{
        use crate::LoleError;
        use std::ffi::CStr;
        if $e.is_null() {
            Err(LoleError::NullPtrError)
        } else {
            unsafe { CStr::from_ptr($e) }
                .to_str()
                .map_err(|e| LoleError::InvalidRawString(format!("{e:?}")))
        }
    }};
}

#[derive(Debug)]
enum PlayerPositionStatus {
    Ok,
    TooNear,
    TooFar,
    WrongFacing,
}

fn get_melee_position_status(
    ideal_position_relative: Vec3,
    tpdiff_unit: Vec3,
    prot_unit: Vec3,
) -> PlayerPositionStatus {
    if ideal_position_relative.length() > 0.5 {
        PlayerPositionStatus::TooFar
    } else if Vec3::dot(tpdiff_unit, prot_unit) < 0.85 {
        PlayerPositionStatus::WrongFacing
    } else {
        PlayerPositionStatus::Ok
    }
}

fn get_caster_position_status(
    range_min: f32,
    range_max: f32,
    tpdiff: Vec3,
    prot_unit: Vec3,
) -> PlayerPositionStatus {
    let distance_from_target = tpdiff.length();
    if distance_from_target >= range_max {
        PlayerPositionStatus::TooFar
    } else if distance_from_target < range_min {
        PlayerPositionStatus::TooNear
    } else if Vec3::dot(tpdiff.unit(), prot_unit) < 0.99 {
        PlayerPositionStatus::WrongFacing
    } else {
        PlayerPositionStatus::Ok
    }
}

pub fn playermode() -> LoleResult<bool> {
    let lua = get_wow_lua_state()?;
    lua_getglobal!(lua, c"LOLE_MODE_ATTRIBS");
    let res = match lua_gettype(lua, -1).try_into() {
        Ok(LuaType::Table) => {
            lua_getfield_(lua, -1, c"playermode");
            match lua_gettype(lua, -1).try_into()? {
                LuaType::Number => {
                    let value = lua_tonumber(lua, -1);
                    Ok(value != 0.0)
                }
                e => Err(LoleError::LuaError(format!("Unexpected type {e:?}"))),
            }
        }
        e => Err(LoleError::LuaError(format!("Unexpected type {e:?}"))),
    };

    lua_pop!(lua, 1);
    res
}

fn write_hwevent_timestamp() -> LoleResult<()> {
    let ticks = read_os_tick_count();
    write_addr(offsets::LAST_HARDWARE_ACTION, &[ticks])
}

fn random_01() -> f32 {
    let mut rng = rand::thread_rng();
    rng.gen()
}

// impl From<postgres::Error> for LoleError {
//     fn from(err: postgres::Error) -> Self {
//         Self::DbError(err)
//     }
// }

// fn get_postgres_conn() -> Result<postgres::Client, postgres::Error> {
//     let conn_str = format!(
//         "postgresql://{}:{}@{}/{}",
//         POSTGRES_USER, POSTGRES_PASS, POSTGRES_ADDR, POSTGRES_DB
//     );
//     postgres::Client::connect(&conn_str, postgres::NoTls)
// }

impl From<serde_json::Error> for LoleError {
    fn from(err: serde_json::Error) -> Self {
        LoleError::SerdeError(err)
    }
}

fn store_path_to_db(name: &str, zonetext: &str, waypoint_data: Vec<Vec3>) -> LoleResult<()> {
    //     let mut client = get_postgres_conn()?;
    //     let waypoint_data = serde_json::to_value(waypoint_data)?;

    //     let path_recording_id: i32 = client
    //         .query_one(
    //             "INSERT INTO path_recordings (name, zonetext, waypoint_data) VALUES ($1, $2, $3) RETURNING id",
    //             &[&name, &zonetext, &waypoint_data],
    //         )?
    //         .get(0);

    //     chatframe_print!(
    //         "Inserted path recording `{}` @ {} to db (id: {})!",
    //         name,
    //         zonetext,
    //         path_recording_id
    //     );

    Ok(())
}

#[derive(Debug)]
struct PathRecording {
    id: i32,
    name: String,
    zonetext: String,
    waypoints: Vec<Vec3>,
}

// // impl From<postgres::Row> for PathRecording {
//     fn from(value: postgres::Row) -> Self {
//         Self {
//             id: value.get(0),
//             name: value.get(1),
//             zonetext: value.get(2),
//             waypoints: serde_json::from_value(value.get(3)).unwrap_or_default(),
//         }
//     }
// }

fn query_path_from_db(name: &str) -> LoleResult<Option<PathRecording>> {
    //     let mut client = get_postgres_conn()?;
    //     if let Some(path_recording) = client.query_opt(
    //         "SELECT id, name, zonetext, waypoint_data FROM path_recordings WHERE name = $1",
    //         &[&name],
    //     )? {
    //         Ok(Some(PathRecording::from(path_recording)))
    //     } else {
    //         Ok(None)
    //     }
    Ok(None)
}

struct TaintReseter(Addr);

impl TaintReseter {
    pub fn new() -> Self {
        let taint_caller = read_addr::<Addr>(TAINT_CALLER);
        write_addr(TAINT_CALLER, &[0x0u32]).unwrap();
        TaintReseter(taint_caller)
    }
}

impl Drop for TaintReseter {
    fn drop(&mut self) {
        write_addr(TAINT_CALLER, &[self.0]).unwrap()
    }
}

pub fn lua_debug_func(lua: lua_State) -> LoleResult<i32> {
    lua_createtable(lua, 0, 0);
    for i in 1i32..=2 {
        lua_createtable(lua, 0, 0);
        // lua_pushstring_(lua, c"0xXDDD");
        // lua_setfield_(lua, -2, c"guid");
        // lua_pushnumber(lua, i as f64 * 0.531);
        // lua_setfield_(lua, -2, c"hp");

        lua_pushstring_(lua, c"guid");
        lua_pushstring_(lua, c"0xXDDD");
        lua_settable(lua, -3);

        lua_pushstring_(lua, c"hp");
        lua_pushnumber(lua, i as f64 * 0.531);
        lua_settable(lua, -3);

        lua_rawseti(lua, -2, i);
    }

    Ok(1)
}

unsafe fn mem_as_slice<'a, T>(addr: Addr, n_bytes: usize) -> &'a [T] {
    // Convert the byte slice to a pointer to u32 and determine its length
    let ptr = addr as *const T;
    let len = (n_bytes as usize) / std::mem::size_of::<T>(); //truncate

    // Create a slice from the pointer and length
    std::slice::from_raw_parts(ptr, len)
}

fn face_target(player: WowObject, target: Option<WowObject>) -> LoleResult<()> {
    if let Some(target) = target {
        let rot = (target.get_pos() - player.get_pos())
            .zero_z()
            .to_rot_value();
        facing::set_facing(player, rot, NOT_MOVING)?;
    }
    Ok(())
}

pub trait PushToTable {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()>;
}

impl PushToTable for &str {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        // another style:
        // lua_pushstring_(lua, c"0xXDDD");
        // lua_setfield_(lua, -2, c"guid");
        // lua_pushnumber(lua, i as f64 * 0.531);
        // lua_setfield_(lua, -2, c"hp");

        let c = CString::new(*self).map_err(|_e| LoleError::StringConvError(format!("{_e:?}")))?;

        lua_pushstring_(lua, key);
        lua_pushstring_(lua, c.as_c_str());
        lua_settable(lua, -3);
        Ok(())
    }
}

impl PushToTable for String {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        self.as_str().push_to_table_with_key(lua, key)
    }
}

impl PushToTable for &CStr {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        lua_pushstring_(lua, key);
        lua_pushstring_(lua, self);
        lua_settable(lua, -3);
        Ok(())
    }
}

impl PushToTable for CString {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        self.as_c_str().push_to_table_with_key(lua, key)
    }
}

impl PushToTable for lua_Integer {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        lua_pushstring_(lua, key);
        lua_pushinteger(lua, *self);
        lua_settable(lua, -3);
        Ok(())
    }
}

impl PushToTable for lua_Number {
    fn push_to_table_with_key(&self, lua: lua_State, key: &CStr) -> LoleResult<()> {
        lua_pushstring_(lua, key);
        lua_pushnumber(lua, *self);
        lua_settable(lua, -3);
        Ok(())
    }
}

fn handle_lop_exec(lua: lua_State) -> LoleResult<i32> {
    let _taint_reseter = TaintReseter::new();

    let nargs = lua_gettop(lua);
    if nargs == 0 {
        println!("lop_exec: no opcode provided");
        return Err(LoleError::MissingOpcode);
    }
    let nargs = nargs - 1;

    // most opcode branches deal with the ObjectManager & player anyway, so just get them here
    let om = ObjectManager::new()?;
    let (player, target) = om.get_player_and_target()?;

    match lua_tointeger(lua, 1).try_into()? {
        Opcode::Nop => {}
        Opcode::TargetGuid if nargs == 1 => {
            let guid_str = lua_tostring!(lua, 2)?;
            let guid = guid_from_str(guid_str)?;
            offsets::CSelectUnit(guid);
        }
        Opcode::Follow if nargs == 1 => {
            let name = lua_tostring!(lua, 2)?;
            if let Some(t) = om.get_unit_by_name(name) {
                let (pp, tp) = (player.get_pos(), t.get_pos());
                let tpdiff = tp - pp;
                let distance = tpdiff.length();
                if distance < 10.0 {
                    dostring!("MoveForwardStop(); FollowUnit('{}')", t.get_name());
                } else {
                    TRYING_TO_FOLLOW.set(Some(t.get_guid()));
                }
            }
        }
        Opcode::StopFollow if nargs == 0 => {
            TRYING_TO_FOLLOW.set(None);
            let target_pos = player.yards_in_front_of(0.1);
            ctm::clear()?;
            ctm::add_to_queue(CtmEvent {
                target_pos,
                priority: CtmPriority::NoOverride,
                action: CtmAction::Move,
                ..Default::default()
            })?;
        }
        Opcode::Dump => {
            for o in om {
                println!("{o}");
            }
        }
        Opcode::CasterRangeCheck if nargs == 2 => {
            if let Some(target) = target {
                let range_min = lua_tonumber_f32!(lua, 2);
                let range_max = lua_tonumber_f32!(lua, 3);
                let (pp, prot) = player.get_pos_and_rotvec();
                let tp = target.get_pos();
                let tpdiff = (tp - pp).zero_z();
                let tpdiff_unit = tpdiff.unit();
                match get_caster_position_status(range_min, range_max, tpdiff, prot) {
                    PlayerPositionStatus::TooNear => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: tp - (range_min + 2.0) * tpdiff_unit,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            ..Default::default()
                        })?;
                    }
                    PlayerPositionStatus::TooFar => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: tp - (range_max - 2.0) * tpdiff_unit,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            ..Default::default()
                        })?;
                    }
                    PlayerPositionStatus::WrongFacing => {
                        facing::set_facing(player, tpdiff_unit.to_rot_value(), NOT_MOVING)?;
                    }

                    PlayerPositionStatus::Ok => {
                        lua_pushboolean(lua, LUA_TRUE);
                        return Ok(1);
                    }
                }
            }
        }

        Opcode::MeleeBehind if nargs == 1 => {
            if let Some(target) = target {
                let (pp, prot) = player.get_pos_and_rotvec();
                let tp = target.get_pos();
                let trot = if let Some(tot) = target.get_target() {
                    // the `rot` value isn't updated very often for Npc:s,
                    // even the wow client itself probably calculates it like this:
                    (tot.get_pos() - tp).unit()
                } else {
                    target.get_rotvec()
                };

                let tpdiff = (tp - pp).zero_z();
                let tpdiff_unit = tpdiff.unit();

                if target.get_target_guid() == Some(player.get_guid()) {
                    facing::set_facing(player, tpdiff_unit.to_rot_value(), NOT_MOVING)?;
                    return Ok(LUA_NO_RETVALS);
                }
                let range_min = lua_tonumber_f32!(lua, 2);
                // TODO: do the if range_min > 2: range_min = range_min - 2 business in Lua?

                const MELEE_NUKE_ANGLE: f32 = 0.35;

                let candidate1 = tp + range_min * trot.rotated_2d_cw((1.0 - MELEE_NUKE_ANGLE) * PI);
                let candidate2 = tp + range_min * trot.rotated_2d_cw((1.0 + MELEE_NUKE_ANGLE) * PI);

                let ideal = pp.select_closer(candidate1, candidate2);
                let ideal_relative = (ideal - pp).zero_z();

                match get_melee_position_status(ideal_relative, tpdiff_unit, prot) {
                    PlayerPositionStatus::TooFar | PlayerPositionStatus::TooNear => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: ideal + 0.5 * prot,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            distance_margin: Some(0.5),
                            ..Default::default()
                        })?;
                    }
                    PlayerPositionStatus::WrongFacing => {
                        facing::set_facing(player, tpdiff.to_rot_value(), NOT_MOVING)?;
                    }
                    PlayerPositionStatus::Ok => {
                        lua_pushboolean(lua, LUA_TRUE);
                        return Ok(1);
                    }
                }
            }
        }

        Opcode::HealerRangeCheck if nargs == 1 => {
            if let Some(target) = target {
                let range_max = lua_tonumber_f32!(lua, 2);
                let pp = player.get_pos();
                let tp = target.get_pos();
                let tpdiff = (tp - pp).zero_z();
                if tpdiff.length() < range_max {
                    return Ok(LUA_NO_RETVALS);
                }
                let tpdiff_unit = tpdiff.unit();

                ctm::add_to_queue(CtmEvent {
                    target_pos: tp - (range_max - 2.0) * tpdiff_unit,
                    priority: CtmPriority::Replace,
                    action: CtmAction::Move,
                    ..Default::default()
                })?;
            }
        }

        Opcode::ClickToMove if nargs == 4 => {
            let x = lua_tonumber_f32!(lua, 2);
            let y = lua_tonumber_f32!(lua, 3);
            let z = lua_tonumber_f32!(lua, 4);
            let prio = lua_tointeger(lua, 5);
            let action = CtmEvent {
                target_pos: Vec3 { x, y, z },
                priority: prio.try_into()?,
                action: CtmAction::Move,
                ..Default::default()
            };
            ctm::add_to_queue(action)?;
        }
        Opcode::DoString if nargs == 1 => {
            let script = lua_tostring!(lua, 2)?;
            dostring!(script);
        }
        Opcode::GetUnitPosition if nargs == 1 => {
            let arg = lua_tostring!(lua, 2)?.trim();
            let object = if arg.starts_with("0x") {
                let guid = guid_from_str(arg)?;
                om.get_object_by_guid(guid)
            } else if arg == "player" {
                Some(player)
            } else if arg == "target" {
                target
            } else if arg == "focus" {
                let focus_guid = om.get_focus_guid();
                om.get_object_by_guid(focus_guid)
            } else {
                om.get_unit_by_name(arg)
            };

            if let Some(object) = object {
                for elem in object.get_xyzr().into_iter() {
                    lua_pushnumber(lua, elem as lua_Number);
                }
                return Ok(4);
            }
        }
        Opcode::GetLastSpellErrMsg if nargs == 0 => {
            if let Some((msg, frame_num)) = LAST_SPELL_ERR_MSG.get() {
                lua_pushnumber(lua, (msg as i32).into());
                lua_pushnumber(lua, frame_num.into());
                return Ok(2);
            } else {
                return Ok(LUA_NO_RETVALS);
            }
        }
        Opcode::RefreshHwEventTimestamp if nargs == 0 => {
            write_hwevent_timestamp()?;
        }
        Opcode::StopFollowSpread if nargs == 0 => {
            TRYING_TO_FOLLOW.set(None);
            // add randomness (for multibox masking)
            let target_pos =
                player.get_pos() + random_01() * Vec3::from_rot_value(random_01() * TWO_PI);
            ctm::add_to_queue(CtmEvent {
                target_pos,
                priority: CtmPriority::NoOverride,
                action: CtmAction::Move,
                ..Default::default()
            })?;
        }
        Opcode::GetCombatParticipants => {
            let mut num = 0i32;
            for c in om
                .iter_units_and_npcs()
                .filter(|o| o.in_combat().unwrap_or(false))
            {
                let guid = CString::new(format!("{}", GUIDFmt(c.get_guid())))?;
                lua_pushstring(lua, guid.as_c_str().as_ptr());
                num += 1;
            }
            return Ok(num);
        }
        Opcode::GetAoeFeasibility if nargs == 2 => {
            let relative_to = lua_tostring!(lua, 2)?;
            let radius = lua_tonumber_f32!(lua, 3);
            let mut feasibility = 0.0;
            let query_pos = match &relative_to[..] {
                "target" => {
                    if let Some(target) = target {
                        target.get_pos()
                    } else {
                        return Ok(LUA_NO_RETVALS);
                    }
                }
                "player" => player.get_pos(),
                other => {
                    return Err(LoleError::LuaError(format!(
                        "Unimplemented AoeFeasibility target {other}"
                    )));
                }
            };

            for mob in om.iter_mobs().filter(|o| {
                o.unit_reaction(&player) < 5
                    && o.is_dead().is_ok_and(|dead| !dead)
                    && o.health().is_ok_and(|health| health > 0)
            }) {
                let dist = (query_pos - mob.get_pos()).length();
                feasibility += (-(1.0 / radius.powi(3)) * dist.powi(3) + 1.0).clamp(0.0, 1.0);
            }
            lua_pushnumber(lua, feasibility as lua_Number);
            return Ok(1);
        }
        Opcode::GetCombatMobs => {
            let mut num = 1i32;
            lua_createtable(lua, 0, 0);

            for c in om.iter_mobs().filter(|o| {
                o.in_combat().is_ok_and(|combat| combat) && o.is_dead().is_ok_and(|dead| !dead)
            }) {
                let guid = CString::new(format!("{}", GUIDFmt(c.get_guid())))?;
                lua_createtable(lua, 0, 0);

                guid.push_to_table_with_key(lua, c"guid")?;
                (c.health()? as lua_Integer).push_to_table_with_key(lua, c"hp")?;
                lua_rawseti(lua, -2, num);

                num += 1;
            }
            return Ok(1);
        }

        Opcode::LootMob => {
            let ppos = player.get_pos();
            let mut lootable: Vec<_> = om
                .iter()
                .filter(|o| {
                    o.npc_has_loot_table().unwrap_or(false) && (o.get_pos() - ppos).length() < 30.0
                })
                .collect();
            lootable.sort_unstable_by_key(|l| ((l.get_pos() - ppos).length() * 1024.0) as u32);
            if let Some(closest) = lootable.iter().next() {
                if (closest.get_pos() - ppos).length() > ctm::constants::LOOT_MINDISTANCE {
                    let action = CtmEvent {
                        target_pos: closest.get_pos(),
                        priority: CtmPriority::Replace,
                        action: CtmAction::Loot,
                        interact_guid: Some(closest.get_guid()),
                        backend: CtmBackend::ClickToMove,
                        ..Default::default()
                    };
                    ctm::add_to_queue(action)?;
                } else {
                    offsets::CSelectUnit(closest.get_guid());
                    dostring!("InteractUnit('target')");
                }
            }
        }

        Opcode::CastGtAoe if nargs == 4 => {
            let spellid = lua_tointeger(lua, 2);
            let pos = Vec3 {
                x: lua_tonumber_f32!(lua, 3),
                y: lua_tonumber_f32!(lua, 4),
                z: lua_tonumber_f32!(lua, 5),
            };
            cast_gtaoe(spellid, pos)?;
            return Ok(LUA_NO_RETVALS);
        }

        Opcode::FaceMob if nargs == 0 => {
            face_target(player, target)?;
            return Ok(LUA_NO_RETVALS);
        }

        Opcode::StorePath if nargs == 3 => {
            let mut path = Vec::new();
            let name = lua_tostring!(lua, 2)?;
            let zonetext = lua_tostring!(lua, 3)?;
            if let Ok(LuaType::Table) = lua_gettype(lua, 4).try_into() {
                lua_pushnil(lua); // starts the outer table iteration
                while lua_next(lua, 4) != 0 {
                    if let Ok(LuaType::Table) = lua_gettype(lua, -1).try_into() {
                        lua_getfield_(lua, -1, c"x");
                        lua_getfield_(lua, -2, c"y");
                        lua_getfield_(lua, -3, c"z");

                        let x = lua_tonumber(lua, -3) as f32;
                        let y = lua_tonumber(lua, -2) as f32;
                        let z = lua_tonumber(lua, -1) as f32;
                        // Pop the fields, but keep the inner table for the next iteration
                        lua_pop!(lua, 3);

                        path.push(Vec3 { x, y, z });
                    }
                    lua_pop!(lua, 1);
                }
            }
            store_path_to_db(name, zonetext, path)?;
        }
        Opcode::PlaybackPath if 1 <= nargs && nargs <= 2 => {
            let name = lua_tostring!(lua, 2)?;
            let reversed = if nargs == 2 {
                lua_toboolean(lua, 3) == 1
            } else {
                false
            };
            if let Some(mut path_recording) = query_path_from_db(&name)? {
                chatframe_print!("Starting playback for path `{}`", name);
                if reversed {
                    path_recording.waypoints.reverse();
                }
                for point in path_recording.waypoints {
                    ctm::add_to_queue(CtmEvent {
                        target_pos: point,
                        priority: CtmPriority::Path,
                        action: CtmAction::Move,
                        distance_margin: Some(0.1),
                        ..Default::default()
                    })?;
                }
            } else {
                chatframe_print!("Path with name '{}' not found in db", name);
            }
        }
        Opcode::Debug => {
            return lua_debug_func(lua);
        }
        Opcode::EjectDll => {
            SHOULD_EJECT.set(true);
        }
        Opcode::QueryInjected => {
            lua_pushboolean(lua, 1);
            return Ok(1);
        }
        #[cfg(feature = "addonmessage_broker")]
        Opcode::SendAddonMessage if nargs >= 2 && nargs < 5 => {
            if let Some(state) = BROKER_STATE.get() {
                let prefix = lua_tostring!(lua, 2)?.to_owned();
                let text = lua_tostring!(lua, 3)?.to_owned();
                let r#type = if nargs > 2 {
                    Some(lua_tostring!(lua, 4)?.to_owned())
                } else {
                    None
                };
                let target = if nargs > 3 {
                    Some(lua_tostring!(lua, 5)?.to_owned())
                } else {
                    None
                };

                match (r#type.as_deref(), target.as_deref()) {
                    (Some("WHISPER"), None) => {
                        eprintln!("(warning: Invalid use of SendAddonMessage(..., WHISPER) with no target)");
                        return Ok(0);
                    }
                    _ => {}
                }

                let _res = state.tx.send(MsgWrapper {
                    from: MsgSender::Peer(state.connection_id, state.character_name.clone()),
                    message: Msg::AddonMessage(AddonMessage {
                        prefix,
                        text,
                        r#type,
                        target,
                        from: state.character_name.clone(),
                    }),
                });
            } else {
                eprintln!("(warning: failed to send AddonMessage: Connection not initialized)");
            }
        }
        Opcode::DumpWowObject => {
            // when mob is looted, base + 0x2A*4 is set to 0, meanwhile "NpcState" seems to not be very interesting
            let n_bytes = if nargs >= 1 {
                lua_tointeger(lua, 2)
            } else {
                1024
            };
            if let (_, Some(o)) = om.get_player_and_target()? {
                chatframe_print!("{}, {}, {}", o, o.base, deref::<u32, 1>(o.base + 0x2a * 4));
                // let slice: &[u8] = unsafe { mem_as_slice(base, n_bytes as usize) };
                // println!(
                //     "dumping {}*{} bytes from wowobject@{:#010X}",
                //     slice.len(),
                //     std::mem::size_of::<u8>(),
                //     base
                // );
                // for (i, u) in slice.iter().enumerate() {
                //     print!("{:02X} ", u);
                //     if (i + 1) % 4 == 0 {
                //         println!("");
                //     }
                // }
            }
        }
        Opcode::RegisterUiErrorMessage if nargs == 1 => {
            // let msg = lua_tostring!(lua, 2)?;
            // match msg {
            //     "You are facing the wrong way!" => {
            //         face_target(player, target)?;
            //     }
            //     "You are too far away!" => {}
            //     _ => {}
            // }
            return Ok(LUA_NO_RETVALS);
        }
        v => return Err(LoleError::InvalidOrUnimplementedOpcodeCallNargs(v, nargs)),
    }
    Ok(LUA_NO_RETVALS)
}

#[no_mangle]
pub unsafe extern "C" fn lop_exec(lua: lua_State) -> i32 {
    match handle_lop_exec(lua) {
        Ok(num_retvals) => num_retvals,
        Err(error) => {
            println!("lop_exec: error: {:?}", error);
            let x = chatframe_print!("lop_exec: error: {:?}", error);
            LUA_NO_RETVALS
        }
    }
}

pub fn get_wow_lua_state() -> LoleResult<lua_State> {
    let res = deref::<lua_State, 1>(offsets::lua::wow_lua_State);
    if !res.is_null() {
        Ok(res)
    } else {
        Err(LoleError::LuaStateIsNull)
    }
}

pub fn register_lop_exec_if_not_registered() -> LoleResult<()> {
    if let Err(e) = ObjectManager::new() {
        // we're not in the world
        return Err(e);
    }

    let lua = get_wow_lua_state()?;
    lua_getglobal!(lua, c"lop_exec");
    let var_type = lua_gettype(lua, -1);
    lua_pop!(lua, 1);
    match var_type.try_into() {
        Ok(LuaType::Function) => {}
        _ => register_lop_exec()?,
    };
    Ok(())
}

#[cfg(feature = "tbc")]
pub fn prepare_lua_prot_patch() -> Patch {
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push_slice(&[
        0xB8,
        0x01,
        0x00,
        0x00,
        0x00,
        0x5D,
        0xC3,
        assembly::NOP,
        assembly::NOP,
    ]);

    Patch {
        name: "Lua_Prot",
        patch_addr: offsets::LUA_PROT_PATCHADDR,
        original_opcodes: copy_original_opcodes(offsets::LUA_PROT_PATCHADDR, 9),
        patch_opcodes,
        kind: PatchKind::OverWrite,
    }
}

extern "stdcall" fn closepetstables_lop_exec(lua: lua_State) -> i32 {
    match handle_lop_exec(lua) {
        Ok(res) => res,
        Err(e) => {
            println!("lop_exec error: {:?}", e);
            0
        }
    }
}

#[cfg(feature = "wotlk")]
#[allow(non_snake_case)]
pub fn prepare_ClosePetStables_patch() -> Patch {
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSH_ESI);
    patch_opcodes.push_call_to(closepetstables_lop_exec as Addr);
    patch_opcodes.push(assembly::RET); // ret val is in eax
    Patch {
        name: "ClosePetStables__lop_exec",
        patch_addr: offsets::lua_funcs::ClosePetStables,
        original_opcodes: copy_original_opcodes(offsets::lua_funcs::ClosePetStables, 15),
        patch_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
