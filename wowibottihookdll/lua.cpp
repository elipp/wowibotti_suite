#include "lua.h"
#include "opcodes.h"

const p_lua_pushcclosure	lua_pushcclosure = (p_lua_pushcclosure)lua335_pushcclosure;
const p_lua_setfield		lua_setfield = (p_lua_setfield)lua335_setfield;
const p_lua_gettop			lua_gettop = (p_lua_gettop)lua335_gettop;
const p_lua_settop			lua_settop = (p_lua_settop)lua335_settop;
const p_lua_pushnumber		lua_pushnumber = (p_lua_pushnumber)lua335_pushnumber;
const p_lua_pushnil			lua_pushnil = (p_lua_pushnil)lua335_pushnil;
const p_lua_pushboolean		lua_pushboolean = (p_lua_pushboolean)lua335_pushboolean;
const p_lua_pushinteger		lua_pushinteger = (p_lua_pushinteger)lua335_pushinteger;
const p_lua_tolstring		lua_tolstring = (p_lua_tolstring)lua335_tolstring;
const p_lua_pushlstring		lua_pushlstring = (p_lua_pushlstring)lua335_pushlstring;
const p_lua_getfield		lua_getfield_ = (p_lua_getfield)lua335_getfield;
const p_lua_tonumber		lua_tonumber = (p_lua_tonumber)lua335_tonumber;
const p_lua_tointeger		lua_tointeger = (p_lua_tointeger)lua335_tointeger;
const p_lua_toboolean		lua_toboolean = (p_lua_toboolean)lua335_toboolean;
const p_lua_gettable		lua_gettable = (p_lua_gettable)lua335_gettable;
const p_lua_next			lua_next = (p_lua_next)lua335_next;

// semi-custom stuff

const p_lua_gettype lua_gettype = (p_lua_gettype)0x84DEB0;
const p_lua_gettypestring lua_gettypestring = (p_lua_gettypestring)0x84DED0;

#define lua243_state 0xE1DB84
#define lua335_state 0xD3F78C

int lua_registered = 0;

int register_lop_exec() {

	lua_State *state = (lua_State*)(*(uint **)lua335_state);

	// TBC values
	//uint *vfp_min = (uint*)0xE1F830;
	//uint *vfp_max = (uint*)0xE1F834;

	uint *vfp_min = (uint*)0xD415B8;
	uint *vfp_max = (uint*)0xD415BC;

	// explanation: there's a check during lua_register that ensures the jump address lies within
	// Wow.exe's .code section, so the upper limit needs to be... adjusted =)
	*vfp_max = 0xEFFFFFFE;

	lua_register(state, "lop_exec", lop_exec);
	lua_register(state, "get_rvals", get_rvals);

	return 1;
}

const lua_rvals_t &dostring_getrvals(const std::string &script) {
	DoString("get_rvals(%s)", script.c_str());
	return LUA_RVALS();
}


static std::string lua_stackval_to_string(lua_State* L, int idx) {
	lua_type type = lua_gettype(L, idx);
	size_t len = 0;

	switch (type) {
	case lua_type::string:
		return std::string(lua_tolstring(L, idx, &len));
		break;
	case lua_type::number:
		return std::to_string(lua_tointeger(L, idx));
		break;
	case lua_type::table:
		return "table";
		break;
	default:
		return "unsupported type";
		break;
	}

}
std::vector<std::string> get_lua_stringtable(lua_State* L, int idx) {
	
	std::vector<std::string> ret;
	lua_type type = lua_gettype(L, idx);
	if (type == lua_type::string) {
		size_t len = 0;
		ret.push_back(lua_tolstring(L, idx, &len));
		return ret;
	}

	if (type != lua_type::table) { return ret; }
	if (idx < 2) return ret;

	lua_pushnil(L);
	while (lua_next(L, idx)) {
		// after lua_next, the key lies at -2 and the value at -1, but because we're interested strictly in tables of only strings, we use the value only
		// (the format of tables constructed such as {"example"} is equivalent to {[1] = "example"})
		ret.push_back(lua_stackval_to_string(L, -1)); 
		lua_pop(L, 1);
	}

	return ret;
}
