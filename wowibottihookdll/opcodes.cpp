#include "defs.h"

#include <Windows.h>
#include <Shlobj.h> // for the function that gets the desktop directory path for current user
#include <WinSock2.h>
#include <time.h>
#include <chrono>
#include <numeric>
#include <filesystem>
#include <iostream>

#include "opcodes.h"
#include "ctm.h"
#include "timer.h"
#include "hooks.h"
#include "creds.h"
#include "dungeon_script.h"
#include "lua.h"
#include "packet.h"
#include "linalg.h"
#include "dipcapture.h"
#include "lua.h"
#include "aux_window.h"

#include "govconn.h"

#define ASSERT(left,operator,right) { if(!((left) operator (right))){ std::cout << "ASSERT FAILED: " << #left << #operator << #right << " @ " << __FILE__ << " (" << __LINE__ << "). " << #left << "=" << (left) << "; " << #right << "=" << (right) << std::endl; assert(false); } }

extern HWND wow_hWnd;

time_t in_world = 0;

static int dump_wowobjects_to_log(const std::string &namefilter, const std::string &typefilter);

struct cast_msg_t previous_cast_msg;

GUID_t string_to_GUID(const std::string &G) {
	char *end;
	GUID_t r = strtoull(G.c_str(), &end, 16);
	return r;
}

typedef std::vector <std::string> lua_stringtable;

#define LUA_ARG_OPTIONAL false
#define LUA_ARG_REQUIRED true

enum {
	NO_RVALS,
	RVALS_1,
	RVALS_2,
	RVALS_3,
	RVALS_N,
};

struct lua_arg {
	std::string name;
	lua_type type;
	bool required;
};

typedef int (*opcode_handler)(lua_State* L);

struct lop_func_t {
	LOP opcode;
	std::string opcode_name;
	std::vector<lua_arg> args;
	int num_return_values;
	opcode_handler handler;
};

static int op_handler_NYI(lua_State* L) {
	puts("op_handler_NYI called!\n");
	return 0;
}


static int send_wowpacket(const BYTE* sockbuf, unsigned len) {
	static BYTE dup[256];
	assert(sizeof(dup) >= len);
	memcpy(dup, sockbuf, len);
	SOCKET s = get_wow_socket_handle();
	encrypt_packet_header(dup);
	return send(s, (const char*)dup, len, 0);
}

static struct follow_state_t {
	int close_enough = 1;
	Timer timer;
	std::string target_name;

	void start(const std::string &name) {
		close_enough = 0;
		target_name = name;
		timer.start();
	}

	void clear() {
		close_enough = 1;
		target_name = "";
	}

} follow_state;


enum class POSFLAG : int {
	NONE = 0,
	TOONEAR,
	TOOFAR,
	WRONG_FACING,
	VALID
};

static const char* to_string(POSFLAG f) {
	static const char* POSFLAG_STRINGS[] = {
		"NONE",
		"TOONEAR",
		"TOOFAR",
		"WRONG_FACING",
		"VALID"
	};

	return POSFLAG_STRINGS[(int)f];
}

static inline POSFLAG get_position_flags_melee(const vec3 &dpos_ideal, const vec3& tpdiff_unit, const vec3& prot_unit) {
	float dist = dpos_ideal.length();
	if (dist > 1.0) { return POSFLAG::TOOFAR; }

	float cosb = dot(tpdiff_unit, prot_unit); // both are of length 1
	if (cosb < 0.85) { return POSFLAG::WRONG_FACING; }

	return POSFLAG::VALID;
}


static inline POSFLAG get_position_flags_ranged(float minrange, float maxrange, const vec3& tpdiff, const vec3& prot_unit) {
	float tpl = tpdiff.length();
	if (tpl < minrange) { return POSFLAG::TOONEAR; }
	if (tpl > maxrange) { return POSFLAG::TOOFAR; }

	float cosb = dot(tpdiff.unit(), prot_unit);
	if (cosb < 0.99) { return POSFLAG::WRONG_FACING; }
	
	return POSFLAG::VALID;
}

static void SetFacing_local(float angle) {
	ObjectManager OM;
	WowObject p;
	OM.get_local_object(&p);

	static DWORD SetFacing_wow = 0x989B70;
	DWORD facing_object = (DEREF(p.get_base() + 0xD8));// + 0x20);
	
	_asm {
		fld angle;
		push angle;
		fstp angle;
		mov ecx, facing_object;
		call SetFacing_wow;
	}
	//PRINT("current facing_angle: %f\n", *facing_angle);
	//*facing_angle = angle;
}

static std::chrono::system_clock::time_point last_CMSG_SET_FACING_sent = std::chrono::system_clock::now();

long long get_time_from_ms(const std::chrono::system_clock::time_point& t) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

static void synthetize_CMSG_SET_FACING(float angle) {
	// NOTE: the client side address of the facing angle is at [ECX of when EIP == 989B9B] + 0x20

	ObjectManager OM;
	GUID_t pGUID = OM.get_local_GUID();
	BYTE packed_GUID[8];
	BYTE gBYTES[8];
	memcpy(gBYTES, &pGUID, sizeof(pGUID));
	BYTE mark = 0;
	BYTE mask = 0;
	for (int i = 0; i < 8; ++i) {
		if (gBYTES[i] > 0) {
			packed_GUID[mark] = gBYTES[i];
			++mark;
			mask |= (1 << i);
		}
	}

	//PRINT("%llX\n", pGUID);
	//print_bytes("gBYTES", gBYTES, sizeof(gBYTES));
	//print_bytes("packed_GUID", packed_GUID, mark);
	//PRINT("mask: %x\n", mask);

	const BYTE packet_hdr[] = {
		0x00, 0x23 + mark, 0xDA, 0x00, 0x00, 0x00,
	};

	bytebuffer_t packet;
	packet.append_bytes(packet_hdr, sizeof(packet_hdr));
	packet.append_bytes(&mask, 1);
	packet.append_bytes(packed_GUID, mark);


	const BYTE packet_flags[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // the first one of these seems to be a movement mask (0x1 for moving forward, 0x2 for backward and so on)
	};

	packet.append_bytes(packet_flags, sizeof(packet_flags));

	DWORD ticks = *(DWORD*)CurrentTicks;
	packet.append_bytes(&ticks, sizeof(ticks));

	WowObject player;
	OM.get_local_object(&player);
	vec3 ppos = player.get_pos();

	packet.append_bytes(&ppos.data[0], sizeof(ppos.data));
	packet << angle;

	// no idea what this shit is, but seems to not give a disconnect
	const BYTE packet_end[] = {
		0x39, 0x03, 0x00, 0x00
	};

	packet.append_bytes(packet_end, sizeof(packet_end));

	send_wowpacket(packet.bytes, packet.length);

	PRINT("sent facing packet!\n");

}

static void set_facing_local_and_remote(float angle) {
	static timer_interval_t timer_interval(150);
	static float prev_angle = 0;

	if (!timer_interval.passed()) return;

	if (fabs(prev_angle - angle) < 0.001) {
		// this is just so we don't send too many packets
		return;
	}

	printf("setting facing locally and remotely! (prev: %f, new: %f)\n", prev_angle, angle);
	synthetize_CMSG_SET_FACING(angle);
	SetFacing_local(angle);

	last_CMSG_SET_FACING_sent = std::chrono::system_clock::now();
	prev_angle = angle;

	timer_interval.reset();

}



