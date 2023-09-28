use std::arch::asm;
use std::ffi::{c_char, c_void};

use crate::{deref, Addr};
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

define_lua_const!(
    lua_gettop,
    (state: lua_State) -> i32
);

define_lua_const!(
    lua_pushcclosure,
    (state: lua_State, closure: lua_CFunction, name: *const c_char) -> i32
);

#[no_mangle]
pub unsafe extern "C" fn test_cclosure(lua: lua_State) -> i32 {
    0
}

#[no_mangle]
pub unsafe extern "cdecl" fn lop_exec(lua: lua_State) -> i32 {
    lua_pushcclosure(lua, test_cclosure, "lop_exec".as_ptr() as *const i8);
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

// typedef int(*lua_CFunction) (lua_State *L);
// typedef double lua_Number;

// typedef void(*p_lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
// extern const p_lua_pushcclosure lua_pushcclosure;

// typedef void(*p_lua_setfield) (lua_State *L, int idx, const char *k);
// extern const p_lua_setfield lua_setfield;

// typedef int(*p_lua_gettop) (lua_State *L);
// extern const p_lua_gettop lua_gettop;

// typedef int(*p_lua_settop) (lua_State* L, int idx);
// extern const p_lua_settop lua_settop;

// #define lua_pop(L,n)  lua_settop(L, -(n)-1)

// typedef void(*p_lua_pushnumber) (lua_State *L, lua_Number n);
// extern const p_lua_pushnumber lua_pushnumber;

// typedef void(*p_lua_pushnil)(lua_State *L);
// extern const p_lua_pushnil lua_pushnil;

// typedef void(*p_lua_pushlstring) (lua_State *L, const char* str, size_t len);
// extern const p_lua_pushlstring lua_pushlstring;

// typedef void(*p_lua_pushinteger) (lua_State *L, int i);
// extern const p_lua_pushinteger lua_pushinteger;

// typedef void(*p_lua_pushboolean) (lua_State *L, int b);
// extern const p_lua_pushboolean lua_pushboolean;

// typedef const char*(*p_lua_tolstring)(lua_State *L, int idx, size_t *len);
// extern const p_lua_tolstring lua_tolstring;

// typedef void(*p_lua_getfield) (lua_State *L, int idx, const char* key, size_t key_len);
// extern const p_lua_getfield lua_getfield_; // note the underscore at the end! we use the following macro instead

// #define lua_getfield(L, idx, key) lua_getfield_(L, idx, key, strlen(key))

// typedef lua_Number(*p_lua_tonumber) (lua_State *L, int idx);
// extern const p_lua_tonumber lua_tonumber;

// typedef int (*p_lua_tointeger) (lua_State *L, int idx);
// extern const p_lua_tointeger lua_tointeger;

// typedef int(*p_lua_toboolean) (lua_State *L, int idx);
// extern const p_lua_toboolean lua_toboolean;

// typedef int(*p_lua_gettable) (lua_State* L, int idx);
// extern const p_lua_gettable lua_gettable;

// typedef int(*p_lua_next) (lua_State* L, int idx);
// extern const p_lua_next lua_next;

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
    pub const lua_setfield: Addr = 0x0072E7E0;
    pub const lua_getfield: Addr = 0x0072F710;
    pub const lua_replace: Addr = 0x0072DC80;
    pub const lua_next: Addr = 0x0072EE30;
    pub const FrameScript_Register: Addr = 0x007059B0;
    pub const lua_gettype: Addr = 0x0072DDC0;
    pub const lua_gettypestring: Addr = 0x0072DDE0;
    pub const lua_gettable: Addr = 0x0072E440;

    pub const wow_lua_State: Addr = 0xE1DB84;
}
