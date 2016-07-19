#include <Windows.h>
#include <Shlobj.h> // for the function that gets the desktop directory path for current user

#include "opcodes.h"
#include "ctm.h"
#include "timer.h"
#include "hooks.h"
#include "creds.h"

extern HWND wow_hWnd;
extern int afkjump_keyup_queued;

static int dump_wowobjects_to_log();

static struct follow_state_t {
	int close_enough = 1;
	Timer timer;
	std::string target_GUID;

	void start(const std::string &GUID) {
		close_enough = 0;
		target_GUID = GUID;
		timer.start();
	}

	void clear() {
		close_enough = 1;
		target_GUID = "";
	}

} follow_state;


static void __stdcall walk_to_target() {

	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	click_to_move(t.get_pos(), CTM_MOVE, 0);

}

static void LOP_face(const std::string &arg) {

	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_local_object();

	if (!p.valid()) return;

	vec3 diff = t.get_pos() - p.get_pos();



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

}

static void LOP_melee_behind(const std::string &arg) {
	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) {
		return;
	}

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_local_object();

	if (!p.valid()) return;

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
		if (tot_GUID == OM.get_local_GUID()) return;
		WowObject tot = OM.get_object_by_GUID(tot_GUID);
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
		click_to_move(point_behind_ctm, CTM_MOVE, 0);
	}
	else {
		DoString("StartAttack()");
		float d = dot(prot_unit, trot_unit);

		 if (d < 0.6) { 
			 // dot product of normalized vectors gives the cosine of the angle between them,
			 // and for auto-attacking, the valid sector is actually rather small, unlike spells,
			 // for which perfectly perpendicular is ok
			vec3 face = (tpos - ppos).unit();
			click_to_move(ppos + face, CTM_MOVE, 0, 1.5);
		}
	}


}

static void LOP_target_GUID(const std::string &arg) {

	std::vector<std::string> tokens;

	tokenize_string(arg, ",", tokens);

	if (tokens.size() > 1) {
		printf("change_target (via DelIgnore_hub): expected 1 argument, got %ul! Ignoring rest.\n", tokens.size());
	}

	std::string GUID_numstr(arg.substr(2, 16)); // better make a copy of it. the GUID_str still has the "0x" prefix in it 

	char *end;
	GUID_t GUID = strtoull(GUID_numstr.c_str(), &end, 16);

	printf("got LOLE_OPCODE_TARGET_GUID: GUID = %llX\nGUID_str + prefix.length() + 2 = \"%s\"\n", GUID, GUID_numstr.c_str());

	if (end != GUID_numstr.c_str() + GUID_numstr.length()) {
		printf("[WARNING]: change_target: couldn't wholly convert GUID string argument (strtoull(\"%s\", &end, 16) failed, bailing out\n", GUID_numstr.c_str());
		return;
	}

	SelectUnit(GUID);
}

static void LOP_blast(const std::string &arg) {
	if (arg == "1") {
		printf("got BLAST_ON AddonMessage, enabling blast.\n");
	}
	else if (arg == "0") {
		printf("got BLAST_OFF AddonMessage, disabling blast.\n");
	}
	else {
		printf("blast (from DelIgnore_hub): warning: unknown argument \"%s\"\n", arg.c_str());
	}
}

static void LOP_range_check(const std::string& arg) {
	GUID_t target_GUID = get_target_GUID();
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_local_object();

	if (!p.valid()) return;

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();

	vec3 diff = tpos - ppos;

	char *endptr;
	float minrange = strtof(arg.c_str(), &endptr);

	if (diff.length() > minrange - 1) {
		// move in a straight line to a distance of minrange-1 yd from the target. Kinda bug-prone though..
		vec3 new_point = tpos - (minrange - 1) * diff.unit();
		click_to_move(new_point, CTM_MOVE, 0x0);
		return;

	}
	else {

		// a continuous kind of facing function could also be considered (ie. just always face directly towards the mob)

		float rot = p.get_rot();
		vec3 rot_unit = vec3(std::cos(rot), std::sin(rot), 0.0);
		float d = dot(diff, rot_unit);

	//	printf("dot product: %f\n", d);

		if (d < 0) {
			click_to_move(ppos + diff.unit(), CTM_MOVE, 0, 1.5); // this seems quite stable for just changing orientation.
		}
	}

}