static int LOP_melee_behind(lua_State *L) {

	float minrange = lua_tonumber(L, 2);

	WowObject p, t;
	if (!OMgr->get_local_object(&p) || !OMgr->get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	minrange = minrange > 2 ? minrange - 2 : minrange; // can't remember what this is for

	vec3 ppos = p.get_pos();
	vec3 prot_unit = p.get_rotvec();

	vec3 tpos = t.get_pos();
	vec3 trot_unit = t.get_rotvec();

	static const float MELEE_NUKE_ANGLE = 0.35;

	const vec3 vs[] = { tpos + minrange * trot_unit.rotated_2d((1.0f - MELEE_NUKE_ANGLE) * M_PI), tpos + minrange * trot_unit.rotated_2d((1.0f + MELEE_NUKE_ANGLE) * M_PI) };
	const vec3 &ideal_position = ((vs[0] - ppos).length() < (vs[1] - ppos).length()) ? vs[0] : vs[1];
	
	vec3 posdiff = ideal_position - ppos;
	posdiff.z = 0;
	
	vec3 tpdiff = tpos - ppos;
	tpdiff.z = 0;
	vec3 tpdiff_unit = tpdiff.unit();

	auto R = get_position_flags_melee(posdiff, tpdiff_unit, prot_unit);

	switch (R) {
	case POSFLAG::TOOFAR: {
		vec3 point_behind_ctm = ideal_position + 0.5 * prot_unit;
		CTM_t act = CTM_t(point_behind_ctm, CTM_MOVE, CTM_PRIO_REPLACE, 0, 0.5);
		ctm_add(act);
		break;
	}
	
	case POSFLAG::WRONG_FACING: {
		float newa = atan2(tpdiff_unit.y, tpdiff_unit.x);
		set_facing_local_and_remote(newa);
		break;
	}

	default:
		break;
	}

	return NO_RVALS;

}

static int LOP_melee_avoid_aoe_buff(lua_State *L) {
	
	int spellID = lua_tointeger(L, 2);

	WowObject p;
	if (!OMgr->get_local_object(&p)) return 0;
	vec3 ppos = p.get_pos();

	for (const auto &o : *OMgr) {

		if (o.get_type() == OBJECT_TYPE_NPC) {

			if (o.NPC_has_buff(spellID) || o.NPC_has_debuff(spellID)) {

				float dist = (o.get_pos() - ppos).length();

				if (dist > 15) {
					break; // this should go to melee_behind
				}
				// LOL XD TODO: Implement
			}
		}
	}

	return NO_RVALS;
}

static void target_unit_with_GUID(GUID_t guid) {

	taint_caller_reseter r; // this sets taint caller to 0 on construction, and resets to whatever was there on destruction :)
	SelectUnit(guid);

	// 0081B530 is the function that gives the taint error message
}

static int LOP_target_GUID(lua_State* L) {
	GUID_t guid = convert_str_to_GUID(lua_tostring(L, 2));
	target_unit_with_GUID(guid);
	return 0;
}

static int LOP_caster_range_check(lua_State *L) {

	lua_Number minrange = lua_tonumber(L, 2);
	lua_Number maxrange = lua_tonumber(L, 3);

	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return 0;

	WowObject p, t;
	if (!OMgr->get_local_object(&p) || !OMgr->get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();
	vec3 tpdiff = tpos - ppos;
	vec3 tpdiff_unit = tpdiff.unit();

	auto R = get_position_flags_ranged(minrange, maxrange, tpdiff, p.get_rotvec());

	//PRINT("got R == %s\n", to_string(R));

	switch (R) {

	case POSFLAG::TOONEAR: {
		vec3 new_point = tpos - (minrange + 2) * tpdiff_unit;
		ctm_add(CTM_t(new_point, CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5));
		break;
	}

	case POSFLAG::TOOFAR: {
		vec3 new_point = tpos - (maxrange - 2) * tpdiff_unit;
		ctm_add(CTM_t(new_point, CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5));
		break;
	}

	case POSFLAG::WRONG_FACING: {
		float newa = atan2(tpdiff_unit.y, tpdiff_unit.x);
		set_facing_local_and_remote(newa);
		break;
	}

	default:
		break;
	}

	return 0;

}

static void followunit(const std::string& targetname) {
	WowObject p, t;
	if (!OMgr->get_local_object(&p)
		|| !OMgr->get_unit_by_name(targetname, &t)) {
		return;
	}

	if (p.get_GUID() == t.get_GUID()) {
		return;
	}

	GUID_t tGUID = t.get_GUID();

	ctm_add(CTM_t(t.get_pos(), CTM_MOVE, CTM_PRIO_FOLLOW, 0, 1.5));

	if ((t.get_pos() - p.get_pos()).length() < 10) {
		PRINT("follow difference < 10! calling WOWAPI FollowUnit()\n");
		DoString("FollowUnit(\"%s\")", t.unit_get_name());
		follow_state.clear();
		ctm_queue_reset();
		return;
	}
	else {
		// close_enough == 1 means the follow attempt either hasn't been started yet or that the char has actually reached its target
		if (follow_state.close_enough) {
			follow_state.start(targetname);
		}
	}


	if (follow_state.timer.get_s() > 10) {
		follow_state.clear();
	}
}

static int LOP_follow(lua_State *L) {
	std::string targetname = lua_tostring(L, 2);
	followunit(targetname);
	return 0;
}

void stopfollow() {

	WowObject p;
	OMgr->get_local_object(&p);

	// TODO: could also just set the CTM action to DONE

	float prot = p.get_rot();
	vec3 rot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);
	ctm_add(CTM_t(p.get_pos() + 0.51*rot_unit, CTM_MOVE, CTM_PRIO_FOLLOW, 0, 1.5));
	follow_state.clear();
}

static int LOP_stopfollow(lua_State *L) {
	stopfollow();
	return NO_RVALS;
}

static int LOP_CTM(lua_State *L) {

	vec3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	int priority = lua_tointeger(L, 5);

	ctm_add(CTM_t(pos, CTM_MOVE, priority, 0, 1.5));

	return NO_RVALS;
}

static int LOP_nop(lua_State *L) {
	return NO_RVALS;
}

static int LOP_hug_spell_object(lua_State *L) {

	int spellID = lua_tointeger(L, 2);

	auto objs = OMgr->get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		PRINT("No objects with spellid %ld\n", spellID);
		ctm_unlock();
		return 0;
	}
	else {
		PRINT("Hugging spellobject with spellID %ld!\n", spellID);
		lua_pushboolean(L, true);
		return 1;
	}

}

static int LOP_avoid_spell_object(lua_State *L) {
	
	int spellID = lua_tointeger(L, 2);
	float radius = lua_tonumber(L, 3);

	auto objs = OMgr->get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		return 0;
	}

	vec3 escape_pos;
	int need_to_escape = 0;

	WowObject p;
	if (!OMgr->get_local_object(&p)) return 0;
	
	vec3 ppos = p.get_pos();

	for (auto &s : objs) {
		vec3 spos = s.DO_get_pos();
		vec3 dist = ppos - spos;
		if (dist.length() < radius) {
			// then we need to run away from it :D
			if (dist.length() < 0.5) { // this avoids a nasty divide by (almost) zero erreur
				escape_pos = spos + (radius + 1.5)*vec3(0, 1, 0);
			}
			else {
				escape_pos = spos + (radius + 1.5)*dist.unit();
			}

			need_to_escape = 1;
		}
	}
	

	if (!need_to_escape) {
		return 0;
	}

	PRINT("ESCAPING spell with id %d at %.0f, %.0f, %.0f\n", spellID, escape_pos.x, escape_pos.y, escape_pos.z);
	ctm_add(CTM_t(escape_pos, CTM_MOVE, CTM_PRIO_EXCLUSIVE, 0, 0.5));

	lua_pushboolean(L, true);
	return 1;

}

static int LOP_spread(lua_State *L) {
	return 0;
}


static void pull_mob(void *mob_GUID) {
	DoString("CastSpellByName(\"Avenger's Shield\")");
}

static void set_target_and_blast(void *noarg) {
	DoString("RunMacroText(\"/lole test_blast_target\")");
}

