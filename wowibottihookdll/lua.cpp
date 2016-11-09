#include "lua.h"
#include "opcodes.h"

p_lua_pushcclosure	lua_pushcclosure = (p_lua_pushcclosure)0x0072E2F0;
p_lua_setfield		lua_setfield = (p_lua_setfield)0x0072E7E0;
p_lua_gettop		lua_gettop = (p_lua_gettop)0x0072DAE0;
p_lua_pushnumber	lua_pushnumber = (p_lua_pushnumber)0x0072E1A0;
p_lua_pushnil		lua_pushnil = (p_lua_pushnil)0x0072E180;
p_lua_pushboolean	lua_pushboolean = (p_lua_pushboolean)0x0072E3B0;
p_lua_pushinteger	lua_pushinteger = (p_lua_pushinteger)0x0072E1D0;
p_lua_tolstring		lua_tolstring = (p_lua_tolstring)0x0072DFF0;
p_lua_pushlstring	lua_pushlstring = (p_lua_pushlstring)0x0072E200;
p_lua_getfield		lua_getfield = (p_lua_getfield)0x0072F710;
p_lua_tonumber		lua_tonumber = (p_lua_tonumber)0x0072DF40;
p_lua_tointeger		lua_tointeger = (p_lua_tointeger)0x0072DF80;

int lua_registered = 0;

int register_lop_exec() {

	lua_State *state = (lua_State*)(*(uint **)0xE1DB84);

	lua_register(state, "lop_exec", lop_exec);

	uint *vfp_min = (uint*)0xE1F830;
	uint *vfp_max = (uint*)0xE1F834;

	// explanation: there's a check during lua_register that ensures the jump address lies within
	// Wow.exe's .code section, so the upper limit needs to be... adjusted =)
	*vfp_max = 0xFFFFFFFF;
	
	return 1;
}



