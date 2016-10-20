#pragma once

#define LUA_GLOBALSINDEX	(-10002)
#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)
#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define PUSHSTRING(L, S) lua_pushlstring(L, S, strlen(S))
#define lua_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))

typedef struct lua_State lua_State;
typedef int(*lua_CFunction) (lua_State *L);
typedef double lua_Number;

typedef void(*p_lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
extern p_lua_pushcclosure lua_pushcclosure;

typedef void(*p_lua_setfield) (lua_State *L, int idx, const char *k);
extern p_lua_setfield lua_setfield;

typedef int(*p_lua_gettop) (lua_State *L);
extern p_lua_gettop lua_gettop;

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
extern p_lua_getfield lua_getfield;

typedef lua_Number(*p_lua_tonumber) (lua_State *L, int idx);
extern p_lua_tonumber lua_tonumber;

typedef int (*p_lua_tointeger) (lua_State *L, int idx);
extern p_lua_tointeger lua_tointeger;

int register_lop_exec();
extern int lua_registered;