static int LOP_tank_pull(lua_State *L) {

	WowObject p, t;

	if (!OMgr->get_local_object(&p) 
		|| !OMgr->get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	vec3 ppos = p.get_pos(), tpos = t.get_pos();
	vec3 d = tpos - ppos, dn = d.unit();

	if (d.length() > 30) {
		vec3 newpos = tpos - 29 * dn;
		CTM_t c(newpos, CTM_MOVE, 0, 0, 0.5);
		c.add_posthook(CTM_posthook_t(pull_mob, NULL, 0, 10));
		ctm_add(c);
	}
	else {
		CTM_t c(ppos + dn, CTM_MOVE, 0, 0, 0.5);
		c.add_posthook(CTM_posthook_t(pull_mob, NULL, 0, 10));
		ctm_add(c);
	}

	return NO_RVALS;

}

static const WO_cached *find_most_hurt_within_CH_bounce(const WO_cached *unit, const WO_cached *unit2, const std::vector<WO_cached> &candidates) {

	if (!unit) return NULL;

	const WO_cached *most_hurt = NULL;

//	PRINT("calling CHbounce with unit %s, unit2 %s\n", unit->name.c_str(), unit2 ? unit2->name.c_str() : "NULL");

	for (unsigned i = 0; i < candidates.size(); ++i) {
		const WO_cached *c = &candidates[i];
		if (c->GUID == unit->GUID) { continue; }
		if (unit2 && c->GUID == unit2->GUID) { continue; }

		if ((c->pos - unit->pos).length() < 12.5) {
			if (!most_hurt) {
				most_hurt = c;
			}
			else {
				if (most_hurt->deficit < c->deficit) {
					most_hurt = c;
				}
			}
			//PRINT("new most_hurt = %s\n", most_hurt->name.c_str());
		}
	}
	
	/*if (most_hurt) {
		PRINT("best next CH target for %s is %s with %u/%u HP\n\n", unit->name.c_str(), most_hurt->name.c_str(), most_hurt->health, most_hurt->health_max);
	}
	else {
		PRINT("couldn't find a suitable CH target for %s\n\n", unit->name.c_str());
	}*/

	return most_hurt;

}

struct chain_heal_trio_t {
	const WO_cached *trio[3];
	int total_deficit;
	float weighed_urgency;
};


static std::vector<std::string> LOP_chain_heal_target(const std::string &arg) {

	std::vector<std::string> tokens;
	tokenize_string(arg, ",", tokens);

	std::unordered_map<std::string, uint> target_heals_map;
	char *endptr;
	for (auto &t : tokens) {
		std::vector<std::string> target_heals;
		tokenize_string(t, ":", target_heals);
		uint inc_heals = strtoul(target_heals[1].c_str(), &endptr, 10);
		target_heals_map.insert({target_heals[0], inc_heals});
	}

	ObjectManager OM;
	
	WowObject next;
	if (!OM.get_first_object(&next)) return std::vector<std::string>();

	// cache units for easier access

	std::vector <WO_cached> units;
	uint maxmaxHP = 0;
	while (next.valid()) {
		if (next.get_type() == OBJECT_TYPE_UNIT) {
			uint hp = next.unit_get_health();
			uint hp_max = next.unit_get_health_max();
			if (hp_max > maxmaxHP) {
				maxmaxHP = hp_max;
			}
			int deficit = hp_max - hp;
			std::string name = next.unit_get_name();
			uint inc_heals = (target_heals_map.count(name) > 0 ? target_heals_map[name] : 0);
			PRINT("unit %s with %u/%u hp checked (deficit: %d, incoming heals: %u)\n", name.c_str(), hp, hp_max, deficit, inc_heals);
			if (hp > 0 && deficit - int(inc_heals) > 1500) {
				units.push_back(WO_cached(next.get_GUID(), next.get_pos(), hp, next.unit_get_health_max(), inc_heals, 0.0, name));
			}
		}
		next = next.next();
	}

	std::vector<WO_cached> deficit_candidates;

	for (auto &u : units) {
		u.heal_urgency = pow((((u.health_max - (u.health + u.inc_heals)) / float(u.health_max)) / 0.5), (maxmaxHP / float(u.health_max)));
		deficit_candidates.push_back(u);
		PRINT("candidate %s with %u/%u hp (heal urgency: %f) added\n", u.name.c_str(), u.health, u.health_max, u.heal_urgency);
	}


	chain_heal_trio_t o;

	memset(&o, 0, sizeof(o));
	
	for (unsigned i = 0; i < deficit_candidates.size(); ++i) {
		// scan vicinity for hurt chars within 12.5 yards

		const WO_cached *c = &deficit_candidates[i];

	//	PRINT("trying to find good CH targets for primary target %s...\n", c->name.c_str());
		
		const WO_cached *most_hurt[2];
		memset(most_hurt, 0, sizeof(most_hurt));

		most_hurt[0] = find_most_hurt_within_CH_bounce(c, NULL, deficit_candidates);
		most_hurt[1] = find_most_hurt_within_CH_bounce(most_hurt[0], c, deficit_candidates);

		float u2 = (most_hurt[0] ? most_hurt[0]->heal_urgency : 0.0);
		float u3 = (most_hurt[1] ? most_hurt[1]->heal_urgency : 0.0);

		// static const int CH_BOUNCE_1 = 3500, CH_BOUNCE2 = 2400, CH_BOUNCE3 = 2100;

		/*int eh1 = (c->deficit > CH_BOUNCE_1) ? CH_BOUNCE_1 : (CH_BOUNCE_1 - c->deficit);
		int eh2 = (d2 > 0) ? ((d2 > CH_BOUNCE2) ? CH_BOUNCE2 : (CH_BOUNCE2 - d2)) : 0;
		int eh3 = (d3 > 0) ? ((d3 > CH_BOUNCE3) ? CH_BOUNCE3 : (CH_BOUNCE3 - d3)) : 0;*/

		float weighed_urgency = c->heal_urgency + u2 * 0.5 + u3 * 0.25;
		PRINT("weighed urgency of trio: %f with main target: %s\n", weighed_urgency, c->name.c_str());

		if (weighed_urgency > o.weighed_urgency) {
			o.trio[0] = c;
			o.trio[1] = most_hurt[0];
			o.trio[2] = most_hurt[1];
			//o.total_deficit = total_deficit;
			o.weighed_urgency = weighed_urgency;
		}
		
	}

	if (o.trio[0]) {
		//DoString("TargetUnit(\"%s\")", o.trio[0]->name.c_str());
		PRINT("Chosen target: %s, weighed urgency of trio: %f\n", o.trio[0]->name.c_str(), o.weighed_urgency);
	}
	
	std::vector<std::string> r;

	for (auto &tar : o.trio) {
		if (tar) {
			r.push_back(tar->name);
		}
	}
	
	return r;
}

static int mob_has_debuff(const WowObject &mob, uint debuff_spellID) {
	for (int i = 0; i < 16; ++i) {
		uint spellID = mob.NPC_get_debuff(i);
		if (spellID == 0) return 0;
		if (spellID == debuff_spellID) {
			return 1;
		}
	}
	return 0;
}

static int LOP_tank_face(lua_State *L) {

	WowObject p, t;
	if (!OMgr->get_local_object(&p) || !OMgr->get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	auto mobs = OMgr->get_all_combat_mobs();
	const auto ppos = p.get_pos();
	const auto prot_unit = p.get_rotvec();

	if (mobs.size() == 0) return 0;

	vec3 diffsum(0, 0, 0);
	for (const auto& m : mobs) {
		diffsum = diffsum + (m.get_pos() - ppos);
	}

	diffsum = (1.0f / (float)mobs.size()) * diffsum;

	//PRINT("Average diff vector: %f %f %f (# of mobs: %zu)\n", diffsum.x, diffsum.y, diffsum.z, mobs.size());

	float orient_new = atan2(diffsum.y, diffsum.x);
	set_facing_local_and_remote(orient_new);

	bool need_to_backpedal = std::any_of(mobs.begin(), mobs.end(),
		[&](const WowObject& o) -> bool {
			vec3 tpdiff_unit = (o.get_pos() - ppos).unit();
			float cosa = dot(tpdiff_unit, prot_unit);
			if (cosa < 0.25) {
				PRINT("mob 0x%llX is behind (even after orientation adjustment), need to backpedal\n", o.get_GUID());
				return true;
			}
			return false;
		});
	
	static bool stop_backpedal_lock = false;

	if (need_to_backpedal) {
		DoString("MoveBackwardStart()");
		stop_backpedal_lock = false;
	}
	else {
		// the lock is because in case somebody needs to manually move (especially backpedal) the tank, it won't be stopped by MoveBackwardStop()
		if (!stop_backpedal_lock) DoString("MoveBackwardStop()"); 
		stop_backpedal_lock = true;
	}

	return NO_RVALS;

}

WowObject get_obj_closest_to(const std::vector<WowObject>& objs, const vec3& to) {

	return *std::min_element(objs.begin(), objs.end(),
		[&](const WowObject& a, const WowObject& b) -> bool {
			return (a.get_pos() - to).length() < (b.get_pos() - to).length();
		});

}

static int LOP_interact_spellnpc(lua_State *L) {

	std::string objname = lua_tostring(L, 2);

	WowObject p;
	OMgr->get_local_object(&p);

	auto n = OMgr->get_NPCs_by_name(objname);
	if (n.size() == 0) {
		PRINT("LOP_interact_spellnpc: error: couldn't find NPC with name \"%s\"!\n", objname.c_str());
		return 0;
	}
	
	vec3 ppos = p.get_pos();
	WowObject o = get_obj_closest_to(n, ppos);
	GUID_t oGUID = o.get_GUID();
	float dist = (o.get_pos() - ppos).length();

	if (dist > 5) {
		PRINT("LOP_interact_spellnpc: too far from object %s (0x%llX), dist = %f\n", objname.c_str(), oGUID, dist);
		return 0;
	}

	else {
		BYTE sockbuf[14] = {
			0x00, 0x0C, 0xF8, 0x03, 0x00, 0x00, // 0x3F8 == CMSG_SPELLCLICK
			0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8
		};

		memcpy(sockbuf + 6, &oGUID, sizeof(oGUID));
		send_wowpacket(sockbuf, sizeof(sockbuf));
		//dump_packet(sockbuf, sizeof(sockbuf));

		lua_pushstring(L, convert_GUID_to_str(oGUID));
		return 1;
	}

}


static int LOP_interact_gobject(lua_State *L) {
	
	// for Meeting Stones, the game sends two opcodes:
	// with opcodes 0xB1 (CMSG_GAMEOBJ_USE) and 0x4B1 (CMSG_GAMEOBJ_REPORT_USE)
	// BUT! for Light/Dark essence in twin val'kyr, the object is actually a NPC (type 3) and not a "GAMEOBJECT"
	

	BYTE sockbuf[14] = {
		0x00, 0x0C, 0xB1, 0x00, 0x00, 0x00, 
		0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8
	};

	BYTE sockbuf2[14] = {
	0x00, 0x0C, 0x81, 0x04, 0x00, 0x00,
	0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8
	};

	std::string objname = lua_tostring(L, 2);

	WowObject o;
	if (!OMgr->get_GO_by_name(objname, &o)) {
		PRINT("LOP_interact_gobject: error: couldn't find gobject with name \"%s\"!\n", objname.c_str());
		return 0;
	}


	WowObject p;
	OMgr->get_local_object(&p);

	vec3 diff = o.GO_get_pos() - p.get_pos();
	float dist = diff.length();

	if (dist > 5.0) {
		PRINT("LOP_interact_gobject: too far from object \"%s\" (dist = %.2f)\n", objname.c_str(), dist);
		return 0;
	}

	GUID_t oGUID = o.get_GUID();

	memcpy(sockbuf + 6, &oGUID, sizeof(GUID_t));
	memcpy(sockbuf2 + 6, &oGUID, sizeof(GUID_t));
			
	send_wowpacket(sockbuf, sizeof(sockbuf));
	send_wowpacket(sockbuf2, sizeof(sockbuf2));

	PRINT("LOP_interact_gobject: interacted with object %s (GUID: 0x%llX)!\n", objname.c_str(), oGUID);

	lua_pushboolean(L, true);
	return 1;
}

static int LOP_execute(lua_State *L) {
	std::string script(lua_tostring(L, 2));
	DoString(script);
	return NO_RVALS;
}

static int LOPDBG_loot(lua_State *L) {
	ObjectManager OM;
	WowObject corpse;
	if (!OM.get_object_by_GUID(get_target_GUID(), &corpse)) return 0;
	
	ctm_add(CTM_t(corpse.get_pos(), CTM_LOOT, CTM_PRIO_EXCLUSIVE, corpse.get_GUID(), 0.5));
	return 1;
}

static void LOPDBG_query_injected(const std::string &arg) {
	DoString("SetCVar(\"screenshotQuality\", \"LOLE\", \"inject\")"); // this is used to signal the addon that we're injected :D
}

static int LOPDBG_pull_test() {

	ObjectManager OM;	
	
	WowObject p, t;
	if (!OM.get_local_object(&p) || !OM.get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();
	vec3 diff = tpos - ppos;
	vec3 dir = diff.unit();

	vec3 pull_pos = ppos;

	if (diff.length() > 25) {
		pull_pos = tpos - 23 * dir;
	}

	ctm_add(CTM_t(pull_pos, CTM_MOVE, 0, 0, 0.5));

	CTM_t pull(pull_pos + dir, CTM_MOVE, 0, 0, 0.5);
	pull.add_posthook(CTM_posthook_t(pull_mob, NULL, 0, 30));
	pull.add_posthook(CTM_posthook_t(set_target_and_blast, NULL, 0, 120));

	ctm_add(pull);
}

static int LOP_get_unit_position(lua_State *L) {

	std::string name = lua_tostring(L, 2);

	GUID_t object_GUID = 0;

	if (name.rfind("0x", 0) == 0) {
		// we're dealing with a GUID
		object_GUID = convert_str_to_GUID(name);
	}
	
	else if (name == "player") {
		object_GUID = OMgr->get_local_GUID();
	}

	else if (name == "target") {
		GUID_t target_GUID = get_target_GUID();
		if (!target_GUID) { return 0; }
	
		object_GUID = target_GUID;
	}

	else if (name == "focus") { // useful for blast target
		GUID_t focus_GUID = get_focus_GUID();
		if (!focus_GUID) { return 0; }

		object_GUID = focus_GUID;
	}

	else {
		WowObject u;
		if (!OMgr->get_unit_by_name(name, &u)) { return 0; }
		object_GUID = u.get_GUID();
	}

	vec3 pos;
	double rot;

	WowObject o;

	if (!OMgr->get_object_by_GUID(object_GUID, &o)) { return 0; }

	pos = o.get_pos(); 
	rot = o.get_rot();

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, rot);

	return 4;
}

static int LOP_get_combat_targets(lua_State *L) {

	WowObject p;
	if (!OMgr->get_local_object(&p)) return 0; 
	vec3 ppos = p.get_pos();

	std::vector<std::string> out;
	for (const auto &i : *OMgr) {
	if (i.get_type() == OBJECT_TYPE_NPC) {
			int reaction = get_reaction(p, i);

			if (reaction < 5 && i.in_combat() && !i.NPC_unit_is_dead() && i.NPC_get_health_max() > 2500) {
				//float dist = (i.get_pos() - ppos).length();
				//if (dist < 30) {
				out.push_back(convert_GUID_to_str(i.get_GUID()));
				//}
			}
		}
	}

	for (const auto& o : out) {
		lua_pushstring(L, o);
	}

	return out.size();

}

static void tank_filter_wellbehaving_mobs(std::vector<WowObject>* mobs, const std::vector<GUID_t>& ignore_tanked_by_GUID) {
	GUID_t pGUID = OMgr->get_local_GUID();
	mobs->erase(
		std::remove_if(mobs->begin(), mobs->end(),
		[&](const WowObject& mob) {
				GUID_t mtguid = mob.NPC_get_target_GUID();
				if (std::any_of(ignore_tanked_by_GUID.begin(), ignore_tanked_by_GUID.end(),
					[&](GUID_t guid) {
						return mtguid == guid;
					})) {
					return true;
				}
				else if (mtguid == pGUID) {
					return true;
				}
				else return false;
		}),
		mobs->end());
}

static int LOP_tank_taunt_loose(lua_State *L) {
	auto mobs = OMgr->get_all_combat_mobs();
	if (mobs.size() == 0) return 0;

	std::string tauntspell = lua_tostring(L, 2);

	std::vector<std::string> ignore_tanked_by_GUID;

	if (lua_gettop(L) > 1) {
		ignore_tanked_by_GUID = get_lua_stringtable(L, 3);
	}

	std::vector<GUID_t> ig;
	for (const auto& G : ignore_tanked_by_GUID) {
		ig.push_back(convert_str_to_GUID(G));
	}

	tank_filter_wellbehaving_mobs(&mobs, ig);

	if (mobs.size() > 0) {
		GUID_t tguid = mobs[0].get_GUID();
		std::string tguidstr = convert_GUID_to_str(tguid);
		lua_pushstring(L, tguidstr);
		return 1;
	}

	else return 0;
}

static inline long get_filesize(FILE* fp) {
	long r;
	fseek(fp, 0, SEEK_END);
	r = ftell(fp);
	rewind(fp);
	return r;
}

static int LOP_read_file(lua_State *L) {
	std::string filename = lua_tostring(L, 2);
	
	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp) {
		dual_echo("Couldn't open file %ls\\\"%s\" for reading\n", std::filesystem::current_path().c_str(), filename.c_str());
		return 0;
	}

	long fs = get_filesize(fp);
	char* buf = new char[fs + 1];
	fread(buf, 1, fs, fp);
	buf[fs] = '\0';

	lua_pushlstring(L, buf, fs);

	dual_echo("loaded file \"%s\" (size = %ld)", filename.c_str(), fs);
	delete[] buf;

	return 1;
}


static int LOP_get_aoe_feasibility(lua_State *L) {
	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return 0;

	float radius = lua_tonumber(L, 2);

	WowObject p, t;

	OMgr->get_local_object(&p);

	if (!OMgr->get_object_by_GUID(target_GUID, &t)) {
		return 0;
	}

	const vec3 tpos = t.get_pos();

	float feasibility = 0; // the mob itself will be counted in the loop, so that's an automatic +1

	for (const auto& i : *OMgr) {
		if (i.get_type() == OBJECT_TYPE_NPC) {
			int reaction = get_reaction(p, i);
			if (reaction < 5 && !i.NPC_unit_is_dead() && i.NPC_get_health_max() > 2500) {
				float dist = get_distance2(t, i);
				if (dist < radius) {
					feasibility += -(dist / radius) + 1;
					//PRINT("0x%llX (%s) is in combat, dist: %f, feasibility: %f, reaction: %d\n", i.get_GUID(), i.NPC_get_name().c_str(), dist, feasibility, reaction);
				}
			}
		}
	}

	lua_pushnumber(L, feasibility);
	return 1;
}

static int LOPDBG_test() {
	float angle = 0.0;
	synthetize_CMSG_SET_FACING(angle);
	SetFacing_local(angle);

	return 1;

	//SOCKET s = get_wow_socket_handle();
	//PRINT("send address: %X, socket = %X\n", &send, s);

	//ObjectManager OM;

	//WowObject i;
	//if (!OM.get_first_object(&i)) return 0;
	//while (i.valid()) {
	//	PRINT("address: %X, GUID: %llX, type: %d\n", i.get_base(), i.get_GUID(), i.get_type());

	//	if (i.get_type() == OBJECT_TYPE_UNIT) {
	//		vec3 pos = i.get_pos();
	//		PRINT("name: %s, position: (%f, %f, %f), %f\n", i.unit_get_name().c_str(), pos.x, pos.y, pos.z, i.get_rot());
	//	}
	//	else if (i.get_type() == OBJECT_TYPE_NPC) {
	//		vec3 pos = i.get_pos();
	//		PRINT("name: %s, (%f, %f, %f), %f, 0x%llX\n", i.NPC_get_name().c_str(), pos.x, pos.y, pos.z, i.get_rot(), i.NPC_get_target_GUID());
	//	}

	//	i = i.next();
	//}

	//return 1;
}

static int LOP_has_aggro(lua_State *L) {
	WowObject p;
	OMgr->get_local_object(&p);

	GUID_t pGUID = p.get_GUID();

	for (const auto &o : *OMgr) {
		if (o.get_type() == OBJECT_TYPE_NPC) {
			GUID_t tGUID = o.NPC_get_target_GUID();
			if (pGUID == tGUID) {
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}

	return 0;
}



int LOP_cast_gtaoe(lua_State *L) {
	static timer_interval_t second(1000);
	if (!second.passed()) return 0;

	int spellID = lua_tointeger(L, 2);
	vec3 coords(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	
	BYTE sockbuf[29] = {
		0x00, 0x1B, 0x2E, 0x01, 0x00, 0x00, // HEADER
		0xFF, // CAST COUNT
		0xAA, 0xBB, 0xCC, 0xDD, // SPELLID
		0x00, 0x40, // FLAGS

		0x00, 0x00, 0x00, 0x00,

		0xA1, 0xA2, 0xA3, 0xA4, // x:float	
		0xB1, 0xB2, 0xB3, 0xB4, // y:float
		0xC1, 0xC2, 0xC3, 0xC4	// z:float
	};

	sockbuf[6] = get_spellcast_counter();
	increment_spellcast_counter();

	memcpy(sockbuf + 7, &spellID, sizeof(spellID));
	memcpy(sockbuf + 17, &coords.x, sizeof(float));
	memcpy(sockbuf + 21, &coords.y, sizeof(float));
	memcpy(sockbuf + 25, &coords.z, sizeof(float));

	send_wowpacket(sockbuf, sizeof(sockbuf));

	return NO_RVALS;
}

static void try_wowctm() {
	// 0xD3F78C

	// the [C24954] + 4 is set to 1 in function 5FA170 (it's actually 00200001 if you click a new one before the next)
	// [C24954] + 14 is set to (some value, tick count??) in at 5FA261
	// 5F9600 returning != 0 is the next problem B) (called at 5FA668)

	// 8697E0 is for right click
	// [D41404] is some value that's being incremented up to F and then back to 0

	// 4F4500 is where the coordinates are set to the struct to be passed to CTM_FINAL
	// 527360 is some 


	DWORD something = DEREF(0xD3F78C);
	DWORD something2 = DEREF(0xC24954);

	DWORD ticks = DEREF(0xB499A4) + 1;

	BYTE skiphax[] = {
		0xB8, 0x01, 0x00, 0x00, 0x00
	};

	WriteProcessMemory(glhProcess, (LPVOID)0x5FC689, skiphax, 5, NULL); // skip the lua validity check B)

	int one = 1;
	if (something) {
		DWORD func = 0x5FC680;
		memcpy((LPVOID)(something2 + 4), &one, 4);
		memcpy((LPVOID)(something2 + 0x14), &ticks, 4);
		memcpy((LPVOID)(something2 + 0x18), &one, 4);
		__asm {
			//	int 3;
			push something;
			call func;
			add esp, 4;
		}

	}
}

float randf() {
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}
static int initial_angle_set = 0;

static int LOP_hconfig(lua_State* L) {
	std::vector<std::string> args;
	tokenize_string(lua_tostring(L, 2), " ", args);
	const std::string& a0 = args[0];

	if (a0 == "set") {
		if (args.size() < 2) {
			echo_wow("hconfig set called but no argument specified!");
		}
		else {
			hconfig_set(args[1]);
		}
	}

	else if (a0 == "enable") {
		hotness_enable(true);
	}
	else if (a0 == "disable") {
		hotness_enable(false);
	}

	else if (a0 == "show") {
		aux_show();
	}

	else if (a0 == "hide") {
		aux_hide();
	}

	else if (a0 == "status") {
		static timer_interval_t warning_time(10000);

		if (!hotness_enabled()) {
			if (warning_time.passed()) {
				echo_wow("WARNING: hconfig status called, but hotness not enabled with hconfig enable!");
				warning_time.reset();
			}
			return 0;
		}

		WowObject player;
		OMgr->get_local_object(&player);

		auto m = hotness_status();

		const BYTE HOTNESS_THRESHOLD = 160;
		const BYTE DH_THRESHOLD = 30;
		int dh = abs((int)m.current_hotness - (int)m.best_hotness);

		if (m.current_hotness > HOTNESS_THRESHOLD && dh > DH_THRESHOLD) {
			dual_echo("HOTNESS: %s - best world pos: %.1f, %.1f (best hotness %u, current %u, threshold: %u)?", player.unit_get_name().c_str(), m.best_world_pos.x, m.best_world_pos.y, m.best_hotness, m.current_hotness, HOTNESS_THRESHOLD);
			dual_echo("walking to a better position ^");
			DoString("SpellStopCasting()");
			ctm_add(CTM_t(m.best_world_pos, CTM_MOVE, CTM_PRIO_FOLLOW, 0, 1.5));
		}
	}
	return NO_RVALS;
}

static int LOP_boss_action(lua_State *L) {
	std::vector<std::string> args;
	tokenize_string(lua_tostring(L, 2), " ", args);

	WowObject player;
	OMgr->get_local_object(&player);
	
	if (args[0] == "Gormok_reset") {
		PRINT("Running gormok angle reset\n");

		initial_angle_set = 0;
		return 0;
	}
		
	else if (args[0] == "Gormok") {

		if (ctm_queue_get_top_prio() == CTM_PRIO_NOOVERRIDE) return 0;

		size_t len;
		WowObject P;
		OMgr->get_local_object(&P);
		auto n = OMgr->get_NPCs_by_name("Fire Bomb");
		if (n.size() == 0) {
			return 0;
		}
		else {
			vec3 ppos = P.get_pos();

			int needed = 0;
			for (auto &o : n) {
				if ((o.get_pos() - ppos).length() < 12) {
					needed = 1;
					break;
				}
			}

			if (!needed) return 0;

			WowObject F;
			if (!OMgr->get_object_by_GUID(get_focus_GUID(), &F)) return 0;
			vec3 fpos = F.get_pos();

			static float angle = 0;
			if (!initial_angle_set) {
				vec3 face = (ppos - fpos).unit();
				angle = atan2(face.y, face.x);
				PRINT("starting Gormok avoidance from initial angle %f\n", angle);
				initial_angle_set = 1;
			}


			vec3 newpos = fpos + 25 * vec3(1, 0, 0).rotated_2d(angle);

			angle += 0.10*M_PI;

			ctm_add(CTM_t(newpos, CTM_MOVE, CTM_PRIO_NOOVERRIDE, 0, 1.0));

		}
	}
	else if (args[0] == "Icehowl") {

	}
	
	else {
		PRINT("Unknown boss action %s\n", args[0].c_str());
	}

	return NO_RVALS;
}

static int LOP_iccrocket(lua_State *L) {

	std::string packet_hexstr = lua_tostring(L, 2);

	ObjectManager OM;
	WowObject rocket_pack;

	DoString("RunMacroText(\"/equip Goblin Rocket Pack\")");

	if (!OM.get_item_by_itemID(49278, &rocket_pack)) {
		return 0;
	}

	GUID_t g = rocket_pack.get_GUID();

	// during a legit item cast, the packet is constructed at 46772E (call to 467190)
	// the data is input at 46720B (call to 40CB10)
	// copied at (40CBEE)
	// the actual packet is constructed in 80AC90 (it's LOCAL.7)
	// first mention at 80B0D1
	// the packet address appears in 434F99C after a call to 47B0A0 (at 80B293)

	// the cast count is written to the packet at 80B2B5 (stored in [EDI+24], which is [[D3F4E4] + 24])
	// (D3F4E4 contains used item GUID)
	// EDI + 8 contains the coordinates apparently (written to the packet at 80B48F)
	// X coordinate written at 9AB96C, Y coordinate at 9AB97C, etc
	// the coords are fetched from [ESI+58, 5C, 60]

	// apparently another packet is sent at 467781
	
	// FIXED! The problem was that the legit function actually sends two packets, one for 0xAB and one for 0x3D3 (VOICE_CHAT_ something ?????)
	// If that doesn't happen, the SARC4 encryption gets messed up

#define PACKET_LATER 0xCC
#define PL PACKET_LATER

	BYTE sockbuf[47] = {
		// the size argument is absolute size of packet - 2
		0x00, 0x2D, 0xAB, 0x00, 0x00, 0x00, // 0x0AB == CMSG_USE_ITEM
		0xFF, 0x03, PL, 0x25, 0x0C, 0x01, 0x00, // PACKET_LATER IS THE "CAST COUNT". 
		PL, PL, PL, PL, PL, PL, PL, PL, // THIS IS ITEM GUID
		0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 
		PL, PL, PL, PL, PL, // if the first byte is 0x00, then our jumping destination is not a game object - if 0xC3, we're on a gameobject. In that case the last 4 bytes of this is the shortened GUID of the gameobject on which we're jumping!
		PL, PL, PL, PL, // X
		PL, PL, PL, PL, // Y
		PL, PL, PL, PL // Z
	};


	sockbuf[8] = get_item_usecount();
	increment_item_usecount();

	memcpy(sockbuf + 13, &g, sizeof(g));

	BYTE bytes[17];
	hexstr_to_bytearray(bytes, sizeof(bytes), packet_hexstr.c_str());

	memcpy(&sockbuf[47 - 17], bytes, 17);
	PRINT("sending following iccpacket:\n");
	dump_packet(sockbuf, 47);
	
	send_wowpacket(sockbuf, sizeof(sockbuf));

	// so this is the other packet :D
	// ON WARMANE, A 0x101 IS SENT! (total length 10, content 0 0 0 0)
	//BYTE sockbuf2[11] = {
	//	0x00, 0x09, 0xD3, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00
	//};

	BYTE sockbuf2[10] = {
		0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00
	};

	send_wowpacket(sockbuf2, sizeof(sockbuf2));

	return NO_RVALS;

}

static int LOP_get_walking_state(lua_State *L) {
	// TODO: implement
	return NO_RVALS;
}

static int LOP_get_ctm_state(lua_State *L) {
	// TODO: implement
	return NO_RVALS;
}

static int LOP_get_previous_cast_msg(lua_State *L) {
	// look at previous_cast_msg
	return 0;
}

static int LOP_avoid_npc_with_name(lua_State* L) {
	std::string name = lua_tostring(L, 2);
	lua_Number radius = lua_tonumber(L, 3);
	
	WowObject p;
	OMgr->get_local_object(&p);
	vec3 ppos = p.get_pos();
	auto n = OMgr->get_NPCs_by_name(name);

	for (auto &i : n) {
		if (i.get_type() == OBJECT_TYPE_NPC) {
			if ((ppos - i.get_pos()).length() < radius) {
				PRINT("Should avoid %s with GUID 0x%llX\n", name.c_str(), i.get_GUID());
				return 1;
			}
		}
	}

	return 0;
}

static int LDOP_debug_test(lua_State* L) {
	return NO_RVALS;
}

static int LDOP_eject_dll(lua_State* L) {
	should_unpatch = 1;
	return NO_RVALS;
}

static int LDOP_console_print(lua_State* L) {
	if (lua_gettop(L) < 2) return 0;
	else puts(lua_tostring(L, 2));
	return NO_RVALS;
}

static int LDOP_lua_register(lua_State* L) {
	lua_registered = 1;
	return NO_RVALS;
}

class lopfunc_list {
	std::vector<lop_func_t> funcs;
	std::vector<lop_func_t> debug_funcs;

public:
	const lop_func_t& operator[](LOP op) const {
		if (IS_DEBUG_OPCODE(op)) {
		//	dual_echo("op: 0x%X, LDOP_MASK: 0x%X\n", (int)op, LDOP_MASK);
			int stripped_index = (int)op & (~LDOP_MASK);
			ASSERT(stripped_index, <, debug_funcs.size());
			return debug_funcs[stripped_index];
		}
		else {
			ASSERT((int)op, <, funcs.size());
			return funcs[(int)op];
		}
	}

	lopfunc_list(std::vector<lop_func_t>&& f, std::vector<lop_func_t>&& df)
		: funcs(std::move(f)), debug_funcs(std::move(df))
	{
		dual_echo("lopfunc_list loaded! Have %d funcs, and %d debug funcs", funcs.size(), debug_funcs.size());
		
		// just check that there are no loops in the table :D
		
		int i = 0;
		for (const auto& f : funcs) {
			ASSERT(i, ==, (int)f.opcode);
			++i;
		}

		i = 0;
		for (const auto& f : debug_funcs) {
			ASSERT(i | LDOP_MASK, ==, (int)f.opcode);
			++i;
		}
	}

};


#define OPSTR(OPCODE_ID) OPCODE_ID, #OPCODE_ID

static const lopfunc_list lop_funcs = {
{
{ OPSTR(LOP::NOP), {}, 0, LOP_nop },

{ OPSTR(LOP::TARGET_GUID),
{{"targetGUID", lua_type::string, LUA_ARG_REQUIRED}},
	RVALS_1, LOP_target_GUID },

{ OPSTR(LOP::CASTER_RANGE_CHECK),
{{"minrange", lua_type::number, LUA_ARG_REQUIRED},
 {"maxrange", lua_type::number, LUA_ARG_REQUIRED}},
	RVALS_1, LOP_caster_range_check },

{ OPSTR(LOP::FOLLOW),
{{"unitGUID", lua_type::string, LUA_ARG_REQUIRED}},
	NO_RVALS, LOP_follow },

{ OPSTR(LOP::CTM),
{{"x", lua_type::number, LUA_ARG_REQUIRED},
 {"y", lua_type::number, LUA_ARG_REQUIRED},
 {"z", lua_type::number, LUA_ARG_REQUIRED},
 {"priority", lua_type::integer, LUA_ARG_REQUIRED}},
	NO_RVALS, LOP_CTM },

{ OPSTR(LOP::TARGET_MARKER), {}, NO_RVALS, op_handler_NYI }, // this would actually be NYI

{ OPSTR(LOP::MELEE_BEHIND), 
{{"minrange", lua_type::number, LUA_ARG_REQUIRED}}, 
	NO_RVALS, LOP_melee_behind },

{ OPSTR(LOP::AVOID_SPELL_OBJECT), 
{{"spellID", lua_type::integer, LUA_ARG_REQUIRED},
 {"radius", lua_type::number, LUA_ARG_REQUIRED}}, 
	RVALS_1, LOP_avoid_spell_object },

{ OPSTR(LOP::HUG_SPELL_OBJECT), 
{{"spellID", lua_type::integer, LUA_ARG_REQUIRED}}, 
	RVALS_1, LOP_hug_spell_object },

{ OPSTR(LOP::SPREAD), {}, NO_RVALS, LOP_spread },

{ OPSTR(LOP::CHAIN_HEAL_TARGET), {{"NOT_IN_USE", lua_type::table, LUA_ARG_REQUIRED}}, RVALS_1, op_handler_NYI },

{ OPSTR(LOP::MELEE_AVOID_AOE_BUFF), 
{{"spellID", lua_type::integer, LUA_ARG_REQUIRED}}, 
	NO_RVALS, LOP_melee_avoid_aoe_buff },

{ OPSTR(LOP::TANK_FACE), {}, NO_RVALS, LOP_tank_face },

{ OPSTR(LOP::TANK_PULL), {}, NO_RVALS, LOP_tank_pull },

{ OPSTR(LOP::GET_UNIT_POSITION), 
{{"unitname", lua_type::string, LUA_ARG_REQUIRED}}, 
	RVALS_3, LOP_get_unit_position },

{ OPSTR(LOP::GET_WALKING_STATE), {}, RVALS_1, LOP_get_walking_state },

{ OPSTR(LOP::GET_CTM_STATE), {}, RVALS_1, LOP_get_ctm_state },

{ OPSTR(LOP::GET_PREVIOUS_CAST_MSG), {}, RVALS_1, LOP_get_previous_cast_msg },

{ OPSTR(LOP::STOPFOLLOW), {}, NO_RVALS, LOP_stopfollow },

{ OPSTR(LOP::CAST_GTAOE),
{{"spellID", lua_type::integer, LUA_ARG_REQUIRED},
 {"x", lua_type::number, LUA_ARG_REQUIRED},
 {"y", lua_type::number, LUA_ARG_REQUIRED},
 {"z", lua_type::number, LUA_ARG_REQUIRED}},
	NO_RVALS, LOP_cast_gtaoe },

{ OPSTR(LOP::HAS_AGGRO), {}, RVALS_1, LOP_has_aggro },

{ OPSTR(LOP::INTERACT_GOBJECT),
{{"objname", lua_type::string, LUA_ARG_REQUIRED}},
	RVALS_1, LOP_interact_gobject },

{ OPSTR(LOP::EXECUTE),
{{"objname", lua_type::string, LUA_ARG_REQUIRED}}, 
	NO_RVALS, LOP_execute },

{ OPSTR(LOP::GET_COMBAT_TARGETS), {}, RVALS_1, LOP_get_combat_targets },

{ OPSTR(LOP::GET_AOE_FEASIBILITY),
{{"radius", lua_type::number, LUA_ARG_REQUIRED},
 {"unitname", lua_type::string, LUA_ARG_OPTIONAL}},
	RVALS_1, LOP_get_aoe_feasibility },

{ OPSTR(LOP::AVOID_NPC_WITH_NAME),
{{"npcname", lua_type::string, LUA_ARG_REQUIRED},
 {"radius", lua_type::number, LUA_ARG_REQUIRED}},
	NO_RVALS, LOP_avoid_npc_with_name },

{ OPSTR(LOP::BOSS_ACTION), 
{ {"commandstring", lua_type::string, LUA_ARG_REQUIRED} }, // separated by spaces
	RVALS_1, LOP_boss_action },

{ OPSTR(LOP::INTERACT_SPELLNPC),
{{"npcGUID", lua_type::string, LUA_ARG_REQUIRED}}, 
	RVALS_1, LOP_interact_spellnpc },

{ OPSTR(LOP::GET_LAST_SPELL_ERRMSG), {}, RVALS_3, LOP_get_previous_cast_msg },

{ OPSTR(LOP::ICCROCKET),
{{"packethexstr", lua_type::string, LUA_ARG_REQUIRED}}, 
	NO_RVALS, LOP_iccrocket },

{ OPSTR(LOP::HCONFIG),
{{"commandstring", lua_type::string, LUA_ARG_REQUIRED} }, // separated by spaces
	RVALS_1, LOP_hconfig },

{ OPSTR(LOP::TANK_TAUNT_LOOSE),
{{"tauntspellname", lua_type::string, LUA_ARG_REQUIRED},
 {"ignoretargetedbyGUID", lua_type::table, LUA_ARG_OPTIONAL}}, 
	RVALS_1, LOP_tank_taunt_loose },

{ OPSTR(LOP::READ_FILE),
{{"filename", lua_type::string, LUA_ARG_REQUIRED} }, 
	RVALS_1, LOP_read_file },
},

 {
	 { OPSTR(LOP::LDOP_NOP), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_DUMP), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_LOOT_ALL), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_PULL_TEST), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_LUA_REGISTER), {}, NO_RVALS, LDOP_lua_register },
	 { OPSTR(LOP::LDOP_LOS_TEST), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_TEST), {}, NO_RVALS, LDOP_debug_test },
	 { OPSTR(LOP::LDOP_CAPTURE_FRAME_RENDER_STAGES), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_CONSOLE_PRINT), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_REPORT_CONNECTED), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_EJECT_DLL), {}, NO_RVALS, LDOP_eject_dll },
	 
	 // EXT stuff

	 { OPSTR(LOP::LDOP_SL_RESETCAMERA), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_WC3MODE), {}, NO_RVALS, op_handler_NYI },
	 { OPSTR(LOP::LDOP_SL_SETSELECT), {}, NO_RVALS, op_handler_NYI },

	//	LDOP_NOP = LDOP_MASK,
	//LDOP_DUMP,
	//LDOP_LOOT_ALL,
	//LDOP_PULL_TEST,
	//LDOP_LUA_REGISTER,
	//LDOP_LOS_TEST,
	//LDOP_TEST,
	//LDOP_CAPTURE_FRAME_RENDER_STAGES,
	//LDOP_CONSOLE_PRINT,
	//LDOP_REPORT_CONNECTED,
	//LDOP_EJECT_DLL,

	//// from the old "LOP_EXT"

	//LDOP_SL_RESETCAMERA,
	//LDOP_WC3MODE,
	//LDOP_SL_SETSELECT,
 }

};


