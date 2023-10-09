use std::f32::consts::PI;
use std::ffi::{c_char, c_void, CStr, CString};

use crate::addrs::SelectUnit;
use crate::ctm::{self, CtmAction, CtmEvent, CtmPriority, TRYING_TO_FOLLOW};
use crate::define_wow_cfunc;
use crate::objectmanager::{guid_from_str, ObjectManager};
use crate::patch::{copy_original_opcodes, deref, write_addr, InstructionBuffer, Patch, PatchKind};
use crate::socket::set_facing;
use crate::vec3::Vec3;
use crate::{
    add_repr_and_tryfrom, asm, LoleError, LoleResult, LAST_FRAME_NUM, LAST_SPELL_ERR_MSG,
    SHOULD_EJECT,
};

#[allow(non_camel_case_types)]
pub type lua_State = *mut c_void;

#[allow(non_camel_case_types)]
type lua_CFunction = unsafe extern "C" fn(lua_State) -> i32;

#[allow(non_camel_case_types)]
type lua_Number = f64;

const LUA_GLOBALSINDEX: i32 = -10002;
const NO_RETVALS: i32 = 0;

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug)]
    enum LuaType {
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

define_wow_cfunc!(
    lua_gettop,
    (state: lua_State) -> i32
);

// const _lua_gettop: extern "C" fn(lua_State) -> i32 = {
//     let ptr = addrs::lua_gettop as *const ();
//     unsafe { std::mem::transmute(ptr) }
//     // let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute(ptr) };
// };

define_wow_cfunc!(
    lua_settop,
    (state: lua_State, idx: i32) -> ());

define_wow_cfunc!(
    lua_pushcclosure,
    (state: lua_State, closure: lua_CFunction, n: i32) -> ());

define_wow_cfunc!(
    lua_pushnumber,
    (state: lua_State, number: lua_Number) -> ());

define_wow_cfunc!(
    lua_getfield,
    (state: lua_State, idx: i32, k: *const c_char) -> i32);

define_wow_cfunc!(
    lua_setfield,
    (state: lua_State, idx: i32, k: *const c_char) -> ());

define_wow_cfunc!(
    lua_tointeger,
    (state: lua_State, idx: i32) -> i32);

define_wow_cfunc!(
    lua_gettype,
    (state: lua_State, idx: i32) -> i32);

define_wow_cfunc!(
    lua_tolstring,
    (state: lua_State, idx: i32, len: *mut usize) -> *const c_char);

macro_rules! lua_tostring {
    ($lua:expr, $idx:literal) => {{
        let mut len: usize = 0;
        let s = lua_tolstring($lua, $idx, &mut len);
        if !s.is_null() {
            cstr_to_str!(s)
        } else {
            Err(LoleError::NullPointerError)
        }
    }};
}

define_wow_cfunc!(
    lua_tonumber,
    (state: lua_State, idx: i32) -> lua_Number);

define_wow_cfunc!(
    lua_pushnil,
    (state: lua_State) -> ());

define_wow_cfunc!(
    lua_pushstring,
    (state: lua_State, str: *const c_char) -> ());

define_wow_cfunc!(
    lua_pushlstring,
    (state: lua_State, str: *const c_char, len: usize) -> ());

macro_rules! lua_tonumber_f32 {
    ($state:expr, $id:literal) => {
        lua_tonumber($state, $id) as f32
    };
}

define_wow_cfunc!(
    lua_dostring,
    (script: *const c_char, _s: *const c_char, taint: u32) -> ());

pub fn dostring(script: &str) -> LoleResult<()> {
    println!("lua_dostring: {script}");
    let c_str = CString::new(script)?;
    Ok(lua_dostring(c_str.as_ptr(), c_str.as_ptr(), 0))
}

macro_rules! lua_setglobal {
    ($lua:expr, $key:expr) => {
        lua_setfield($lua, LUA_GLOBALSINDEX, $key);
    };
}

macro_rules! lua_register {
    ($lua:expr, $name:expr, $closure:expr) => {
        lua_pushcclosure($lua, $closure, 0);
        lua_setglobal!($lua, $name as *const c_char);
    };
}

macro_rules! lua_unregister {
    ($lua:expr, $name:expr) => {
        lua_pushnil($lua);
        lua_setglobal!($lua, $name as *const c_char);
    };
}

macro_rules! lua_getglobal {
    ($lua:expr, $s:expr) => {
        lua_getfield($lua, LUA_GLOBALSINDEX, $s as *const c_char)
    };
}

pub fn register_lop_exec() -> LoleResult<()> {
    write_addr(addrs::vfp_max, &[0xEFFFFFFFu32])?;
    let lua = get_lua_state()?;
    lua_register!(lua, b"lop_exec\0".as_ptr(), lop_exec);
    println!("lop_exec registered!");
    Ok(())
}

pub fn unregister_lop_exec() -> LoleResult<()> {
    write_addr(addrs::vfp_max, &[0xEFFFFFFFu32])?;
    let lua = get_lua_state()?;
    lua_unregister!(lua, b"lop_exec\0".as_ptr());
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
        Debug = 0x400,
        Dump = 0x401,
        DoString = 0x402,
        EjectDll = 0x403,
    }
}