static void LOP_follow_GUID(const std::string& arg) {
	// the arg should contain host character GUID
	// CONSIDER: change this to straight up player name and implement a get WowObject by name func?
	ObjectManager OM;

	char *endptr;
	GUID_t GUID = strtoull(arg.c_str(), &endptr, 16);

	WowObject p = OM.get_local_object();

	if (!p.valid()) {
		printf("follow_unit_with_GUID: LOLE_OPCODE_FOLLOW: getting local object failed? WTF? XD\n");
		return;
	}

	if (GUID == 0) {
		float prot = p.get_rot();
		vec3 rot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);
		click_to_move(p.get_pos() + 0.51*rot_unit, CTM_MOVE, 0);
		follow_state.clear();
		return;
	}

	WowObject o = OM.get_object_by_GUID(GUID);

	if (!o.valid()) {
		printf("follow_unit_with_GUID: LOLE_OPCODE_FOLLOW: couldn't find unit with GUID 0x%016llX (doesn't exist?)\n", GUID);
		// not reset
		follow_state.clear();

		return;
	}

	if (p.get_GUID() == o.get_GUID()) {
		return;
	}

	click_to_move(o.get_pos(), CTM_MOVE, 0);

	if ((o.get_pos() - p.get_pos()).length() < 10) {
		printf("follow difference < 10! calling WOWAPI FollowUnit()\n");
		DoString("FollowUnit(\"%s\")", o.unit_get_name().c_str());
		follow_state.clear();
	}
	else {
		// close_enough == 1 means the follow attempt either hasn't been started yet or that the char has actually reached its target
		if (follow_state.close_enough) {
			follow_state.start(arg);
		}
	}


	if (follow_state.timer.get_s() > 10) {
		follow_state.clear();
	}

	return;

}

static void LOP_caster_face(const std::string &arg) {
	//face_queued = 1;
}

static void LOP_CTM_act(const std::string &arg) {
	// arg should contain 3 args, <x,y,z>

	std::vector<std::string> tokens;
	tokenize_string(arg, ",", tokens);

	if (tokens.size() != 3) {
		printf("act_on_CTM_broadcast: error: expected exactly 3 arguments (x,y,z), got %lu!\n", tokens.size());
		return;
	}

	char *endptr;

	float x, y, z;

	x = strtof(tokens[0].c_str(), &endptr);
	y = strtof(tokens[1].c_str(), &endptr);
	z = strtof(tokens[2].c_str(), &endptr);

	click_to_move(vec3(x, y, z), CTM_MOVE, 0);

}

static void LOP_nop(const std::string& arg) {
	return;
}

static void LOP_dungeon_script(const std::string &arg) {
	// =)
}

static void LOP_target_marker(const std::string &arg) {
	// arg = marker name
	GUID_t GUID = get_raid_target_GUID(arg);

	if (GUID == 0) {
		DoString("ClearTarget()");
	}

	else {
		SelectUnit(GUID);
	}
}

static void LOP_afk_clear(const std::string &arg) {
	PostMessage(wow_hWnd, WM_KEYDOWN, VK_LEFT, get_KEYDOWN_LPARAM(VK_LEFT));
	afkjump_keyup_queued = 2;
}

static void LOP_avoid_spell_object(const std::string &arg) {
	char *endptr;
	long spellID = strtoul(arg.c_str(), &endptr, 16);
}

static void LOP_hug_spell_object(const std::string &arg) {
	char *endptr;
	long spellID = strtoul(arg.c_str(), &endptr, 10);

	ObjectManager OM;

	auto objs = OM.get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		printf("No objects with spellid %ld\n", spellID);
		ctm_unlock();
		return;
	}
	else {
		printf("hugging %ld!\n", spellID);
	}

	// just run to obj #1 for now

	click_to_move(objs[0].DO_get_pos(), CTM_MOVE, 0);

	ctm_lock();

}

