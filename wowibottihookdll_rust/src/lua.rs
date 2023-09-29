use std::arch::asm;
use std::ffi::{c_char, c_void, CStr};

use crate::{deref, write_addr, Addr};
type lua_State = *mut c_void;

macro_rules! define_lua_const {
    ($name:ident, ($($param:ident : $param_ty:ty),*) -> $ret_ty:ty) => {
        pub fn $name ($($param : $param_ty),*) -> $ret_ty {
            let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute_copy(&addrs::$name) };
            func($($param),*)
        }
    }
}

type lua_CFunction = unsafe extern "C" fn(lua_State) -> i32;

const LUA_GLOBALSINDEX: i32 = -10002;

fn strlen(s: *const c_char) -> usize {
    unsafe { CStr::from_ptr(s).to_str().expect("str conversion").len() }
}

define_lua_const!(
    lua_gettop,
    (state: lua_State) -> i32
);

define_lua_const!(
    lua_pushcclosure,
    (state: lua_State, closure: lua_CFunction, n: i32) -> ());

define_lua_const!(
    lua_getfield,
    (state: lua_State, idx: i32, k: *const c_char) -> ());

define_lua_const!(
    lua_setfield,
    (state: lua_State, idx: i32, k: *const c_char) -> ());

macro_rules! lua_setglobal {
    ($lua:expr, $key:expr) => {
        lua_setfield($lua, LUA_GLOBALSINDEX, $key);
    };
}

macro_rules! lua_register {
    ($lua:expr, $name:expr, $closure:expr) => {
        lua_pushcclosure($lua, $closure, 0);
        lua_setglobal!($lua, $name.as_ptr() as *const c_char);
    };
}

macro_rules! lua_getglobal {
    ($lua:expr, $s:expr) => {
        lua_getfield($lua, LUA_GLOBALSINDEX, $s)
    };
}

pub fn register_lop_exec() {
    unsafe { write_addr(addrs::vfp_max, 0xEFFFFFFFu32) };
    let lua = get_lua_State().expect("lua_State");
    lua_register!(lua, b"lop_exec\0", lop_exec);
    println!("lop_exec registered!")
}

#[no_mangle]
pub unsafe extern "C" fn test_cclosure(lua: lua_State) -> i32 {
    0
}

#[no_mangle]
pub unsafe extern "C" fn lop_exec(lua: lua_State) -> i32 {
    println!("moikkulis");
    return 0;
}

pub fn get_lua_State() -> Option<lua_State> {
    let res = deref::<lua_State, 1>(addrs::wow_lua_State);
    if !res.is_null() {
        Some(res)
    } else {
        None
    }
}

mod addrs {
    use crate::Addr;
    pub const lua_isstring: Addr = 0x0072DE70;
    pub const lua_gettop: Addr = 0x0072DAE0;
    pub const lua_settop: Addr = 0x0072DB00;
    pub const lua_tonumber: Addr = 0x0072DF40;
    pub const lua_tointeger: Addr = 0x0072DF80;
    pub const lua_tostring: Addr = 0x0072DFF0;
    pub const lua_touserdata: Addr = 0x0072E120;
    pub const lua_toboolean: Addr = 0x0072DFC0;
    pub const lua_pushnumber: Addr = 0x0072E1A0;
    pub const lua_pushinteger: Addr = 0x0072E1D0;
    pub const lua_tolstring: Addr = 0x0072DE70;
    pub const lua_pushlstring: Addr = 0x0072E200;
    pub const lua_pushstring: Addr = 0x0072E250;
    pub const lua_pushboolean: Addr = 0x0072E3B0;
    pub const lua_pushcclosure: Addr = 0x0072E2F0;
    pub const lua_pushnil: Addr = 0x0072E180;
    pub const lua_gettable: Addr = 0x0072E440;
    pub const lua_setfield: Addr = 0x0072E7E0;
    pub const lua_getfield: Addr = 0x0072F710;
    pub const lua_replace: Addr = 0x0072DC80;
    pub const lua_next: Addr = 0x0072EE30;
    pub const lua_gettype: Addr = 0x0072DDC0;
    pub const lua_gettypestring: Addr = 0x0072DDE0;
    pub const FrameScript_Register: Addr = 0x007059B0;

    pub const wow_lua_State: Addr = 0xE1DB84;

    // the Lua stack in wow prevents all lua_pushcclosure calls
    // where the closure address isn't within Wow.exe's .code section
    // this is the upper limit
    pub const vfp_max: Addr = 0xE1F834;
}
