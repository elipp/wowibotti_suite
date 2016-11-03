#include "defs.h"

#include <Windows.h>
#include <Shlobj.h> // for the function that gets the desktop directory path for current user

#include "opcodes.h"
#include "ctm.h"
#include "timer.h"
#include "hooks.h"
#include "creds.h"
#include "dungeon_script.h"
#include "lua.h"
#include "packet.h"

extern HWND wow_hWnd;

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
 LOPFUNC(LOP_CASTER_RANGE_CHECK, 1, 1, 0),
 LOPFUNC(LOP_FOLLOW, 1, 1, 0),
 LOPFUNC(LOP_CTM, 3, 3, 0),
 LOPFUNC(LOP_DUNGEON_SCRIPT, 1, 2, 0),
 LOPFUNC(LOP_TARGET_MARKER, 1, 1, 0),
 LOPFUNC(LOP_MELEE_BEHIND, 0, 0, 0),
 LOPFUNC(LOP_AVOID_SPELL_OBJECT, 1, 2, 0),
 LOPFUNC(LOP_HUG_SPELL_OBJECT, 1, 1, 0),
 LOPFUNC(LOP_SPREAD, 0, 0, 0),
 LOPFUNC(LOP_CHAIN_HEAL_TARGET, 1, 1, 1),
 LOPFUNC(LOP_MELEE_AVOID_AOE_BUFF, 2, 2, 0),
 LOPFUNC(LOP_TANK_FACE, 0, 0, 0),
 LOPFUNC(LOP_WALK_TO_PULLING_RANGE, 0, 0, 0),
 LOPFUNC(LOP_GET_UNIT_POSITION, 1, 1, 3),
 LOPFUNC(LOP_GET_WALKING_STATE, 0, 0, 1),
 LOPFUNC(LOP_GET_CTM_STATE, 0, 0, 1),
 LOPFUNC(LOP_GET_PREVIOUS_CAST_MSG, 0, 0, 1),
 LOPFUNC(LOP_STOPFOLLOW, 0, 0, 0),
 LOPFUNC(LOP_CAST_GTAOE, 4, 4, 0),

};

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

static int LOP_melee_behind() {
	
	ObjectManager OM;

	WowObject p, t;
	if (!OM.get_local_object(&p) || !OM.get_object_by_GUID(get_target_GUID(), &t)) {
		return 0;
	}


	// basically rotating the vector (1, 0, 0) target_rot radians anti-clockwise
	//vec3 rot_unit(1.0 * std::cos(target_rot) - 0.0 * std::sin(target_rot), 1.0 * std::sin(target_rot) + 0.0 * std::cos(target_rot), 0.0);

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();
	
	// ok, so turns out the get_rot() variable isn't really kept up-to-date in the OM,
	// so the actual rotation will need to be figured out by means of looking at who the mob is
	// targeting, and looking at the difference vector (since the mob is always directly facing the target)

	GUID_t tot_GUID = t.NPC_get_target_GUID();
	float target_rot;

	if (tot_GUID) {
		if (tot_GUID == OM.get_local_GUID()) return 0;
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

	float prot = p.get_rot();
	vec3 prot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);

	//vec3 point_behind(tc.x - std::cos(target_rot), tc.y - std::sin(target_rot), tc.z);
	vec3 point_behind_actual = tpos - 3.0*trot_unit;
	vec3 point_behind_ctm = point_behind_actual + 0.5*prot_unit;
	vec3 diff = point_behind_ctm - ppos;
	
	if (diff.length() > 1.0) {
		ctm_add(CTM_t(point_behind_ctm, CTM_MOVE, CTM_PRIO_LOW, 0, 0.5));
		return 1;
	}
	else {
		DoString("StartAttack()");
		float d = dot(prot_unit, trot_unit);

		 if (d < 0.6) { 
			 // dot product of normalized vectors gives the cosine of the angle between them,
			 // and for auto-attacking, the valid sector is actually rather small, unlike spells,
			 // for which perfectly perpendicular is ok
			vec3 face = (tpos - ppos).unit();
			ctm_add(CTM_t(ppos + face, CTM_MOVE, CTM_PRIO_LOW, 0, 1.5));
		}
	}


}

