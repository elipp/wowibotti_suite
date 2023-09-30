#pragma once

#include <vector>
#include <string>
#include <cstring>

#define LUA_GLOBALSINDEX	(-10002)
#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)
#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))
#define lua_getglobal(L,s)  lua_getfield(L, LUA_GLOBALSINDEX, s)

typedef struct lua_State lua_State;
typedef std::vector<std::string> lua_rvals_t;

int get_rvals(lua_State *L);
const lua_rvals_t& LUA_RVALS();
const lua_rvals_t &dostring_getrvals(const std::string &script);

typedef int(*lua_CFunction) (lua_State *L);
typedef double lua_Number;

typedef void(*p_lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
extern const p_lua_pushcclosure lua_pushcclosure;

typedef void(*p_lua_setfield) (lua_State *L, int idx, const char *k);
extern const p_lua_setfield lua_setfield;

typedef int(*p_lua_gettop) (lua_State *L);
extern const p_lua_gettop lua_gettop;

typedef int(*p_lua_settop) (lua_State* L, int idx);
extern const p_lua_settop lua_settop;

#define lua_pop(L,n)  lua_settop(L, -(n)-1)

typedef void(*p_lua_pushnumber) (lua_State *L, lua_Number n);
extern const p_lua_pushnumber lua_pushnumber;

typedef void(*p_lua_pushnil)(lua_State *L);
extern const p_lua_pushnil lua_pushnil;

typedef void(*p_lua_pushlstring) (lua_State *L, const char* str, size_t len);
extern const p_lua_pushlstring lua_pushlstring;

typedef void(*p_lua_pushinteger) (lua_State *L, int i);
extern const p_lua_pushinteger lua_pushinteger;

typedef void(*p_lua_pushboolean) (lua_State *L, int b);
extern const p_lua_pushboolean lua_pushboolean;

typedef const char*(*p_lua_tolstring)(lua_State *L, int idx, size_t *len);
extern const p_lua_tolstring lua_tolstring;

typedef void(*p_lua_getfield) (lua_State *L, int idx, const char* key, size_t key_len);
extern const p_lua_getfield lua_getfield_; // note the underscore at the end! we use the following macro instead

#define lua_getfield(L, idx, key) lua_getfield_(L, idx, key, strlen(key))

typedef lua_Number(*p_lua_tonumber) (lua_State *L, int idx);
extern const p_lua_tonumber lua_tonumber;

typedef int (*p_lua_tointeger) (lua_State *L, int idx);
extern const p_lua_tointeger lua_tointeger;

typedef int(*p_lua_toboolean) (lua_State *L, int idx);
extern const p_lua_toboolean lua_toboolean;

typedef int(*p_lua_gettable) (lua_State* L, int idx);
extern const p_lua_gettable lua_gettable;

typedef int(*p_lua_next) (lua_State* L, int idx);
extern const p_lua_next lua_next;

int register_lop_exec();

std::vector<std::string> get_lua_stringtable(lua_State* L, int idx);

inline const char* lua_tostring(lua_State* L, int idx) {
	size_t len = 0;
	return lua_tolstring(L, idx, &len);
}

inline void lua_pushstring(lua_State* L, const std::string& str) {
	lua_pushlstring(L, str.c_str(), str.length());
}

extern int lua_registered;

enum class LUA_TYPE : int {
	NIL = 0,
	BOOLEAN = 1, // there's some weird redefinition error, hence underscore
	USERDATA = 2,
	NUMBER = 3,
	INTEGER = 3, // NOT OFFICIAL!
	STRING = 4,
	TABLE = 5,
	FUNCTION = 6,
	USERDATA2 = 7,
	PROTO = 8,
	UPVAL = 9,
};
// TBC CLIENT:
// CPU Dump
// Address   Value      Comments
// 008D7890   008919D0  ; ASCII "nil"
// 008D7894   00891994  ; ASCII "boolean"
// 008D7898   008D7884  ; ASCII "userdata"
// 008D789C   0089198C  ; ASCII "number"
// 008D78A0   008909C0  ; ASCII "string"
// 008D78A4   008D6FE4  ; ASCII "table"
// 008D78A8   008D7878  ; ASCII "function"
// 008D78AC   008D7884  ; ASCII "userdata"
// 008D78B0   008D7870  ; ASCII "thread"
// 008D78B4   008D7868  ; ASCII "proto"
// 008D78B8   008D7860  ; ASCII "upval"


// these are officially LUA_TYPE(lua_State *L, int idx);
// and lua_typename(lua_State *L, int tp);
typedef LUA_TYPE (__cdecl *p_lua_gettype) (lua_State* L, int idx);
extern const p_lua_gettype lua_gettype;

typedef const char*(__cdecl *p_lua_gettypestring) (lua_State* L, LUA_TYPE type);
extern const p_lua_gettypestring lua_gettypestring;

#define lua_gettypestr(STATE, idx) lua_gettypestring(STATE, lua_gettype(STATE, idx))

lua_State *get_lua_State(); 
	

//define lua335_getfield 0x0059AAE0

// hehe wow lua is confirmed to be 5.1.1 now :D

		//CPU Dump
		//Address   ASCII dump
		//	00A47098           Lua : Lua 5.1.1 Copyrigh
		//	00A470B8  t(C) 1994 - 2006 Lua.org, PUC - Rio
		//	00A470D8   $
		//	$Authors : R.Ierusalimschy, L
		//	00A470F8.H.de Figueiredo & W.Celes $

		//	00A47118  $URL : www.lua.org $
		//	$Permission
		//	00A47138  is hereby granted, free of charg
		//	00A47158  e, to any person obtaining$
		//	$a c
		//	00A47178  opy of this software and associa
		//	00A47198  ted documentation files(the$
		//		$'
		//		00A471B8  Software'), to deal in the Softw
		//		00A471D8  are without restriction, includi
		//		00A471F8  ng$
		//		$without limitation the righ
		//		00A47218  ts to use, copy, modify, merge,
		//		00A47238  publish, $
		//		$distribute, sublicens
		//		00A47258  e, and /or sell copies of the Sof
		//		00A47278  tware, and to$
		//		$permit persons t
		//		00A47298  o whom the Software is furnished
		//		00A472B8   to do so, subject to$
		//		$the foll
		//		00A472D8  owing conditions : $
		//		$The above co
		//		00A472F8  pyright notice and this permissi
		//		00A47318  on notice shall be$
		//		$included in
		//		00A47338   all copies or substantial porti
		//		00A47358  ons of the Software.