static bool check_num_args(LOP opcode, int nargs) {

	if (opcode >= LOP::NUM_OPCODES) return 1;

	const lop_func_t& f = lop_funcs[opcode];
	if (nargs < f.args.size()) { // TODO check types also
		PRINT("error: %s: expected at least %d argument(s), got %d\n", f.opcode_name.c_str(), f.args.size(), nargs);
		return false;
	}

	return true;
}

static bool check_arg_types(lua_State* L, LOP opcode) {
	const lop_func_t& lf = lop_funcs[opcode];
	for (int i = 0; i < lf.args.size(); ++i) {
		const lua_arg& arg = lf.args[i];
		if (arg.type != lua_gettype(L, i + 2)) {
			dual_echo("lop_exec(%s): check_arg_types: wrong type for argument number %d (\"%s\")! Expected %s, got %s\n",
				lf.opcode_name.c_str(), i + 1, arg.name.c_str(), lua_gettypestring(L, arg.type), lua_gettypestr(L, i + 2));
			return false;
		}
	}

	return true;
}

static bool check_lop_args(lua_State* L) {
	int nargs = lua_gettop(L);
	if (nargs < 1) {
		dual_echo("lop_exec: error: no arguments (or nil) passed\n");
		return false;
	}
	LOP opcode = (LOP)lua_tointeger(L, 1);

	//PRINT("opcode: 0x%X\n", (int)opcode);

	if (IS_DEBUG_OPCODE(opcode)) {
		return true; // we don't check these
	}

	if (!check_num_args(opcode, nargs)) {
		return false;
	}

	if (!check_arg_types(L, opcode)) {
		return false;
	}

	return true;
}