#[macro_export]
macro_rules! cstr_to_str {
    ($e:expr) => {
        unsafe { CStr::from_ptr($e) }
            .to_str()
            .map_err(|e| LoleError::InvalidRawString(format!("{e:?}")))
    };
}

enum PlayerPosition {
    Ok,
    TooNear,
    TooFar,
    WrongFacing,
}

fn get_melee_position_status(
    ideal_position_relative: Vec3,
    tpdiff_unit: Vec3,
    prot_unit: Vec3,
) -> PlayerPosition {
    if ideal_position_relative.length() > 1.0 {
        PlayerPosition::TooFar
    } else if Vec3::dot(tpdiff_unit, prot_unit) < 0.85 {
        PlayerPosition::WrongFacing
    } else {
        PlayerPosition::Ok
    }
}

fn get_caster_position_status(
    range_min: f32,
    range_max: f32,
    tpdiff: Vec3,
    prot_unit: Vec3,
) -> PlayerPosition {
    let distance_from_target = tpdiff.length();
    if distance_from_target >= range_max {
        PlayerPosition::TooFar
    } else if distance_from_target < range_min {
        PlayerPosition::TooNear
    } else if Vec3::dot(tpdiff.unit(), prot_unit) < 0.99 {
        PlayerPosition::WrongFacing
    } else {
        PlayerPosition::Ok
    }
}

fn handle_lop_exec(lua: lua_State) -> LoleResult<i32> {
    let nargs = lua_gettop(lua);
    if nargs == 0 {
        println!("lop_exec: no opcode provided");
        return Err(LoleError::MissingOpcode);
    }
    let nargs = nargs - 1;
    match lua_tointeger(lua, 1).try_into()? {
        Opcode::Nop => {}
        Opcode::TargetGuid if nargs == 1 => {
            let guid_str = lua_tostring!(lua, 2)?;
            let guid = guid_from_str(guid_str)?;
            SelectUnit(guid);
        }
        Opcode::Follow if nargs == 1 => {
            let name = lua_tostring!(lua, 2)?;
            let om = ObjectManager::new()?;
            if let Some(o) = om.get_unit_by_name(name) {
                let p = om.get_player()?;
                let (pp, tp) = (p.get_pos(), o.get_pos());
                let tpdiff = tp - pp;
                let distance = tpdiff.length();
                if distance < 10.0 {
                    dostring(&format!("FollowUnit(\"{}\")", o.get_name()))?;
                    TRYING_TO_FOLLOW.set(None);
                } else {
                    TRYING_TO_FOLLOW.set(Some(o.get_guid()));
                }
            }
        }
        Opcode::StopFollow if nargs == 0 => {
            dostring("MoveForwardStart()")?;
            dostring("MoveForwardStop()")?;
        }
        Opcode::Dump => {
            for o in ObjectManager::new()? {
                println!("{o}");
            }
        }
        Opcode::CasterRangeCheck if nargs == 2 => {
            let om = ObjectManager::new()?;
            if let (player, Some(target)) = om.get_player_and_target()? {
                let range_min = lua_tonumber_f32!(lua, 2);
                let range_max = lua_tonumber_f32!(lua, 3);
                let (pp, prot) = player.get_pos_and_rotvec();
                let tp = target.get_pos();
                let tpdiff = tp - pp;
                let tpdiff_unit = tpdiff.unit();
                match get_caster_position_status(range_min, range_max, tpdiff, prot) {
                    PlayerPosition::TooNear => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: tp - (range_min + 2.0) * tpdiff_unit,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            ..Default::default()
                        })?;
                    }
                    PlayerPosition::TooFar => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: tp - (range_max - 2.0) * tpdiff_unit,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            ..Default::default()
                        })?;
                    }
                    PlayerPosition::WrongFacing => {
                        if !ctm::event_in_progress()? {
                            set_facing(pp, tpdiff.to_rot_value())?;
                        }
                    }
                    PlayerPosition::Ok => {}
                }
            }
        }

        Opcode::MeleeBehind if nargs == 1 => {
            const MELEE_NUKE_ANGLE: f32 = 0.35;
            let om = ObjectManager::new()?;
            let (player, target) = om.get_player_and_target()?;
            if let Some(target) = target {
                let range_min = lua_tonumber_f32!(lua, 2);
                // TODO: do the if range_min > 2: range_min = range_min - 2 business in Lua
                let (pp, prot) = player.get_pos_and_rotvec();
                let (tp, trot) = target.get_pos_and_rotvec();

                let candidate1 = tp + range_min * trot.rotated_2d_cw((1.0 - MELEE_NUKE_ANGLE) * PI);
                let candidate2 = tp + range_min * trot.rotated_2d_cw((1.0 + MELEE_NUKE_ANGLE) * PI);

                let ideal = pp.select_closer(candidate1, candidate2);
                let ideal_relative = (ideal - pp).zero_z();
                let tpdiff = (tp - pp).zero_z();

                match get_melee_position_status(ideal_relative, tpdiff.unit(), prot) {
                    PlayerPosition::TooFar => {
                        ctm::add_to_queue(CtmEvent {
                            target_pos: ideal + 0.5 * prot,
                            priority: CtmPriority::Replace,
                            action: CtmAction::Move,
                            ..Default::default()
                        })?;
                    }
                    PlayerPosition::WrongFacing => set_facing(pp, tpdiff.to_rot_value())?,
                    _ => {}
                }
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
            dostring(script)?;
        }
        Opcode::GetUnitPosition if nargs == 1 => {
            let om = ObjectManager::new()?;
            let arg = lua_tostring!(lua, 2)?.trim();
            let object = if arg.starts_with("0x") {
                let guid = guid_from_str(arg)?;
                om.get_object_by_guid(guid)
            } else if arg == "player" {
                Some(om.get_player()?)
            } else if arg == "target" {
                let target_guid = om.get_player_target_guid();
                om.get_object_by_guid(target_guid)
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
                return Ok(NO_RETVALS);
            }
        }
        Opcode::Debug => {
            let om = ObjectManager::new()?;
            let player = om.get_player()?;
            set_facing(player.get_pos(), 0.0)?;
        }
        Opcode::EjectDll => {
            SHOULD_EJECT.set(true);
        }
        v => return Err(LoleError::InvalidOrUnimplementedOpcodeCallNargs(v, nargs)),
    }
    Ok(NO_RETVALS)
}

