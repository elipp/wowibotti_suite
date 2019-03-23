#include "defs.h"

#include <Windows.h>
#include <Shlobj.h> // for the function that gets the desktop directory path for current user
#include <WinSock2.h>

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
#include "wc3mode.h"
#include "lua.h"

extern HWND wow_hWnd;
Timer since_noclip;
int noclip_enabled;

static int dump_wowobjects_to_log();

struct cast_msg_t previous_cast_msg;

struct lop_func_t {
	std::string opcode_name;
	int min_arguments;
	int max_arguments;
	int num_return_values;
};

#define LOPFUNC(OPCODE_ID, MINARGS, MAXARGS, NUM_RETURN_VALUES) { #OPCODE_ID, MINARGS, MAXARGS, NUM_RETURN_VALUES }

enum {
	ARG_TYPE_INT,
	ARG_TYPE_NUMBER,
	ARG_TYPE_STRING
};

static lop_func_t lop_funcs[] = {

	 LOPFUNC(LOP_NOP, 0, 0, 0),
	 LOPFUNC(LOP_TARGET_GUID, 1, 1, 0),
	 LOPFUNC(LOP_CASTER_RANGE_CHECK, 2, 2, 0),
	 LOPFUNC(LOP_FOLLOW, 1, 1, 0),
	 LOPFUNC(LOP_CTM, 4, 4, 0),
	 LOPFUNC(LOP_DUNGEON_SCRIPT, 1, 2, 0),
	 LOPFUNC(LOP_TARGET_MARKER, 1, 1, 0),
	 LOPFUNC(LOP_MELEE_BEHIND, 0, 0, 0),
	 LOPFUNC(LOP_AVOID_SPELL_OBJECT, 1, 2, 0),
	 LOPFUNC(LOP_HUG_SPELL_OBJECT, 1, 1, 0),
	 LOPFUNC(LOP_SPREAD, 0, 0, 0),
	 LOPFUNC(LOP_CHAIN_HEAL_TARGET, 1, 1, 1),
	 LOPFUNC(LOP_MELEE_AVOID_AOE_BUFF, 1, 1, 0),
	 LOPFUNC(LOP_TANK_FACE, 0, 0, 0),
	 LOPFUNC(LOP_WALK_TO_PULLING_RANGE, 0, 0, 0),
	 LOPFUNC(LOP_GET_UNIT_POSITION, 1, 1, 3),
	 LOPFUNC(LOP_GET_WALKING_STATE, 0, 0, 1),
	 LOPFUNC(LOP_GET_CTM_STATE, 0, 0, 1),
	 LOPFUNC(LOP_GET_PREVIOUS_CAST_MSG, 0, 0, 1),
	 LOPFUNC(LOP_STOPFOLLOW, 0, 0, 0),
	 LOPFUNC(LOP_CAST_GTAOE, 4, 4, 0),
	 LOPFUNC(LOP_HAS_AGGRO, 0, 0, 1),
	 LOPFUNC(LOP_INTERACT_GOBJECT, 1, 1, 1),
	 LOPFUNC(LOP_GET_BISCUITS, 0, 0, 0),
	 LOPFUNC(LOP_LOOT_BADGE, 1, 1, 0),
	 LOPFUNC(LOP_LUA_UNLOCK, 0, 0, 0),
	 LOPFUNC(LOP_LUA_LOCK, 0, 0, 0),
	 LOPFUNC(LOP_EXECUTE, 1, 1, 0),
	 LOPFUNC(LOP_FOCUS, 1, 1, 0),
	 LOPFUNC(LOP_CAST_SPELL, 2, 2, 0),
	 LOPFUNC(LOP_GET_COMBAT_TARGETS, 0, 0, 0),
	 LOPFUNC(LOP_GET_AOE_FEASIBILITY, 1, 1, 1),
	 LOPFUNC(LOP_AVOID_NPC_WITH_NAME, 1, 1, 0),
};


static int LOP_lua_unlock() {

	static BYTE LUA_prot_patch[] = {
		0xB8, 0x01, 0x00, 0x00, 0x00, // MOV EAX, 1
		0xC3						  // RET
	};

	WriteProcessMemory(glhProcess, (LPVOID)LUA_prot, LUA_prot_patch, 6, NULL);

	//set_taint_caller_zero();

	return 1;
}

static int LOP_lua_lock() {
	static const BYTE LUA_prot_original[] = {
		// we're just making an early exit, so nevermind opcode boundaries
		0x55,
		0x8B, 0xEC,
		0x83, 0x3D, 0x9C // this one is cut in the middle
	};

	WriteProcessMemory(glhProcess, (LPVOID)LUA_prot, LUA_prot_original, 6, NULL);

	//reset_taint_caller();

	return 1;
}