static void LOP_spread(const std::string &arg) {

}

static void LOP_pull_mob(const std::string &arg) {
	char *endptr;
	GUID_t GUID = strtoull(arg.c_str(), &endptr, 16);
}

static void LOP_report_login(const std::string &arg) {
	if (arg == "1") {
		credentials.logged_in = 1;
	}
	else {
		credentials.logged_in = 0;
	}
}

static void pull_mob() {
	DoString("CastSpellByName(\"Avenger's Shield\")");
}

static void LOP_walk_to_pull(const std::string &arg) {

	ObjectManager OM;
	WowObject p = OM.get_local_object();
	WowObject t = OM.get_object_by_GUID(get_target_GUID());

	vec3 ppos = p.get_pos(), tpos = t.get_pos();
	vec3 d = tpos - ppos, dn = d.unit();

	if (d.length() > 30) {
		vec3 newpos = tpos - 29 * dn;
		CTM_t c(newpos, CTM_MOVE, 0, 0, 0.5);
		c.set_posthook(CTM_posthook_t(pull_mob, 10));
		ctm_add(c);
	}
	else {
		CTM_t c(ppos + dn, CTM_MOVE, 0, 0, 0.5);
		c.set_posthook(CTM_posthook_t(pull_mob, 10));
		ctm_add(c);
	}

	ctm_act();

}

static const WO_cached *find_most_hurt_within_CH_bounce(const WO_cached *unit, const WO_cached *unit2, const std::vector<WO_cached> &candidates) {

	if (!unit) return NULL;

	const WO_cached *most_hurt = NULL;

//	printf("calling CHbounce with unit %s, unit2 %s\n", unit->name.c_str(), unit2 ? unit2->name.c_str() : "NULL");

	for (int i = 0; i < candidates.size(); ++i) {
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
			//printf("new most_hurt = %s\n", most_hurt->name.c_str());
		}
	}
	
	if (most_hurt) {
		//printf("best next CH target for %s is %s with %u/%u HP\n\n", unit->name.c_str(), most_hurt->name.c_str(), most_hurt->health, most_hurt->health_max);
	}
	else {
		//printf("couldn't find a suitable CH target for %s\n\n", unit->name.c_str());
	}

	return most_hurt;

}