#[no_mangle]
pub unsafe extern "C" fn lop_exec(lua: lua_State) -> i32 {
    match handle_lop_exec(lua) {
        Ok(num_retvals) => num_retvals,
        Err(error) => {
            println!("lop_exec: error: {:?}", error);
            NO_RETVALS
        }
    }
}

pub fn chatframe_print(msg: &str) {
    let _ = dostring(&format!("DEFAULT_CHAT_FRAME:AddMessage(\"* dll: {msg}\")"));
}

pub fn get_lua_state() -> LoleResult<lua_State> {
    let res = deref::<lua_State, 1>(addrs::wow_lua_State);
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

    let lua = get_lua_state()?;
    lua_getglobal!(lua, b"lop_exec\0".as_ptr());
    match lua_gettype(lua, -1).try_into()? {
        LuaType::Function => {}
        _ => register_lop_exec()?,
    }
    lua_settop(lua, -1);
    Ok(())
}

pub fn prepare_lua_prot_patch() -> Patch {
    use asm::NOP;
    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push_slice(&[0xB8, 0x01, 0x00, 0x00, 0x00, 0x5D, 0xC3, NOP, NOP]);

    Patch {
        name: "Lua_Prot",
        patch_addr: crate::addrs::LUA_PROT_PATCHADDR,
        original_opcodes: copy_original_opcodes(crate::addrs::LUA_PROT_PATCHADDR, 9),
        patch_opcodes,
        kind: PatchKind::OverWrite,
    }
}

#[allow(non_upper_case_globals)]
mod addrs {
    use crate::Addr;
    pub const lua_gettop: Addr = 0x0072DAE0;
    pub const lua_settop: Addr = 0x0072DB00;
    pub const lua_tonumber: Addr = 0x0072DF40;
    pub const lua_tointeger: Addr = 0x0072DF80;
    pub const lua_tostring: Addr = 0x0072DFF0;
    pub const lua_touserdata: Addr = 0x0072E120;
    pub const lua_toboolean: Addr = 0x0072DFC0;
    pub const lua_pushnumber: Addr = 0x0072E1A0;
    pub const lua_pushinteger: Addr = 0x0072E1D0;
    pub const lua_isstring: Addr = 0x0072DE70;
    pub const lua_tolstring: Addr = 0x0072DFF0;
    pub const lua_pushlstring: Addr = 0x0072E200;
    pub const lua_pushstring: Addr = 0x0072E250;
    pub const lua_pushboolean: Addr = 0x0072E3B0;
    pub const lua_pushcclosure: Addr = 0x0072E2F0;
    pub const lua_pushnil: Addr = 0x0072E180;
    pub const lua_gettable: Addr = 0x0072E440;
    pub const lua_setfield: Addr = 0x0072E7E0;
    pub const lua_getfield: Addr = 0x0072E470;
    pub const lua_getfield_wow_weird: Addr = 0x0072F710;
    pub const lua_replace: Addr = 0x0072DC80;
    pub const lua_next: Addr = 0x0072EE30;
    pub const lua_gettype: Addr = 0x0072DDC0;
    pub const lua_gettypestring: Addr = 0x0072DDE0;
    pub const lua_dostring: Addr = 0x00706C80;

    pub const wow_lua_State: Addr = 0xE1DB84;

    // the Lua stack in wow prevents all lua_pushcclosure calls
    // where the closure address isn't within Wow.exe's .code section
    // this is the upper limit

    pub const vfp_max: Addr = 0xE1F834;
    pub const FrameScript_Register: Addr = 0x007059B0;
}