static int LOP_execute(const std::string &arg) {
	// this actually seems unnecessary

	//LOP_lua_unlock();
	DoString("%s", arg.c_str());
	//LOP_lua_lock();

	return 1;
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


	// COMMENTARY ON THE NOW REMOVED LOP_FACE
	// less than 0.5 is good for just changing orientation (without walking). see click_to_move()

	// The SetFacing function is effective on the local client level, 
	// as it seems like the server still thinks the character is facing 
	// the way it was before the call to SetFacing. So use CTM magic.

	// this formula is for a directed angle (clockwise angle). atan2(det, dot). 
	// now we're taking the angle with respect to the x axis (north in wow), so the computation becomes simply:

	//vec3 diff = p.get_pos() - t.get_pos(); // OK, THE FOLLOWING CODE WORKS WITH PLAYER - TARGET,
	//float directed_angle = atan2(diff.y, diff.x);

	//printf("player coords: (%f, %f, %f), target coords: (%f, %f, %f), diff = (%f, %f, %f)\nangle = %f, player rot = %f\n", 
	//pc.x, pc.y, pc.z, tc.x, tc.y, tc.z, diff.x, diff.y, diff.z, directed_angle, p.get_rot());

	// the wow angle system seems to be counter-clockwise

	//set_facing(directed_angle + M_PI);

static void face_posthook(void *a) {
	float angle = *(float*)a;
	PRINT("Executed face_posthook (angle: %f)\n", angle);
	ctm_face_angle(angle);
	LOP_execute("RunMacroText(\"/startattack\")");
}

static int LOP_melee_behind() {
	
	ObjectManager OM;

	WowObject p, t;
	if (!OM.get_local_object(&p) || !OM.get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}


	// basically rotating the vector (1, 0, 0) target_rot radians anti-clockwise
	//vec3 rot_unit(1.0 * std::cos(target_rot) - 0.0 * std::sin(target_rot), 1.0 * std::sin(target_rot) + 0.0 * std::cos(target_rot), 0.0);

	vec3 ppos = p.get_pos();
	float prot = p.get_rot();

	vec3 tpos = t.get_pos();
	
	// ok, so turns out the get_rot() variable isn't really kept up-to-date in the OM,
	// so the actual rotation will need to be figured out by means of looking at who the mob is
	// targeting, and looking at the difference vector (since the mob is always directly facing the target)

	GUID_t tot_GUID = t.NPC_get_target_GUID();
	float target_rot;

	if (tot_GUID) {
		if (tot_GUID == OM.get_local_GUID()) {
			vec3 face = (tpos - ppos).unit();
			float newa = atan2(face.y, face.x);
			PRINT("prot: %f, target_rot: %f\n", prot, newa);
			ctm_add(CTM_t::construct_CTM_face(CTM_PRIO_REPLACE, newa));
			return 1;
		}
		WowObject tot;
		OM.get_object_by_GUID(tot_GUID, &tot);
		vec3 trot_diff = tot.get_pos() - tpos;
		target_rot = atan2(trot_diff.y, trot_diff.x);
	}
	else {
		// it's kinda funny this is the fallback tbh :DD
		target_rot = t.get_rot();
	}

	vec3 trot_unit = vec3(std::cos(target_rot), std::sin(target_rot), 0.0);

	vec3 prot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);

	//vec3 point_behind(tc.x - std::cos(target_rot), tc.y - std::sin(target_rot), tc.z);
	vec3 point_behind_actual = tpos + 3 * trot_unit.rotated_2d(0.8*M_PI); // this should put us into a nice "dragon slaying position" :D
	vec3 point_behind_ctm = point_behind_actual + 0.5*prot_unit;
	vec3 diff = point_behind_ctm - ppos;
	diff.z = 0;
	float dl = diff.length();
	if (dl > 1) {
		CTM_t act = CTM_t(point_behind_ctm, CTM_MOVE, CTM_PRIO_REPLACE, 0, 0.5);
		vec3 face = (tpos - ppos).unit();
		float newa = atan2(face.y, face.x);
		act.add_posthook(CTM_posthook_t(face_posthook, &newa, sizeof(newa), 10));
		act.add_posthook(CTM_posthook_t(face_posthook, &newa, sizeof(newa), 40));
		act.add_posthook(CTM_posthook_t(face_posthook, &newa, sizeof(newa), 70));

		ctm_add(act);
		
		return 1;
	}
	else {
	
		// dot product of normalized vectors gives the cosine of the angle between them,
		// and for auto-attacking, the valid sector is actually rather small, unlike spells,
		// for which perfectly perpendicular is ok
		//vec3 face = (tpos - ppos).unit();
		////ctm_add(CTM_t(ppos + face, CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5)); // TODO: CHANGE THIS TO FACE_ANGLE!!!
		//float newa = atan2(face.y, face.x);
		//PRINT("prot: %f, target_rot: %f\n", prot, newa);
		//ctm_add(CTM_t::construct_CTM_face(CTM_PRIO_REPLACE, newa));
		
	}

	return 1;

}

