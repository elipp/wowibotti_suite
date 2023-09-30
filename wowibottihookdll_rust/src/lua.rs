use std::arch::asm;
use std::ffi::{c_char, c_void, CStr};

use crate::objectmanager::ObjectManager;
use crate::{deref, write_addr, Addr};
type lua_State = *mut c_void;

macro_rules! define_lua_const {
    ($name:ident, ($($param:ident : $param_ty:ty),*) -> $ret_ty:ty) => {
        pub fn $name ($($param : $param_ty),*) -> $ret_ty {
            let ptr = addrs::$name as *const ();
            let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute(ptr) };
            func($($param),*)
        }
    }
}

type lua_CFunction = unsafe extern "C" fn(lua_State) -> i32;

const LUA_GLOBALSINDEX: i32 = -10002;

#[repr(u8)]
#[derive(Debug)]
enum LuaType {
    Nil,
    Boolean,
    UserData,
    Number,
    // Integer, // not official Lua
    String,
    Table,
    Function,
    UserData2,
    Thread,
    Proto,
    Upval,
    Unknown(i32),
}

impl From<i32> for LuaType {
    fn from(value: i32) -> Self {
        match value {
            0 => LuaType::Nil,
            1 => LuaType::Boolean,
            2 => LuaType::UserData,
            3 => LuaType::Number,
            4 => LuaType::String,
            5 => LuaType::Table,
            6 => LuaType::Function,
            7 => LuaType::UserData2,
            8 => LuaType::Thread,
            9 => LuaType::Proto,
            10 => LuaType::Upval,
            v => LuaType::Unknown(v),
        }
    }
}

define_lua_const!(
    lua_gettop,
    (state: lua_State) -> i32
);

define_lua_const!(
    lua_settop,
    (state: lua_State, idx: i32) -> ());

define_lua_const!(
    lua_pushcclosure,
    (state: lua_State, closure: lua_CFunction, n: i32) -> ());

// The wow-version of lua_getfield uses an non-standard 4th argument, which is the length of the key string. (but it is calculated regardless?)
define_lua_const!(
    lua_getfield,
    (state: lua_State, idx: i32, k: *const c_char) -> i32);

// The wow-version of lua_getfield uses an non-standard 4th argument, which is the length of the key string. (but it is calculated regardless?)
define_lua_const!(
    lua_setfield,
    (state: lua_State, idx: i32, k: *const c_char) -> ());

define_lua_const!(
    lua_tointeger,
    (state: lua_State, idx: i32) -> i32);

define_lua_const!(
    lua_gettype,
    (state: lua_State, idx: i32) -> i32);

define_lua_const!(
    lua_tolstring,
    (state: lua_State, idx: i32, len: *mut usize) -> *const c_char);

define_lua_const!(
    lua_tonumber,
    (state: lua_State, idx: i32) -> f64);

define_lua_const!(
    lua_dostring,
    (script: *const c_char, _s: *const c_char, taint: u32) -> ());

macro_rules! DoString {
    ($script:expr) => {
        lua_dostring(
            $script.as_ptr() as *const c_char,
            $script.as_ptr() as *const c_char,
            0,
        )
    };
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

macro_rules! lua_getglobal {
    ($lua:expr, $s:expr) => {
        lua_getfield($lua, LUA_GLOBALSINDEX, $s as *const c_char)
    };
}

pub fn register_lop_exec() {
    unsafe { write_addr(addrs::vfp_max, 0xEFFFFFFFu32) };
    let lua = get_lua_State().expect("lua_State");
    lua_register!(lua, b"lop_exec\0".as_ptr(), lop_exec);
    println!("lop_exec registered!")
}

#[derive(Debug)]
#[repr(u8)]
enum Opcode {
    Nop,
    Dump,
    DoString,
    Debug,
    Unknown(i32),
}

impl From<i32> for Opcode {
    fn from(value: i32) -> Self {
        match value {
            0 => Opcode::Nop,
            1 => Opcode::Dump,
            2 => Opcode::DoString,
            0xFF => Opcode::Debug,
            v => Opcode::Unknown(v),
        }
    }
}

macro_rules! cstr_to_str {
    ($e:expr) => {
        unsafe { CStr::from_ptr($e) }
            .to_str()
            .expect("CStr conversion")
    };
}

#[no_mangle]
pub unsafe extern "C" fn lop_exec(lua: lua_State) -> i32 {
    let nargs = lua_gettop(lua);
    println!("lop_exec: got {nargs} args");
    if nargs == 0 {
        return 0;
    }
    let op = lua_tointeger(lua, 1);
    match op.into() {
        Opcode::Nop => {}
        Opcode::Dump => {
            if let Ok(om) = ObjectManager::new() {
                for o in om {
                    println!("{o}");
                }
            }
        }
        Opcode::DoString if nargs > 1 => {
            asm!("int 3h");
            let mut len = 0;
            let m = lua_tolstring(lua, 2, std::ptr::addr_of_mut!(len));
            println!("{m:p}");
            let s = cstr_to_str!(m);
            let script = format!("{s}");
            println!("DoString({s})");
            DoString!(script);
            // lua_getglobal!(lua, s);
            // let m = lua_gettype(lua, -1);
            // println!("{m:?}");
            // lua_settop(lua, -2);
        }
        Opcode::Debug => {
            let res = lua_getglobal!(lua, b"lop_exec\0".as_ptr() as *const _);
            let m = lua_gettype(lua, -1);
            lua_settop(lua, -1);
            println!("res: {res}, type: {m}");
        }
        v => {
            // println!("Unknown op {v:?}");
        }
    }

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

pub fn register_if_not_registered() {
    if ObjectManager::new().is_err() {
        // we're not in the world
        return;
    }

    let lua = get_lua_State().expect("lua_State");
    lua_getglobal!(lua, b"lop_exec\0".as_ptr());
    match lua_gettype(lua, -1).into() {
        LuaType::Function => {}
        _ => register_lop_exec(),
    }
    lua_settop(lua, -1);
}

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
