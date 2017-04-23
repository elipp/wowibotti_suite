#include "lua.h"
#include "opcodes.h"

p_lua_pushcclosure	lua_pushcclosure = (p_lua_pushcclosure)lua335_pushcclosure;
p_lua_setfield		lua_setfield = (p_lua_setfield)lua335_setfield;
p_lua_gettop		lua_gettop = (p_lua_gettop)lua335_gettop;
p_lua_pushnumber	lua_pushnumber = (p_lua_pushnumber)lua335_pushnumber;
p_lua_pushnil		lua_pushnil = (p_lua_pushnil)lua335_pushnil;
p_lua_pushboolean	lua_pushboolean = (p_lua_pushboolean)lua335_pushboolean;
p_lua_pushinteger	lua_pushinteger = (p_lua_pushinteger)lua335_pushinteger;
p_lua_tolstring		lua_tolstring = (p_lua_tolstring)lua335_tolstring;
p_lua_pushlstring	lua_pushlstring = (p_lua_pushlstring)lua335_pushlstring;
p_lua_getfield		lua_getfield = (p_lua_getfield)lua335_getfield;
p_lua_tonumber		lua_tonumber = (p_lua_tonumber)lua335_tonumber;
p_lua_tointeger		lua_tointeger = (p_lua_tointeger)lua335_tointeger;

#define lua243_state 0xE1DB84
#define lua335_state 0xD3F78C

int lua_registered = 0;

int register_lop_exec() {

	lua_State *state = (lua_State*)(*(uint **)lua335_state);

	lua_register(state, "lop_exec", lop_exec);

	// TBC values
	//uint *vfp_min = (uint*)0xE1F830;
	//uint *vfp_max = (uint*)0xE1F834;

	uint *vfp_min = (uint*)0xD415B8;
	uint *vfp_max = (uint*)0xD415BC;

	// explanation: there's a check during lua_register that ensures the jump address lies within
	// Wow.exe's .code section, so the upper limit needs to be... adjusted =)
	*vfp_max = 0xEFFFFFFE;
	
	return 1;
}