static int LOP_melee_avoid_aoe_buff(long spellID) {
	
	ObjectManager OM;

	WowObject o;
	if (!OM.get_first_object(&o)) return 0;

	while (o.valid()) {

		if (o.get_type() == OBJECT_TYPE_NPC) {

			if (o.NPC_has_buff(spellID) || o.NPC_has_debuff(spellID)) {

				WowObject p;
				
				if (!OM.get_local_object(&p)) return 0;

				float dist = (o.get_pos() - p.get_pos()).length();

				if (dist > 15) {
					break; // this should go to melee_behind
				}

				WowObject Ceucho;
				if (!OM.get_unit_by_name("Ceucho", &Ceucho)) return 0;

				vec3 diff_unit = (Ceucho.get_pos() - o.get_pos()).unit();
				ctm_add(CTM_t(o.get_pos() + 15 * diff_unit, CTM_MOVE, CTM_PRIO_EXCLUSIVE, 0, 0.5));
				return 1;
			}
		}
		o = o.next();
	}

	return 0;
}

 void settarget_GUID(GUID_t GUID) {

	static GUID_t * const GUID_addr1 = (GUID_t*)0xBD07B0;
	static GUID_t * const GUID_addr2 = (GUID_t*)0xBD07C0;

	//*GUID_addr1 = GUID;
	//*GUID_addr2 = GUID;

	set_taint_caller_zero();
	SelectUnit(GUID);
	reset_taint_caller();

	// 0081B530 is the function that gives the taint error message
}

static int LOP_target_GUID(const std::string &arg) {
	GUID_t GUID = convert_str_to_GUID(arg);
	settarget_GUID(GUID);
	return 1;
}

static int LOP_focus(const std::string &arg) {
	GUID_t GUID = convert_str_to_GUID(arg);

	static GUID_t * const FOCUS_GUID = (GUID_t*)0xBD07D0;

	*FOCUS_GUID = GUID;

	//ObjectManager OM;
	//WowObject f;
	//if (OM.get_object_by_GUID(GUID, &f)) return 0;
	//const std::string name;

	//if (f.get_type() == OBJECT_TYPE_NPC) {

	//}

	//esscript_add("FocusUnit(\"Printf\")");

	return 1;

}

static int LOP_range_check(double minrange, double maxrange) {
	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return 0;

	ObjectManager OM;

	WowObject p, t;
	if (!OM.get_local_object(&p) || !OM.get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();

	vec3 diff = tpos - ppos;

	if (diff.length() > maxrange - 1) {
		// move in a straight line to a distance of minrange-1 yd from the target. Kinda bug-prone though..
		vec3 new_point = tpos - (maxrange - 1) * diff.unit();
		ctm_add(CTM_t(new_point, CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5));
		return 1;

	}
	else if (diff.length() < minrange) {
		// move slightly away from the mob
		vec3 new_point = tpos - (minrange + 3) * diff.unit();
		ctm_add(CTM_t(new_point, CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5));
		return 1;
	}

	else {

		// change facing to face the mob

		// a continuous kind of facing function could also be considered (ie. just always face directly towards the mob)

		float rot = p.get_rot();
		vec3 rot_unit = vec3(std::cos(rot), std::sin(rot), 0.0);
		float d = dot(diff, rot_unit);

	//	PRINT("dot product: %f\n", d);

		if (d < 0) {
			ctm_add(CTM_t(ppos + diff.unit(), CTM_MOVE, CTM_PRIO_REPLACE, 0, 1.5)); // this seems quite stable for just changing orientation.
			//ctm_face_target();
		}
	}

	return 1;

}

static int LOP_follow_unit(const std::string& targetname) {

	ObjectManager OM;

	WowObject p, t;
	if (!OM.get_local_object(&p) 
		|| !OM.get_unit_by_name(targetname, &t)) {
		return 0;
	}

	if (p.get_GUID() == t.get_GUID()) {
		return 1;
	}

	GUID_t tGUID = t.get_GUID();
	
	ctm_add(CTM_t(t.get_pos(), CTM_MOVE, CTM_PRIO_FOLLOW, 0, 1.5));

	if ((t.get_pos() - p.get_pos()).length() < 10) {
		PRINT("follow difference < 10! calling WOWAPI FollowUnit()\n");
		LOP_execute("FollowUnit(\"" + t.unit_get_name() + "\")"); // not even protected :)
		follow_state.clear();
		ctm_queue_reset();
		return 1;
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

	return 1;

}

void stopfollow() {
	ObjectManager OM;

	WowObject p;
	OM.get_local_object(&p);

	float prot = p.get_rot();
	vec3 rot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);
	ctm_add(CTM_t(p.get_pos() + 0.51*rot_unit, CTM_MOVE, CTM_PRIO_FOLLOW, 0, 1.5));
	follow_state.clear();
}

static int LOP_stopfollow() {
	stopfollow();
	return 1;
}

static void LOP_CTM_act(double x, double y, double z, int priority) {
	ctm_add(CTM_t(vec3(x, y, z), CTM_MOVE, priority, 0, 1.5));
}

static void LOP_nop(const std::string& arg) {
	return;
}

static void LOP_dungeon_script(const std::string &command, const std::string &arg) {

	if (command == "load") {
		dscript_load(arg);
	}
	else if (command == "run") {
		dscript_run();
	}
	else if (command == "stop") {
		dscript_unload();
	}
	else if (command == "next") {
		dscript_next();
	}
	else if (command == "state") {
		dscript_state(arg);
	}
}

static int LOP_target_marker(const std::string &arg) {
	// arg = marker name
	GUID_t GUID = get_raid_target_GUID(arg);

	if (GUID == 0) {
		DoString("ClearTarget()");
		return 0;
	}

	else {
		SelectUnit(GUID);
		return 1;
	}
}

static int LOP_hug_spell_object(long spellID) {

	ObjectManager OM;

	auto objs = OM.get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		PRINT("No objects with spellid %ld\n", spellID);
		ctm_unlock();
		return 0;
	}
	else {
		PRINT("Hugging spellobject with spellID %ld!\n", spellID);
		return 1;
	}

}

