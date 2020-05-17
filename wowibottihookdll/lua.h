#pragma once

#include <vector>
#include <string>
#include <cstring>

#define LUA_GLOBALSINDEX	(-10002)
#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)
#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define PUSHSTRING(L, S) lua_pushlstring(L, S, strlen(S))
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
extern p_lua_pushcclosure lua_pushcclosure;

typedef void(*p_lua_setfield) (lua_State *L, int idx, const char *k);
extern p_lua_setfield lua_setfield;

typedef int(*p_lua_gettop) (lua_State *L);
extern p_lua_gettop lua_gettop;

typedef int(*p_lua_settop) (lua_State* L, int idx);
extern p_lua_settop lua_settop;

#define lua_pop(L,n)  lua_settop(L, -(n)-1)

typedef void(*p_lua_pushnumber) (lua_State *L, lua_Number n);
extern p_lua_pushnumber lua_pushnumber;

typedef void(*p_lua_pushnil)(lua_State *L);
extern p_lua_pushnil lua_pushnil;

typedef void(*p_lua_pushlstring) (lua_State *L, const char* str, size_t len);
extern p_lua_pushlstring lua_pushlstring;

typedef void(*p_lua_pushinteger) (lua_State *L, int i);
extern p_lua_pushinteger lua_pushinteger;

typedef void(*p_lua_pushboolean) (lua_State *L, int b);
extern p_lua_pushboolean lua_pushboolean;

typedef const char*(*p_lua_tolstring)(lua_State *L, int idx, size_t *len);
extern p_lua_tolstring lua_tolstring;

typedef void(*p_lua_getfield) (lua_State *L, int idx, const char* key, size_t key_len);
extern p_lua_getfield lua_getfield_; // note the underscore at the end! we use the following macro instead

#define lua_getfield(L, idx, key) lua_getfield_(L, idx, key, strlen(key))

typedef lua_Number(*p_lua_tonumber) (lua_State *L, int idx);
extern p_lua_tonumber lua_tonumber;

typedef int (*p_lua_tointeger) (lua_State *L, int idx);
extern p_lua_tointeger lua_tointeger;

typedef int(*p_lua_toboolean) (lua_State *L, int idx);
extern p_lua_toboolean lua_toboolean;

typedef int(*p_lua_gettable) (lua_State* L, int idx);
extern p_lua_gettable lua_gettable;

typedef int(*p_lua_next) (lua_State* L, int idx);
extern p_lua_next lua_next;

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

#define lua335_unregisterfunction 0x00817FD0
#define lua335_getvariable 0x00818010
#define lua335_execute 0x00819210
#define lua335_gettext 0x00819D40
#define lua335_signalevent 0x0081B530
#define lua335_gettop 0x0084DBD0
#define lua335_settop 0x0084DBF0
#define lua335_isnumber 0x0084DF20
#define lua335_isstring 0x0084DF60
#define lua335_equal 0x0084DFE0
#define lua335_tonumber 0x0084E030
#define lua335_tointeger 0x0084E070
#define lua335_toboolean 0x0084E0B0
#define lua335_tolstring 0x0084E0E0
#define lua335_objlen 0x0084E150
#define lua335_tocfunction 0x0084E1C0
#define lua335_tothread 0x0084E1F0
#define lua335_touserdata 0x0084E210
#define lua335_pushnil 0x0084E280
#define lua335_pushnumber 0x0084E2A0
#define lua335_pushinteger 0x0084E2D0
#define lua335_pushlstring 0x0084E300
#define lua335_pushstring 0x0084E350
#define lua335_pushcclosure 0x0084E400
#define lua335_pushboolean 0x0084E4D0
#define lua335_gettable 0x0084E560
#define lua335_findtable 0x0084E590
#define lua335_createtable 0x0084E6E0
#define lua335_setfield 0x0084E900
#define lua335_displayerror 0x0084F280
#define lua335_getfield 0x0084F3B0
#define lua335_next 0x0084EF50
#define wowlua335_next 0x00854690
#define wowlua335_type 0x00854660
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