static void LOP_get_best_CH(const std::string &arg) {

	ObjectManager OM;
	
	WowObject next = OM.get_first_object();

	// cache units for easier access

	std::vector <WO_cached> units;
	while (next.valid()) {
		if (next.get_type() == OBJECT_TYPE_UNIT) {
			units.push_back(WO_cached(next.get_GUID(), next.get_pos(), next.unit_get_health(), next.unit_get_health_max(), next.unit_get_name()));
		}
		next = next.getNextObject();
	}

	std::vector<WO_cached> deficit_candidates;

	for (auto &u : units) {
		if (u.deficit > 1500) {
			deficit_candidates.push_back(u);
			//printf("candidate %s with %u/%u hp added (deficit == %d > 1500)\n", u.name.c_str(), u.health, u.health_max, u.deficit);
		}
	}

	struct chain_heal_trio {
		const WO_cached *trio[3];
		int total_deficit;
	};

	chain_heal_trio o;

	memset(&o, 0, sizeof(o));
	
	for (int i = 0; i < deficit_candidates.size(); ++i) {
		// scan vicinity for hurt chars within 12.5 yards

		const WO_cached *c = &deficit_candidates[i];

	//	printf("trying to find good CH targets for primary target %s...\n", c->name.c_str());
		
		const WO_cached *most_hurt[2];
		memset(most_hurt, 0, sizeof(most_hurt));

		most_hurt[0] = find_most_hurt_within_CH_bounce(c, NULL, deficit_candidates);
		most_hurt[1] = find_most_hurt_within_CH_bounce(most_hurt[0], c, deficit_candidates);

		int total_deficit = c->deficit + (most_hurt[0] ? most_hurt[0]->deficit : 0) + (most_hurt[1] ? most_hurt[1]->deficit : 0);
	
		if (total_deficit > o.total_deficit) {
		//	printf("found better one with deficit %d\n", total_deficit);
			o.trio[0] = c;
			o.trio[1] = most_hurt[0];
			o.trio[2] = most_hurt[1];
			o.total_deficit = total_deficit;
		}
		
	}

	/*printf("Found optimal CH targets: %s (%u/%u), %s (%u/%u), %s (%u/%u); total deficit = %d\n",
		(o.trio[0] ? o.trio[0]->name.c_str() : "NULL"), (o.trio[0] ? o.trio[0]->health : 0), (o.trio[0] ? o.trio[0]->health_max : 0),
		(o.trio[1] ? o.trio[1]->name.c_str() : "NULL"), (o.trio[1] ? o.trio[1]->health : 0), (o.trio[1] ? o.trio[1]->health_max : 0),
		(o.trio[2] ? o.trio[2]->name.c_str() : "NULL"), (o.trio[2] ? o.trio[2]->health : 0), (o.trio[2] ? o.trio[2]->health_max : 0),
		o.total_deficit);*/

	if (o.trio[0]) DoString("TargetUnit(\"%s\")", o.trio[0]->name.c_str());

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

static void LOP_maulgar_get_felhound(const std::string &arg) {
	ObjectManager OM;

	WowObject next = OM.get_first_object();

	while (next.valid()) {

		if (next.get_type() == OBJECT_TYPE_NPC) {
			if (next.NPC_get_name() == "Wild Fel Stalker") {
				if (!mob_has_debuff(next, 18647)) { // this is Banish (Rank 2)
					SelectUnit(next.get_GUID());
					DoString("RunMacroText(\"/stopcasting /cast Banish\")");
					return;
				}
			}
		}

		next = next.getNextObject();
	}

	SelectUnit(0);
}

static void LOPDBG_dump(const std::string &arg) {
	dump_wowobjects_to_log();
}

static void LOPDBG_loot(const std::string &arg) {
	ObjectManager OM;
	WowObject corpse = OM.get_object_by_GUID(get_target_GUID());
	
	click_to_move(corpse.get_pos(), CTM_LOOT, corpse.get_GUID());
}

static void LOPDBG_query_injected(const std::string &arg) {
	DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D
}

static void LOPDBG_pull_test(const std::string &arg) {

	vec3 pull_pos(-1787, 4867, 0.5);

	CTM_t test1(vec3(-1837, 4862, 2), CTM_MOVE, 0, 0, 0.5);
	CTM_t test2(pull_pos, CTM_MOVE, 0, 0, 0.5);

	ObjectManager OM;

	WowObject p = OM.get_local_object();
	WowObject t = OM.get_object_by_GUID(get_target_GUID());

	vec3 ppos = p.get_pos(), tpos = t.get_pos();
	vec3 dir = (tpos - ppos).unit();
	CTM_t test3(pull_pos + dir, CTM_MOVE, 0, 0, 0.5);

	test3.set_posthook(CTM_posthook_t(pull_mob, 30));

	ctm_add(test1);
	ctm_add(test2);
	ctm_add(test3);
	
}

static const struct {
	std::string name;
	hubfunc_t func;
	uint num_args;
} opcode_funcs[] = {
	{ "LOLE_NOP", LOP_nop, 0 },
	{ "LOLE_TARGET_GUID", LOP_target_GUID, 1 },
	{ "LOLE_BLAST", LOP_blast, 1 },
	{ "LOLE_CASTER_RANGE_CHECK", LOP_range_check, 1 },
	{ "LOLE_FOLLOW", LOP_follow_GUID, 1 },
	{ "LOLE_CASTER_FACE", LOP_caster_face, 0 },
	{ "LOLE_CTM_BROADCAST", LOP_CTM_act, 3 },
	{ "LOLE_COOLDOWNS", LOP_nop, 0 }, // this is actually done in LUA, but just to keep the protocol congruent
	{ "LOLE_CC", LOP_nop, 0 },		   // this also is done in LUA
	{ "LOLE_DUNGEON_SCRIPT", LOP_dungeon_script, 1 },
	{ "LOLE_TARGET_MARKER", LOP_target_marker, 1 },
	{ "LOLE_DRINK", LOP_nop, 0},
	{ "LOLE_MELEE_BEHIND", LOP_melee_behind, 0},
	{ "LOLE_LEAVE_PARTY", LOP_nop, 0},
	{ "LOLE_AFK_CLEAR", LOP_afk_clear, 0},
	{ "LOLE_RELEASE_SPIRIT", LOP_nop, 0},
	{ "LOLE_MAIN_TANK", LOP_nop, 0},
	{ "LOLE_AVOID_SPELL_OBJECT", LOP_avoid_spell_object, 1 },
	{ "LOLE_HUG_SPELL_OBJECT", LOP_hug_spell_object, 1 },
	{ "LOLE_SPREAD", LOP_spread, 0 },
	{ "LOLE_PULL_MOB", LOP_nop, 0 },
	{ "LOLE_REPORT_LOGIN", LOP_report_login, 1 },
	{ "LOLE_WALK_TO_PULLING_RANGE", LOP_walk_to_pull, 0 },
	{ "LOLE_GET_BEST_CHAINHEAL_TARGET", LOP_get_best_CH, 0},
	{ "LOLE_MAULGAR_GET_UNBANISHED_FELHOUND", LOP_maulgar_get_felhound, 0 }
};

static const struct {
	std::string name;
	hubfunc_t func;
	uint num_args;
} debug_opcode_funcs[] = {
	{ "LOLE_DEBUG_NOP", LOP_nop, 0 },
	{ "LOLE_DEBUG_DUMP", LOPDBG_dump, 0 },
	{ "LOLE_DEBUG_LOOT_ALL", LOPDBG_loot, 0},
	{ "LOLE_DEBUG_QUERY_INJECTED", LOPDBG_query_injected, 0 },
	{ "LOLE_DEBUG_PULL_TEST", LOPDBG_pull_test, 0 }
};

static const size_t num_opcode_funcs = sizeof(opcode_funcs) / sizeof(opcode_funcs[0]);
static const size_t num_debug_opcode_funcs = sizeof(debug_opcode_funcs) / sizeof(debug_opcode_funcs[0]);

// ----------------------
//	  public interface
// -----------------------

int opcode_call(int opcode, const std::string &arg) {
	
	if (opcode > num_opcode_funcs-1) {
		printf("opcode_call: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return 0;
	}
	opcode_funcs[opcode].func(arg);

	return 1;
}

int opcode_debug_call(int debug_opcode, const std::string &arg) {
	if (debug_opcode > num_debug_opcode_funcs - 1) {
		printf("opcode_debug_call: error: unknown opcode %lu. (valid range: 0 - %lu)\n", debug_opcode, num_debug_opcode_funcs);
		return 0;
	}

	debug_opcode_funcs[debug_opcode].func(arg);

	return 1;
}


int opcode_get_num_args(int opcode) {
	if (opcode > num_opcode_funcs - 1) {
		printf("opcode_get_num_args: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return -1;
	}

	return opcode_funcs[opcode].num_args;
}

const std::string &opcode_get_funcname(int opcode) {
	
	static std::string err = "ERROR";

	if (opcode > num_opcode_funcs - 1) {
		printf("opcode_get_funcname: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return err;
	}

	return opcode_funcs[opcode].name;
}

const std::string &debug_opcode_get_funcname(int opcode_unmasked) {
	static std::string err = "ERROR";

	if (opcode_unmasked > num_debug_opcode_funcs - 1) {
		printf("opcode_get_funcname: error: unknown DEBUG opcode %lu. (valid range: 0 - %lu)\n", opcode_unmasked, num_debug_opcode_funcs);
		return err;
	}

	return debug_opcode_funcs[opcode_unmasked].name;
}

// follow stuff

void refollow_if_needed() {
	if (!follow_state.close_enough) LOP_follow_GUID(follow_state.target_GUID);
}

static int dump_wowobjects_to_log() {

	char desktop_path[MAX_PATH];

	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, desktop_path))) {
		printf("SHGetFolderPath for CSIDL_DESKTOPDIRECTORY failed, errcode %d\n", GetLastError());
		return 0;
	}

	static const std::string log_path = std::string(desktop_path) + "\\wodump.log";
	FILE *fp = fopen(log_path.c_str(), "w");

	if (!fp) {
		printf("Opening file \"%s\" failed!\n", log_path.c_str());
		return 0;
	}

	ObjectManager OM;

	printf("Dumping WowObjects to file \"%s\"!\n", log_path.c_str());
	WowObject next = OM.get_first_object();
	GUID_t target_GUID = get_target_GUID();

	fprintf(fp, "local GUID = 0x%016llX, player target: %016llX, first object base = %p, next.next base = %p\n", OM.get_local_GUID(), target_GUID, next.get_base(), next.get_base());

	while (next.valid()) {
		if (!(next.get_type() == OBJECT_TYPE_ITEM || next.get_type() == OBJECT_TYPE_CONTAINER)) {  // filter out items and containers :P
			fprintf(fp, "object GUID: 0x%016llX, base addr = %X, type: %s\n", next.get_GUID(), next.get_base(), next.get_type_name().c_str());

			if (next.get_type() == OBJECT_TYPE_NPC || next.get_type() == OBJECT_TYPE_UNIT) {
				vec3 pos = next.get_pos();
				fprintf(fp, "coords = (%f, %f, %f), rot: %f\n", pos.x, pos.y, pos.z, next.get_rot());

				if (next.get_type() == OBJECT_TYPE_NPC) {
					fprintf(fp, "name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d\n\n", next.NPC_get_name().c_str(), next.NPC_get_health(), next.NPC_get_health_max(), next.NPC_get_target_GUID(), next.in_combat());
					fprintf(fp, "buffs (by spellID):\n");
					for (int n = 1; n <= 16; ++n) {
						uint spellID = next.NPC_get_buff(n);
						if (spellID) fprintf(fp, "%d: %u\n", n, spellID);
					}

					fprintf(fp, "----\ndebuffs (by spellID)\n");

					for (int n = 1; n <= 16; ++n) {
						uint spellID = next.NPC_get_debuff(n);
						if (spellID) fprintf(fp, "%d: %u\n", n, spellID);
					}

				
				}
				else if (next.get_type() == OBJECT_TYPE_UNIT) {
					fprintf(fp, "name: %s, health: %u/%u, target GUID: 0x%016llX, combat = %d\n", next.unit_get_name().c_str(), next.unit_get_health(), next.unit_get_health_max(), next.unit_get_target_GUID(), next.in_combat());
					fprintf(fp, "buffs (by spellID):\n");
					for (int n = 1; n <= 16; ++n) {
						uint spellID = next.unit_get_buff(n);
						if (spellID) fprintf(fp, "%d: %u\n", n, spellID);
					}

					fprintf(fp, "----\ndebuffs (by spellID)\n");

					for (int n = 1; n <= 16; ++n) {
						uint spellID = next.unit_get_debuff(n);
						if (spellID) fprintf(fp, "%d: %u\n", n, spellID);
					}

					fprintf(fp, "----\n");
				}
			}
			else if (next.get_type() == OBJECT_TYPE_DYNAMICOBJECT) {
				vec3 DO_pos = next.DO_get_pos();
				fprintf(fp, "position: (%f, %f, %f), spellID: %d\n\n", DO_pos.x, DO_pos.y, DO_pos.z, next.DO_get_spellID());
			}

		}
		next = next.getNextObject();
	}
	
	
	fclose(fp);

	return 1;
}