static int LOP_avoid_spell_object(long spellID, float radius) {
		
	ObjectManager OM;

	auto objs = OM.get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		return 0;
	}

	vec3 escape_pos;
	int need_to_escape = 0;

	WowObject p;
	if (!OM.get_local_object(&p)) return 0;
	
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

	return 1;

}

static int LOP_spread() {
	return 1;
}


static void pull_mob(void *mob_GUID) {
	DoString("CastSpellByName(\"Avenger's Shield\")");
}

static void set_target_and_blast(void *noarg) {
	DoString("RunMacroText(\"/lole test_blast_target\")");
}

static int LOP_walk_to_pull() {

	ObjectManager OM;
	WowObject p, t;

	if (!OM.get_local_object(&p) 
		|| !OM.get_object_by_GUID(get_target_GUID(), &t)) {
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

	return 1;

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
		if (spellID == debuff_spellID) {
			return 1;
		}
	}
	return 0;
}

static int LOP_tank_face() {
	ObjectManager OM;

	WowObject p, t;
	if (!OM.get_local_object(&p) || !OM.get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}

	if (t.get_GUID() == OM.get_local_GUID()) return 1;

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();
	vec3 diff = tpos - ppos;
	vec3 dir = diff.unit();

	CTM_t face(ppos + dir, CTM_MOVE, 0, 0, 1.5);
	ctm_add(face);
}

static int LOP_interact_object(const std::string &objname) {
	BYTE sockbuf[14] = {
		0x00, 0x0C, 0xB1, 0x00, 0x00, 0x00,
		0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8
	};

	ObjectManager OM;
	WowObject o;
	if (!OM.get_GO_by_name(objname, &o)) {
		PRINT("LOP_interact_object: error: couldn't find gobject with name \"%s\"!\n", objname.c_str());
		return 0;
	}

	WowObject p;
	OM.get_local_object(&p);

	vec3 diff = o.GO_get_pos() - p.get_pos();
	float dist = diff.length();

	if (dist > 2.5) {
		PRINT("LOP_interact_object: too far from object \"%s\" (dist = %.2f)\n", objname.c_str(), dist);
		return 0;
	}

	GUID_t oGUID = o.get_GUID();
	memcpy(sockbuf + 6, &oGUID, sizeof(GUID_t));
	
	encrypt_packet_header(sockbuf);

	SOCKET s = get_wow_socket_handle();
	send(s, (const char*)sockbuf, sizeof(sockbuf), 0);

}

static void LOPDBG_dump(const std::string &arg) {
	dump_wowobjects_to_log();
}