static int LOP_melee_avoid_aoe_buff(long spellID) {
	
	ObjectManager OM;

	WowObject o = OM.get_first_object();

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

static int LOP_target_GUID(const std::string &arg) {

	std::string GUID_numstr(arg.substr(2, 16)); // better make a copy of it. the GUID_str still has the "0x" prefix in it 

	char *end;
	GUID_t GUID = strtoull(GUID_numstr.c_str(), &end, 16);

	if (end != GUID_numstr.c_str() + GUID_numstr.length()) {
		PRINT("[WARNING]: change_target: couldn't wholly convert GUID string argument (strtoull(\"%s\", &end, 16) failed, bailing out\n", GUID_numstr.c_str());
		return 0;
	}
	
	PRINT("got LOLE_OPCODE_TARGET_GUID: GUID = %llX\nGUID_str + prefix.length() + 2 = \"%s\"\n", GUID, GUID_numstr.c_str());

	SelectUnit(GUID);

	return 1;
}

static int LOP_range_check(double minrange) {
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

	if (diff.length() > minrange - 1) {
		// move in a straight line to a distance of minrange-1 yd from the target. Kinda bug-prone though..
		vec3 new_point = tpos - (minrange - 1) * diff.unit();
		ctm_add(CTM_t(new_point, CTM_MOVE, CTM_PRIO_LOW, 0, 1.5));
		return 1;

	}
	else {

		// a continuous kind of facing function could also be considered (ie. just always face directly towards the mob)

		float rot = p.get_rot();
		vec3 rot_unit = vec3(std::cos(rot), std::sin(rot), 0.0);
		float d = dot(diff, rot_unit);

	//	PRINT("dot product: %f\n", d);

		if (d < 0) {
			ctm_add(CTM_t(ppos + diff.unit(), CTM_MOVE, CTM_PRIO_LOW, 0, 1.5)); // this seems quite stable for just changing orientation.
		}
	}

	return 1;

}

static int LOP_follow_unit(const std::string& targetname) {

	// CONSIDER: change this to straight up player name and implement a get WowObject by name func?
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
	
	click_to_move(t.get_pos(), CTM_MOVE, 0);

	if ((t.get_pos() - p.get_pos()).length() < 10) {
		PRINT("follow difference < 10! calling WOWAPI FollowUnit()\n");
		DoString("FollowUnit(\"%s\")", t.unit_get_name().c_str());
		follow_state.clear();
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

static int LOP_stopfollow() {
	
	ObjectManager OM;

	WowObject p;
	OM.get_local_object(&p);

	float prot = p.get_rot();
	vec3 rot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);
	click_to_move(p.get_pos() + 0.51*rot_unit, CTM_MOVE, 1.5);
	follow_state.clear();

	return 1;

}

static void LOP_CTM_act(double x, double y, double z) {
	click_to_move(vec3(x, y, z), CTM_MOVE, 0);
}

static void LOP_nop(const std::string& arg) {
	return;
}

static void LOP_dungeon_script(const std::string &command, const std::string &scriptname) {

	if (command == "run") {
		dscript_load(scriptname);
	}
	else if (command == "stop") {
		dscript_unload();
	}
	else if (command == "next") {
		dscript_next();
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
		if ((ppos - spos).length() < radius) {
			// then we need to run away from it :D
			escape_pos = spos + (radius+1.5)*(ppos - spos).unit();
			need_to_escape = 1;
		}
	}
	

	if (!need_to_escape) {
		return 0;
	}

	ctm_add(CTM_t(escape_pos, CTM_MOVE, CTM_PRIO_EXCLUSIVE, 0, 0.5));

	return 1;

}

static int LOP_spread() {
	return 1;
}


static void pull_mob() {
	DoString("CastSpellByName(\"Avenger's Shield\")");
}

static void set_target_and_blast() {
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
		c.add_posthook(CTM_posthook_t(pull_mob, 10));
		ctm_add(c);
	}
	else {
		CTM_t c(ppos + dn, CTM_MOVE, 0, 0, 0.5);
		c.add_posthook(CTM_posthook_t(pull_mob, 10));
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
	
	WowObject next = OM.get_first_object();

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
		PRINT("candidate %s with %u/%u hp (heal urgency: %f) added\n", u.name.c_str(), u.health, u.health_max, u.heal_urgency, u.deficit);
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

static int LOP_maulgar_get_felhound() {

	// TODO: ONLY RETURN TARGET GUID, DON'T CAST BANISH HERE ETC
	ObjectManager OM;

	WowObject next = OM.get_first_object();

	while (next.valid()) {

		if (next.get_type() == OBJECT_TYPE_NPC) {
			if (next.NPC_get_name() == "Wild Fel Stalker") {
				if (!mob_has_debuff(next, 18647)) { // this is Banish (Rank 2)
					SelectUnit(next.get_GUID());
					DoString("CastSpellByName(\"Banish\")");
					return 1;
				}
			}
		}

		next = next.next();
	}

	SelectUnit(0);

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
	pull.add_posthook(CTM_posthook_t(pull_mob, 30));
	pull.add_posthook(CTM_posthook_t(set_target_and_blast, 120));

	ctm_add(pull);
}

static int LOP_get_unit_position(const std::string &name, double *out) {
	ObjectManager OM;
	
	if (name == "player") {
		WowObject p;
		if (!OM.get_local_object(&p)) return 0;

		vec3 ppos = p.get_pos();
		out[0] = ppos.x;
		out[1] = ppos.y;
		out[2] = ppos.z;

		return 1;
	}
	else if (name == "target") {
		WowObject t;
		GUID_t target_GUID = get_target_GUID();

		if (!target_GUID) { return 0; }

		if (!OM.get_object_by_GUID(target_GUID, &t)) {
			return 0;
		}
		vec3 tpos = t.get_pos();

		out[0] = tpos.x;
		out[1] = tpos.y;
		out[2] = tpos.z;

		return 1;
	}
	else {
		WowObject u;
		if (!OM.get_unit_by_name(name, &u)) { return 0; }

		vec3 upos = u.get_pos();

		out[0] = upos.x;
		out[1] = upos.y;
		out[2] = upos.z;
		
		return 1;
		
	}

	return 0;

}

static int have_aggro() {
	ObjectManager OM;

	WowObject p;
	OM.get_local_object(&p);

	GUID_t pGUID = p.get_GUID();

	WowObject o = OM.get_first_object();

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
	
	case LOP_TARGET_GUID:
		LOP_target_GUID(lua_tolstring(L, 2, &len));
		break;

	case LOP_CASTER_RANGE_CHECK: {
		double minrange = lua_tonumber(L, 2);
		LOP_range_check(minrange);
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
		PRINT("LOP_CTM: %f, %f, %f\n", x, y, z);

		LOP_CTM_act(x, y, z);
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
		double coords[3];
		int r = LOP_get_unit_position(lua_tolstring(L, 2, &len), coords);
		
		if (!r) { return 0; }
		else {
			lua_pushnumber(L, coords[0]);
			lua_pushnumber(L, coords[1]);
			lua_pushnumber(L, coords[2]);
			return 3;
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
		BYTE sockbuf[] = {
			0x00, 0x19, 0x2E, 0x01, 0x00, 0x00, // HEADER
			0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x40, 0x00, 0x00, 0x00,
		 // ^^^^^^^^^^^^^^^^^^^^^^ - SPELLID
			0xA1, 0xA2, 0xA3, 0xA4, // x:float	
			0xB1, 0xB2, 0xB3, 0xB4, // y:float
			0xC1, 0xC2, 0xC3, 0xC4	// z:float
		};

		long spellID = lua_tointeger(L, 2);
		
		float x, y, z;
		x = lua_tonumber(L, 3);
		y = lua_tonumber(L, 4);
		z = lua_tonumber(L, 5);

		memcpy(sockbuf + 6, &spellID, sizeof(spellID));
		memcpy(sockbuf + 15, &x, sizeof(x));
		memcpy(sockbuf + 19, &y, sizeof(y));
		memcpy(sockbuf + 23, &z, sizeof(z));

		encrypt_packet_header(sockbuf);
		
		SOCKET s = get_wow_socket_handle();
		send(s, (const char*)sockbuf, sizeof(sockbuf), 0);

		break;
	}
	case LOP_HAS_AGGRO: 
		if (have_aggro()) {
			lua_pushboolean(L, 1);
			return 1;
		}
		break;

	case LDOP_LUA_REGISTERED:
		lua_registered = 1;
		break;

	case LDOP_DUMP:
		dump_wowobjects_to_log();
		break;
	case LDOP_LOS_TEST: {
		break;
	}

	default:
		PRINT("lop_exec: unknown opcode %d!\n", opcode);
		break;

	}
			 
	return 0;
}


// follow stuff

void refollow_if_needed() {
	if (!follow_state.close_enough) LOP_follow_unit(follow_state.target_name);
}

static int dump_wowobjects_to_log() {

	char desktop_path[MAX_PATH];

	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, desktop_path))) {
		PRINT("SHGetFolderPath for CSIDL_DESKTOPDIRECTORY failed, errcode %d\n", GetLastError());
		return 0;
	}

	static const std::string log_path = std::string(desktop_path) + "\\wodump.log";
	FILE *fp;
	errno_t err = fopen_s(&fp, log_path.c_str(), "w");

	if (!fp) {
		PRINT("Opening file \"%s\" failed!\n", log_path.c_str());
		return 0;
	}

	ObjectManager OM;

	PRINT("Dumping WowObjects to file \"%s\"!\n", log_path.c_str());
	GUID_t target_GUID = get_target_GUID();

	fprintf(fp, "Basic info: ObjectManager base: %X, local GUID = 0x%016llX, player target: 0x%016llX\n\n", OM.get_base_address(), OM.get_local_GUID(), target_GUID);

	for (WowObject o = OM.get_first_object(); o.valid(); o = o.next()) {
		uint type = o.get_type();
		if (type == OBJECT_TYPE_ITEM || type == OBJECT_TYPE_CONTAINER) { continue; }  
		
		fprintf(fp, "object GUID: 0x%016llX, base addr = 0x%X, type: %s\n", o.get_GUID(), o.get_base(), o.get_type_name().c_str());

		if (type == OBJECT_TYPE_NPC || type == OBJECT_TYPE_UNIT) {
			vec3 pos = o.get_pos();
			fprintf(fp, "coords = (%f, %f, %f), rot: %f\n", pos.x, pos.y, pos.z, o.get_rot());

			if (type == OBJECT_TYPE_NPC) {
				fprintf(fp, "name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d\n\n", o.NPC_get_name().c_str(), o.NPC_get_health(), o.NPC_get_health_max(), o.NPC_get_target_GUID(), o.in_combat());
				fprintf(fp, "--- buffs (by spellID): ---\n");
				for (int n = 1; n <= 16; ++n) { // the maximum is actually 40, but..
					uint spellID = o.NPC_get_buff(n);
					if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
					else break;
				}

				fprintf(fp, "--- debuffs (by spellID): ---\n");

				for (int n = 1; n <= 16; ++n) {
					uint spellID = o.NPC_get_debuff(n);
					if (spellID != 0) {
						uint duration = o.NPC_get_debuff_duration(n, spellID);
						fprintf(fp, "%d: %u, duration = %u\n", n, spellID, duration);
					}
					else break;
				}

				fprintf(fp, "\n");
				
			}
			else if (type == OBJECT_TYPE_UNIT) {
				fprintf(fp, "name: %s, health: %u/%u, target GUID: 0x%016llX, combat = %d\n", o.unit_get_name().c_str(), o.unit_get_health(), o.unit_get_health_max(), o.unit_get_target_GUID(), o.in_combat());
				fprintf(fp, "--- buffs (by spellID): ---\n");
				for (int n = 1; n <= 16; ++n) {
					uint spellID = o.unit_get_buff(n);
					if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
				}

				fprintf(fp, "--- debuffs (by spellID): ---\n");

				for (int n = 1; n <= 16; ++n) {
					uint spellID = o.unit_get_debuff(n);
					if (spellID != 0) fprintf(fp, "%d: %u\n", n, spellID);
				}

				fprintf(fp, "\n");
			}
		}
		else if (type == OBJECT_TYPE_DYNAMICOBJECT) {
			vec3 DO_pos = o.DO_get_pos();
			fprintf(fp, "position: (%.1f, %.1f, %.1f), spellID: %d\n\n", DO_pos.x, DO_pos.y, DO_pos.z, o.DO_get_spellID());
		}
		else if (type == OBJECT_TYPE_GAMEOBJECT) {
			vec3 GO_pos = o.GO_get_pos();
			fprintf(fp, "name: %s, position: (%.1f, %.1f, %.1f)\n\n", o.GO_get_name().c_str(), GO_pos.x, GO_pos.y, GO_pos.z);
		}

		fprintf(fp, "----------------------------------------------------------------------------\n");

	}
	fclose(fp);
	err = fopen_s(&fp, log_path.c_str(), "r");

	char buf[256];

	while (fgets(buf, 256, fp) != NULL) {
		if (*buf) {
			buf[strlen(buf) - 1] = '\0'; // the AddMessage function will fail with unfinished string error without this
			DoString("DEFAULT_CHAT_FRAME:AddMessage(\"|cFFFFD100%s\")", buf);
		}
	}
	
	fclose(fp);

	return 1;
}