int lop_exec(lua_State* L) {

	// NOTE: the return value of this function --> number of values returned to caller in LUA

	if (check_lop_args(L)) {
		LOP op = (LOP)lua_tointeger(L, 1);
		return lop_funcs[op].handler(L);
	}

	//case LDOP_CAPTURE_FRAME_RENDER_STAGES:
	//	enable_capture_render();
	//	break;

	//case LDOP_LUA_REGISTERED:
	//	lua_registered = 1;
	//	break;

	//case LDOP_DUMP: {
	//	const char *typefilter = lua_tolstring(L, 2, &len);
	//	const char *namefilter = lua_tolstring(L, 3, &len);
	//	dump_wowobjects_to_log(typefilter ? typefilter : "", namefilter ? namefilter : "");
	//	break;
	//}

	//case LDOP_TEST: {
	//	break;
	//}
	//case LDOP_NOCLIP: {
	//	ObjectManager OM;
	//	WowObject p;
	//	if (OM.get_object_by_GUID(get_target_GUID(), &p)) {
	//		if (p.get_type() == OBJECT_TYPE_NPC) {
	//			printf("is dead: %d\n", p.NPC_unit_is_dead());
	//		}
	//	}
	//	//enable_noclip();
	//	break;
	//}

	//case LDOP_CONSOLE_PRINT: {
	//	const char* s = lua_tolstring(L, 2, &len);
	//	puts(s);

	//	//FILE *fp = fopen("C:\\Users\\Elias\\Desktop\\lua.log", "a");
	//	//fputs(s, fp);
	//	//fclose(fp);
	//	break;
	//}

	////case LDOP_REPORT_CONNECTED: {
	////	// this is sent by the addon, so if we're getting this, we're most definitely in world
	////	std::string msg = "status " + std::string(lua_tolstring(L, 2, &len));
	////	send_to_governor(msg.c_str(), msg.length() + 1);
	////	in_world = time(NULL);
	////	return 0;
	////}

	//case LDOP_EJECT_DLL: {
	//	should_unpatch = 1;
	//	return 0;
	//}

	//default:
	//	PRINT("lop_exec: unknown opcode %d!\n", opcode);
	//	break;

	//}
			 
	return 0;
}