static int LOPDBG_loot(const std::string &arg) {
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

static int LOP_get_unit_position(const std::string &name, vec3 *pos_out, double *rot) {
	ObjectManager OM;
	
	if (name == "player") {
		WowObject p;
		if (!OM.get_local_object(&p)) return 0;

		*pos_out = p.get_pos();
		*rot = p.get_rot();

		return 1;
	}
	else if (name == "target") {
		WowObject t;
		GUID_t target_GUID = get_target_GUID();

		if (!target_GUID) { return 0; }

		if (!OM.get_object_by_GUID(target_GUID, &t)) {
			return 0;
		}
		
		*pos_out = t.get_pos();
		*rot = t.get_rot();

		return 1;
	}
	else {
		WowObject u;
		if (!OM.get_unit_by_name(name, &u)) { return 0; }

		*pos_out = u.get_pos();
		*rot = u.get_rot();
		
		return 1;
		
	}

	return 0;

}

static int LOP_get_combat_targets(std::vector <GUID_t> *out) {
	ObjectManager OM;

	WowObject p;
	if (!OM.get_local_object(&p)) return 0; 
	vec3 ppos = p.get_pos();

	WowObject i;
	if (!OM.get_first_object(&i)) return 0;

	while (i.valid()) {
		if (i.get_type() == OBJECT_TYPE_NPC) {
			int reaction = get_reaction(p, i);

			if (reaction < 5 && i.in_combat() && !i.NPC_unit_is_dead() && i.NPC_get_health_max() > 2500) {
				//float dist = (i.get_pos() - ppos).length();
				//if (dist < 30) {
				out->push_back(i.get_GUID());
				//}
			}
			
		}

		i = i.next();
	}

	return out->size();

}

static float LOP_get_aoe_feasibility(float threshold) {
	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return -1;

	ObjectManager OM;
	WowObject p, t, i;

	OM.get_local_object(&p);

	if (!OM.get_object_by_GUID(target_GUID, &t)) {
		return -1;
	}
	vec3 tpos = t.get_pos();

	if (!OM.get_first_object(&i)) {
		return -1;
	}

	float feasibility = 0; // the mob itself will be counted in the loop, so that's an automatic +1

	while (i.valid()) {
		if (i.get_type() == OBJECT_TYPE_NPC) {

			//settarget_GUID(i.get_GUID());
			//auto R = dostring_getrvals("UnitReaction(\"player\", \"target\")");
			//int reaction = std::stoi(R[0]);
			int reaction = get_reaction(p, i);

			if (reaction < 5 && !i.NPC_unit_is_dead() && i.NPC_get_health_max() > 2500) {
				float dist = get_distance2(t, i);
				if (dist < threshold) {
					feasibility += -(dist / threshold) + 1;
					PRINT("0x%llX (%s) is in combat, dist: %f, feasibility: %f, reaction: %d\n", i.get_GUID(), i.NPC_get_name().c_str(), dist, feasibility, reaction);
				}
			}

		}
		i = i.next();
	}

	PRINT("\n");

	return feasibility;
}

static int LOPDBG_test() {

	SOCKET s = get_wow_socket_handle();
	PRINT("send address: %X, socket = %X\n", &send, s);

	ObjectManager OM;

	WowObject i;
	if (!OM.get_first_object(&i)) return 0;
	while (i.valid()) {
		PRINT("address: %X, GUID: %llX, type: %d\n", i.get_base(), i.get_GUID(), i.get_type());

		if (i.get_type() == OBJECT_TYPE_UNIT) {
			vec3 pos = i.get_pos();
			PRINT("name: %s, position: (%f, %f, %f), %f\n", i.unit_get_name().c_str(), pos.x, pos.y, pos.z, i.get_rot());
		}
		else if (i.get_type() == OBJECT_TYPE_NPC) {
			vec3 pos = i.get_pos();
			PRINT("name: %s, (%f, %f, %f), %f, 0x%llX\n", i.NPC_get_name().c_str(), pos.x, pos.y, pos.z, i.get_rot(), i.NPC_get_target_GUID());
		}

		i = i.next();
	}

	return 1;
}

static int LOPDBG_capture_frame_render_stages() {

}

static int have_aggro() {
	ObjectManager OM;

	WowObject p;
	OM.get_local_object(&p);

	GUID_t pGUID = p.get_GUID();

	WowObject o;
	if (!OM.get_first_object(&o)) return -1;

	while (o.valid()) {
		if (o.get_type() == OBJECT_TYPE_NPC) {
			GUID_t tGUID = o.NPC_get_target_GUID();
			if (pGUID == tGUID) return 1;
		}
		o = o.next();
	}

	return 0;
}

static int check_num_args(int opcode, int nargs) {

	if (opcode >= LOP_NUM_OPCODES) return 1;

	lop_func_t &f = lop_funcs[opcode];
	if (nargs < f.min_arguments || nargs > f.max_arguments) {
		PRINT("error: %s: expected %d to %d argument(s), got %d\n", f.opcode_name.c_str(), f.min_arguments, f.max_arguments, nargs);
		return 0;
	}

	return 1;
}

static const DWORD noclip_dgo = 0x006A4B6E;
static const DWORD noclip_go = 0x006A49FE;

void enable_noclip() {
	static DWORD noclip_enabled_dgo = 0x968B1DEB;
	static DWORD noclip_enabled_go = 0x0000B4E9;
	writeAddr(noclip_dgo, &noclip_enabled_dgo, sizeof(DWORD));
	writeAddr(noclip_go, &noclip_enabled_go, sizeof(DWORD));
	since_noclip.start();
	noclip_enabled = 1;
}

void disable_noclip() {
	static DWORD noclip_disabled_dgo = 0x968B1D74;
	static DWORD noclip_disabled_go = 0x00B3840F;
	writeAddr(noclip_dgo, &noclip_disabled_dgo, sizeof(DWORD));
	writeAddr(noclip_go, &noclip_disabled_go, sizeof(DWORD));
	noclip_enabled = 0;
}

static void dump_packet(BYTE *packet, size_t len) {
	for (int i = 0; i < len; ++i) {
		PRINT("%02X ", packet[i]);
	}

	PRINT("\n");
}

void LOP_cast_gtaoe(DWORD spellID, const vec3 &coords) {
	
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

	dump_packet(sockbuf, sizeof(sockbuf));

	encrypt_packet_header(sockbuf);

	dump_packet(sockbuf, sizeof(sockbuf));


	SOCKET s = get_wow_socket_handle();
	send(s, (const char*)sockbuf, sizeof(sockbuf), 0);
}

static void LOP_cast_spell(DWORD spellID, GUID_t g) {
	BYTE sockbuf[] = {
		0x00, 0x00, // packet length
		0x2E, 0x01, 0x00, 0x00, // opcode
		0xFF, // CAST COUNT
		0xAA, 0xBB, 0xCC, 0xDD, // SPELLID
		0x00, 0x02,
		0x00, 0x00, 0x00,
		0xFF, // guid mask
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF // PACKED GUID of TARGET (max len 8 ofc)
	};

	sockbuf[6] = get_spellcast_counter();
	increment_spellcast_counter();

	BYTE gpack[8+1]; // the first byte is for the mask :P
	memset(gpack, 0, 9);
	
	int gi = 0;
	for (int i = 0; i < 8; ++i) {
		if (g & 0xFF) {
			gpack[0] |= BYTE(1 << i);
			gpack[gi+1] = g & 0xFF;
			++gi;
		}
		g >>= 8;
	}
	
	PRINT("mask: %02X, target GUID (packed): ", gpack[0]);
	for (int i = 1; i < gi+1; ++i) {
		PRINT("%02X ", gpack[i]);
	}

	BYTE packet_len = 16 + (gi + 1) - 2; // -2 for the length bytes themselves O_O
	sockbuf[1] = packet_len;

	memcpy(sockbuf + 7, &spellID, 4);
	memcpy(sockbuf + 16, gpack, gi + 1);

	PRINT("\n");

	dump_packet(sockbuf, packet_len + 2);
	
	encrypt_packet_header(sockbuf);
	dump_packet(sockbuf, sizeof(sockbuf));

	SOCKET s = get_wow_socket_handle();
	send(s, (const char*)sockbuf, packet_len + 2, 0);
}

static void get_biscuits(void *noarg) {
	LOP_interact_object("Refreshment Table");
}

int LOP_get_biscuits() {
	ObjectManager OM;

	WowObject t;
	if (!OM.get_GO_by_name("Refreshment Table", &t)) return 0;
	
	CTM_t b = CTM_t(t.GO_get_pos(), CTM_MOVE, CTM_PRIO_REPLACE, 0, 0.5);
	b.add_posthook(CTM_posthook_t(get_biscuits, NULL, 0, 100));
	
	ctm_add(b);
}

int LOP_loot_badge(const std::string &GUID_str) {



	GUID_t corpse_GUID = convert_str_to_GUID(GUID_str);
	if (corpse_GUID == 0) return 0;

	ObjectManager OM;
	WowObject p, c;
	if (!OM.get_object_by_GUID(corpse_GUID, &c) || !OM.get_local_object(&p)) return 0;
	
	// TODO: fix the following detour code =D fails because the new dot product checking system in char_is_moving() messes up
	// really tight direction changes :P

	//vec3 diff = p.get_pos() - c.get_pos();

	//if (diff.length() < 2) {
	//	// the wow CTM-loot doesn't work if we're too close, need to make a small detour first =)
	//	ctm_add(CTM_t(p.get_pos() + vec3(6, -6, 0), CTM_MOVE, CTM_PRIO_LOW, 0, 1.5));
	//}

	ctm_add(CTM_t(c.get_pos(), CTM_LOOT, CTM_PRIO_LOW, corpse_GUID, 1.5));

}

static int LOPSL_reset_camera() {
	PRINT("resetting camera\n");
	reset_camera();

	return 1;
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

int lop_exec(lua_State *L) {

	// NOTE: the return value of this function --> number of values returned to caller in LUA

	int nargs = lua_gettop(L);

	if (nargs < 1) {
		PRINT("lop_exec: no arguments; doing nothing!\n");
		return 0;
	}

	int opcode = lua_tointeger(L, 1);
	
	//if (opcode < LOP_NUM_OPCODES) {
	//	PRINT("lop_exec: opcode = %d (%s)\n", opcode, lop_funcs[opcode].opcode_name.c_str());
	//}
	
	//for (int i = 2; i <= nargs; ++i) {
	//	size_t len;
	//	const char* str = lua_tolstring(L, i, &len);

	//	if (!str) {
	//		PRINT("lua_tolstring for argument #%d failed (tables aren't allowed!\n");
	//	}

	//	PRINT("arg %d: \"%s\"\n", i - 1, str);
	//}


	if (!check_num_args(opcode, nargs - 1)) { return 0; }

	size_t len;

	switch (opcode) {
	case LOP_NOP:
		break;

	case LOP_LUA_UNLOCK:
		//LOP_lua_unlock();
		break;

	case LOP_LUA_LOCK:
		// this is now deprecated =D
		//LOP_lua_lock();
		break;
	
	case LOP_EXECUTE:
		LOP_execute(lua_tolstring(L, 2, &len));
		break;

	case LOP_TARGET_GUID:
		LOP_target_GUID(lua_tolstring(L, 2, &len));
		break;

	case LOP_FOCUS:
		LOP_focus(lua_tolstring(L, 2, &len));
		break;

	case LOP_CASTER_RANGE_CHECK: {
		double minrange = lua_tonumber(L, 2);
		double maxrange = lua_tonumber(L, 3);
		LOP_range_check(minrange, maxrange);
		break;
	}

	case LOP_FOLLOW:
		LOP_follow_unit(lua_tolstring(L, 2, &len));
		break;
	
	case LOP_CTM: {
		double x, y, z;
		x = lua_tonumber(L, 2);
		y = lua_tonumber(L, 3);
		z = lua_tonumber(L, 4);
		int prio = lua_tointeger(L, 5);
		PRINT("LOP_CTM: %f, %f, %f, prio\n", x, y, z);

		LOP_CTM_act(x, y, z, prio);
		break;
	}
	
	case LOP_DUNGEON_SCRIPT: {
		const char* command = lua_tolstring(L, 2, &len);
	
		const char* scriptname = NULL;
		if (nargs > 2) {
			scriptname = lua_tolstring(L, 3, &len);
		}

		LOP_dungeon_script(command, scriptname ? scriptname : "");
		break;	
	}
	
	case LOP_TARGET_MARKER:
		LOP_target_marker(lua_tolstring(L, 2, &len));
		break;

	case LOP_MELEE_BEHIND:
		LOP_melee_behind();
		break;

	case LOP_AVOID_SPELL_OBJECT: {
		long spellID = lua_tointeger(L, 2);
		double radius = lua_tonumber(L, 3);
		LOP_avoid_spell_object(spellID, radius);
		break;
	}

	case LOP_HUG_SPELL_OBJECT: {
		long spellID = lua_tointeger(L, 2);
		LOP_hug_spell_object(spellID);
		break;
	}
	case LOP_SPREAD:
		LOP_spread();
		break;

	case LOP_CHAIN_HEAL_TARGET: {
		const char* current_heals = lua_tolstring(L, 2, &len);
		std::vector<std::string> names = LOP_chain_heal_target(current_heals);
		int n_rvals = 0;
		for (auto &n : names) {
			PUSHSTRING(L, n.c_str());
			++n_rvals;
		}
		return n_rvals;
	}

	case LOP_MELEE_AVOID_AOE_BUFF: {
		long spellID = lua_tointeger(L, 2);
		LOP_melee_avoid_aoe_buff(spellID);
		break;
	}

	case LOP_TANK_FACE:
		LOP_tank_face();
		break;

	case LOP_WALK_TO_PULLING_RANGE:
		LOP_walk_to_pull();
		break;

	case LOP_GET_UNIT_POSITION: {
		vec3 pos;
		double rot;
		int r = LOP_get_unit_position(lua_tolstring(L, 2, &len), &pos, &rot);
		
		if (!r) { return 0; }
		else {
			lua_pushnumber(L, pos.x);
			lua_pushnumber(L, pos.y);
			lua_pushnumber(L, pos.z);
			lua_pushnumber(L, rot);
			return 4;
		}
		break;
	}

	case LOP_GET_WALKING_STATE: 
		if (get_wow_CTM_state() != CTM_DONE) {
			lua_pushboolean(L, 1);
			return 1;
		}
		else {
			return 0;
		}
		break;

	case LOP_GET_CTM_STATE:
		// get the CTM state of the lole DLL
		break;

	case LOP_GET_PREVIOUS_CAST_MSG:
		lua_pushinteger(L, previous_cast_msg.msg);
		lua_pushnumber(L, (double)(previous_cast_msg.timestamp)/1000.0);
		return 2;
		break;

	case LOP_STOPFOLLOW:
		LOP_stopfollow();
		break;

	case LOP_CAST_GTAOE: {
		static timer_interval_t second(1000);

		if (!second.passed()) break;

		uint spellID = lua_tointeger(L, 2);
		vec3 pos = vec3(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	
		LOP_cast_gtaoe(spellID, pos);
		second.reset();

		break;
	}

	case LOP_CAST_SPELL: 
		LOP_cast_spell(lua_tointeger(L, 2), convert_str_to_GUID(lua_tolstring(L, 3, &len)));
		break;

	case LOP_HAS_AGGRO: 
		if (have_aggro()) {
			lua_pushboolean(L, 1);
			return 1;
		}
		break;
	
	case LOP_INTERACT_GOBJECT: 
		LOP_interact_object(lua_tolstring(L, 2, &len));
		break;
	
	case LOP_GET_BISCUITS:
		LOP_get_biscuits();
		break;

	case LOP_LOOT_BADGE:
		LOP_loot_badge(lua_tolstring(L, 2, &len));
		break;

	case LOP_GET_COMBAT_TARGETS: {
		std::vector<GUID_t> targets;
		LOP_get_combat_targets(&targets);
		
		for (int i = 0; i < targets.size(); ++i) {
			PUSHSTRING(L, convert_GUID_to_str(targets[i]).c_str());
		}

		return targets.size();

		break;
	}

	case LOP_GET_AOE_FEASIBILITY: {
		float f = LOP_get_aoe_feasibility(lua_tonumber(L, 2));
		lua_pushnumber(L, f);
		return 1;
	}

	case LOP_SL_RESETCAMERA:
		LOPSL_reset_camera();
		break;

	case LOP_WC3MODE: {
		int b = lua_tointeger(L, 2);
		enable_wc3mode(b);

		break;
	}

	case LOP_SL_SETSELECT: {
		wc3mode_setselection(lua_tolstring(L, 2, &len));
		break;
	}

	case LOP_AVOID_NPC_WITH_NAME: {
		
		if (ctm_queue_get_top_prio() == CTM_PRIO_NOOVERRIDE) return 0;

		size_t len;
		std::string name(lua_tolstring(L, 2, &len));
		ObjectManager OM;
		WowObject P;
		OM.get_local_object(&P);
		auto n = OM.get_NPCs_by_name(name);
		if (n.size() == 0) {
			return 0;
		}
		else {

			static float angle = 0;

			vec3 ppos = P.get_pos();
			int needed = 0;
			for (auto &o : n) {
				if ((o.get_pos() - ppos).length() < 15) {
					needed = 1;
					break;
				}
			}

			if (!needed) return 0;

			WowObject F;
			if (!OM.get_object_by_GUID(get_focus_GUID(), &F)) return 0;
			vec3 fpos = F.get_pos();

			vec3 newpos = fpos + 28 * vec3(1, 0, 0).rotated_2d(angle);

			angle += 0.25*M_PI;

			ctm_add(CTM_t(newpos, CTM_MOVE, CTM_PRIO_NOOVERRIDE, 0, 1.0));


		}
		return 0;
	
	}

	case LDOP_CAPTURE_FRAME_RENDER_STAGES:
		enable_capture_render();
		break;

	case LDOP_LUA_REGISTERED:
		lua_registered = 1;
		break;

	case LDOP_DUMP:
		dump_wowobjects_to_log();
		break;

	case LDOP_TEST: {
		ObjectManager OM;
		WowObject i;
		OM.get_first_object(&i);
		while (i.valid()) {
			if (i.get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
				vec3 pos = i.DO_get_pos();
				PRINT("dynamicobject: base: 0x%X, spellid: %u, pos: (%.1f, %.1f, %.1f)\n", i.get_base(), i.DO_get_spellID(), pos.x, pos.y, pos.z);
			}

			i = i.next();
		}

		break;
	}
	case LDOP_NOCLIP: {
		ObjectManager OM;
		WowObject p;
		if (OM.get_object_by_GUID(get_target_GUID(), &p)) {
			if (p.get_type() == OBJECT_TYPE_NPC) {
				printf("is dead: %d\n", p.NPC_unit_is_dead());
			}
		}
		//enable_noclip();
		break;
	}

	case LDOP_CONSOLE_PRINT: {
		const char* s = lua_tolstring(L, 2, &len);
		puts(s);

		//FILE *fp = fopen("C:\\Users\\Elias\\Desktop\\lua.log", "a");
		//fputs(s, fp);
		//fclose(fp);
		break;
	}

	default:
		PRINT("lop_exec: unknown opcode %d!\n", opcode);
		break;

	}
			 
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
	if (!follow_state.close_enough) LOP_follow_unit(follow_state.target_name);
}

static int dump_wowobjects_to_log() {

	ObjectManager OM;

	fprintf(stdout, "Basic info: ObjectManager base: %X, local GUID = 0x%016llX\n\n", OM.get_base_address(), OM.get_local_GUID());
	WowObject o;
	if (!OM.get_first_object(&o)) return 0;

	for (; o.valid(); o = o.next()) {
		uint type = o.get_type();
		if (type == OBJECT_TYPE_ITEM || type == OBJECT_TYPE_CONTAINER) { continue; }  
		
		fprintf(stdout, "object GUID: 0x%016llX, base addr = 0x%X, type: %s\n", o.get_GUID(), o.get_base(), o.get_type_name().c_str());

		if (type == OBJECT_TYPE_NPC || type == OBJECT_TYPE_UNIT) {
			vec3 pos = o.get_pos();
			fprintf(stdout, "coords = (%f, %f, %f), rot: %f\n", pos.x, pos.y, pos.z, o.get_rot());

			if (type == OBJECT_TYPE_NPC) {
				fprintf(stdout, "name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d\n\n", o.NPC_get_name().c_str(), o.NPC_get_health(), o.NPC_get_health_max(), o.NPC_get_target_GUID(), o.in_combat());
				//fprintf(fp, "--- buffs (by spellID): ---\n");
				//for (int n = 1; n <= 16; ++n) { // the maximum is actually 40, but..
				//	uint spellID = o.NPC_get_buff(n);
				//	if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
				//	else break;
				//}

				//fprintf(fp, "--- debuffs (by spellID): ---\n");

				//for (int n = 1; n <= 16; ++n) {
				//	uint spellID = o.NPC_get_debuff(n);
				//	if (spellID != 0) {
				//		uint duration = o.NPC_get_debuff_duration(n, spellID);
				//		fprintf(fp, "%d: %u, duration = %u\n", n, spellID, duration);
				//	}
				//	else break;
				//}

				//fprintf(fp, "\n");
				
			}
			else if (type == OBJECT_TYPE_UNIT) {
				fprintf(stdout, "name: %s, health: %u/%u, target GUID: 0x%016llX, combat = %d\n", o.unit_get_name().c_str(), o.unit_get_health(), o.unit_get_health_max(), o.unit_get_target_GUID(), o.in_combat());
				//fprintf(fp, "--- buffs (by spellID): ---\n");
				//for (int n = 1; n <= 16; ++n) {
				//	uint spellID = o.unit_get_buff(n);
				//	if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
				//}

				//fprintf(fp, "--- debuffs (by spellID): ---\n");

				//for (int n = 1; n <= 16; ++n) {
				//	uint spellID = o.unit_get_debuff(n);
				//	if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
				//}

				//fprintf(fp, "\n");
			}
		}
		else if (type == OBJECT_TYPE_DYNAMICOBJECT) {
			vec3 DO_pos = o.DO_get_pos();
			fprintf(stdout, "position: (%.1f, %.1f, %.1f), spellID: %d\n\n", DO_pos.x, DO_pos.y, DO_pos.z, o.DO_get_spellID());
		}
		else if (type == OBJECT_TYPE_GAMEOBJECT) {
			vec3 GO_pos = o.GO_get_pos();
		//	fprintf(fp, "name: %s, position: (%.1f, %.1f, %.1f)\n\n", o.GO_get_name().c_str(), GO_pos.x, GO_pos.y, GO_pos.z);
		}

		fprintf(stdout, "----------------------------------------------------------------------------\n");

	}
	
	//SelectUnit(target_GUID); // this is because the *_get_[de]buff calls call SelectUnit

	return 1;
}