static std::vector<std::string> lua_rvals;

const std::vector<std::string>& LUA_RVALS() {
	return lua_rvals;
}

int get_rvals(lua_State *L) {
	//get count of returns on stack
	int n = lua_gettop(L);
	lua_rvals = std::vector <std::string>();

	//loop to retreive these
	for (int i = 1; i <= n; i++)
	{
		//using lua_tostring to get the result
		const char *rval = lua_tolstring(L, i, NULL);
		//make sure its valid
		if (rval && rval[0]) {
			lua_rvals.push_back(rval);
		}
	}

	//returning 0 args
	return 0;
}


// follow stuff

void refollow_if_needed() {
	if (!follow_state.close_enough) followunit(follow_state.target_name);
}

static int dump_wowobjects_to_log(const std::string &type_filter, const std::string &name_filter) {

	ObjectManager OM;

	dual_echo("Basic info: ObjectManager base: %X, local GUID = 0x%016llX", OM.get_base_address(), OM.get_local_GUID());
	WowObject o;
	if (!OM.get_first_object(&o)) return 0;

	for (; o.valid(); o = o.next()) {
		uint type = o.get_type();
		if (type == OBJECT_TYPE_CONTAINER) {
			continue;
		}

		else if (type == OBJECT_TYPE_ITEM && (type_filter == "ITEM")) {
			dual_echo("object GUID: 0x%016llX, base addr: 0x%X, type: %s, itemID: %u", o.get_GUID(), o.get_base(), o.get_type_name().c_str(), o.item_get_ID());
		}
		
		else if (type == OBJECT_TYPE_NPC && (type_filter == "" || type_filter == "NPC")) {
			vec3 pos = o.get_pos();
			std::string name = o.NPC_get_name();
			if (name_filter == "" || (name_filter != "" && name.find(name_filter) != std::string::npos)) {
				dual_echo("object GUID: 0x%016llX, base addr = 0x%X, type: %s", o.get_GUID(), o.get_base(), o.get_type_name().c_str());
				dual_echo("coords = (%f, %f, %f), rot: %f", pos.x, pos.y, pos.z, o.get_rot());
				dual_echo("name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d, mounted GUID: 0x%016llX", o.NPC_get_name().c_str(), o.NPC_get_health(), o.NPC_get_health_max(), o.NPC_get_target_GUID(), o.in_combat(), o.NPC_get_mounted_GUID());
				dual_echo("----------------------------------------------------------------------------");
			}
		}
		else if (type == OBJECT_TYPE_UNIT && (type_filter == "" || type_filter == "UNIT")) {
			vec3 pos = o.get_pos();
			std::string name = o.unit_get_name();
			if (name_filter == "" || (name_filter != "" && name.find(name_filter) != std::string::npos)) {
				dual_echo("object GUID: 0x%016llX, base addr = 0x%X, type: %s", o.get_GUID(), o.get_base(), o.get_type_name().c_str());
				dual_echo("coords = (%f, %f, %f), rot: %f", pos.x, pos.y, pos.z, o.get_rot());
				dual_echo("name: %s, health: %u/%u, target GUID: 0x%016llX, combat = %d", o.unit_get_name().c_str(), o.unit_get_health(), o.unit_get_health_max(), o.unit_get_target_GUID(), o.in_combat());
				dual_echo("----------------------------------------------------------------------------");
			}
		}
		else if (type == OBJECT_TYPE_DYNAMICOBJECT && (type_filter == "" || type_filter == "DYNAMICOBJECT")) {
			vec3 DO_pos = o.DO_get_pos();
			dual_echo("object GUID: 0x%016llX, base addr = 0x%X, type: %s", o.get_GUID(), o.get_base(), o.get_type_name().c_str());
			dual_echo("position: (%.1f, %.1f, %.1f), spellID: %d", DO_pos.x, DO_pos.y, DO_pos.z, o.DO_get_spellID());
			dual_echo("----------------------------------------------------------------------------");
		}
		else if (type == OBJECT_TYPE_GAMEOBJECT && (type_filter == "" || type_filter == "GAMEOBJECT")) {
			vec3 GO_pos = o.GO_get_pos();
			dual_echo("object GUID: 0x%016llX, base addr = 0x%X, type: %s", o.get_GUID(), o.get_base(), o.get_type_name().c_str());
			dual_echo("name: %s, position: (%.2f, %.2f, %.2f)", o.GO_get_name().c_str(), GO_pos.x, GO_pos.y, GO_pos.z);
			dual_echo("----------------------------------------------------------------------------");
		}


	}
	
	//SelectUnit(target_GUID); // this is because the *_get_[de]buff calls call SelectUnit

	return 1;
}
