#include <Windows.h>
#include <Shlobj.h> // for the function that gets the desktop directory path for current user

#include "opcodes.h"
#include "ctm.h"
#include "timer.h"

extern HWND wow_hWnd;
extern int afkjump_keyup_queued;

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

static int dump_wowobjects_to_log() {

	ObjectManager OM;

	char desktop_path[MAX_PATH];

	if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, desktop_path))) {
		printf("SHGetFolderPath for CSIDL_DESKTOPDIRECTORY failed, errcode %d\n", GetLastError());
		return 0;
	}

	static const std::string log_path = std::string(desktop_path) + "\\wodump.log";
	FILE *fp = fopen(log_path.c_str(), "w");

	if (fp) {
		printf("Dumping WowObjects to file \"%s\"!\n", log_path.c_str());
		WowObject next = OM.get_first_object();
		GUID_t target_GUID;
		readAddr(PLAYER_TARGET_ADDR, &target_GUID, sizeof(target_GUID));
		fprintf(fp, "local GUID = 0x%016llX, player target: %016llX\n", OM.get_local_GUID(), target_GUID);

		while (next.valid()) {

			if (!(next.get_type() == OBJECT_TYPE_ITEM || next.get_type() == OBJECT_TYPE_CONTAINER)) {  // filter out items and containers :P
				fprintf(fp, "object GUID: 0x%016llX, base addr = %X, type: %s\n", next.get_GUID(), next.get_base(), next.get_type_name().c_str());

				if (next.get_type() == OBJECT_TYPE_NPC || next.get_type() == OBJECT_TYPE_UNIT) {
					vec3 pos = next.get_pos();
					fprintf(fp, "coords = (%f, %f, %f), rot: %f\n", pos.x, pos.y, pos.z, next.get_rot());

					if (next.get_type() == OBJECT_TYPE_NPC) {
						fprintf(fp, "name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d, dead = %d, has_loot = %d\n\n", next.NPC_get_name().c_str(), next.NPC_getCurHealth(), next.NPC_getMaxHealth(), next.NPC_get_target_GUID(), next.in_combat(), next.unit_dead(), next.NPC_has_loot());
					
					}
					else if (next.get_type() == OBJECT_TYPE_UNIT) {
						fprintf(fp, "name: %s, target GUID: 0x%016llX, combat = %d\n", next.unit_get_name().c_str(), next.unit_get_target_GUID(), next.in_combat());
						fprintf(fp, "buffs (by spellID):\n");
						for (int n = 1; n <= 16; ++n) {
							int spellID = next.unit_get_buff(n);
							if (spellID) fprintf(fp, "%d: %u\n", n, spellID);
						}

						fprintf(fp, "----\ndebuffs (by spellID)\n");

						for (int n = 1; n <= 16; ++n) {
							int spellID = next.unit_get_debuff(n);
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

	}
	else {
		printf("Opening file \"%s\" failed!\n", log_path.c_str());
	}
	return 1;
}



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
	long spellID = strtoul(arg.c_str(), &endptr, 10);
}

static void LOP_hug_spell_object(const std::string &arg) {
	char *endptr;
	long spellID = strtoul(arg.c_str(), &endptr, 10);

	ObjectManager OM;

	printf("got spellID %d as arg!\n", spellID);

	auto objs = OM.get_spell_objects_with_spellID(spellID);

	if (objs.empty()) {
		ctm_unlock();
		return;
	}

	// just run to obj #1 for now

	click_to_move(objs[0].DO_get_pos(), CTM_MOVE, 0);

	ctm_lock();

}

static void LOP_spread(const std::string &arg) {

}

static void LOPDBG_dump(const std::string &arg) {
	dump_wowobjects_to_log();
}

static std::unordered_map<GUID_t, WowObject> corpses_to_loot;

static int loot_process_active = 0;
static Timer loot_timer;
static GUID_t trying_to_loot_GUID = 0;
static GUID_t distance_taken = 0;
static GUID_t previous_corpse = 0;

static void LOPDBG_loot_all(const std::string &arg) {
	ObjectManager OM;
	corpses_to_loot = OM.get_lootable_corpses_within_range(20);
	trying_to_loot_GUID = 0;
	loot_process_active = 1;
	distance_taken = 0;
	loot_timer.start();

	ctm_unlock();
}

static WowObject find_farthest_corpse() {

	ObjectManager OM;
	vec3 my_pos = OM.get_local_object().get_pos();

	if (corpses_to_loot.size() == 1) {
		return corpses_to_loot.begin()->second;
	}

	float max_dist;
	WowObject farthest = corpses_to_loot.begin()->second;
	max_dist = (my_pos - farthest.get_pos()).length();

	for (auto &o : corpses_to_loot) {
		float dist = (my_pos - o.second.get_pos()).length();

		if (dist > max_dist) {
			farthest = o.second;
			max_dist = dist;
		}
	}

	return farthest;

}

void loot_next() {
	if (!loot_process_active) { return; }

	if (ctm_locked()) { return; } // no point doing all this logic for nothing

	if (corpses_to_loot.empty()) {
		printf("loot_next: no corpses to loot!\n");
		loot_process_active = 0;
		return;
	}

	ObjectManager OM;
	//auto o = find_farthest_corpse();
	auto o = corpses_to_loot.begin()->second;

	WowObject om_o = OM.get_object_by_GUID(o.get_GUID());

	if (!om_o.valid()) {
		printf("it appears that the corpse with GUID %llX doesn't exist in the OM (in the world?). Moving on...\n", o.get_GUID());
		corpses_to_loot.erase(o.get_GUID());
		loot_timer.start();
		ctm_unlock();
		return;
	}

	if (loot_timer.get_ms() > 4000) {
		printf("Loot-confirmation for GUID %llX timed out (4000ms), skipping...\n", o.get_GUID());
		corpses_to_loot.erase(o.get_GUID());
		loot_timer.start();
		ctm_unlock();
		return;
	}

	if (trying_to_loot_GUID != o.get_GUID()) {
		printf("trying to loot corpse with GUID %llX...\n", o.get_GUID());
		trying_to_loot_GUID = o.get_GUID();
	}

	SelectUnit(o.get_GUID());
	click_to_move(o.get_pos(), CTM_LOOT, o.get_GUID(), 0.33); 
	// 0.5 is basically the lowest safe value for min_distance. anything lower than that is subject to all kinds of stupid things..

	ctm_lock();
}

static void LOPDBG_looted_GUID(const std::string &arg) {

	char *endptr;
	GUID_t GUID = strtoull(arg.c_str(), &endptr, 16);
	previous_corpse = GUID;
	corpses_to_loot.erase(GUID);
	printf("erasing corpse with GUID %llX from list\n", GUID);

	ObjectManager OM;

	auto o = OM.get_object_by_GUID(GUID);
	int looted_mask = 0x80000000; // XDD we'll see if this works
	writeAddr(DEREF(o.get_base() + 0x120), &looted_mask, sizeof(int));

	loot_timer.start();
	
	ctm_unlock();

	loot_next();
}

static void LOPDBG_query_injected(const std::string &arg) {
	DoString("SetCVar(\"screenshotQuality\", \"1\", \"inject\")"); // this is used to signal the addon that we're injected :D
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
	{ "LOLE_OPCODE_AVOID_SPELL_OBJECT", LOP_avoid_spell_object, 1 },
	{ "LOLE_OPCODE_HUG_SPELL_OBJECT", LOP_hug_spell_object, 1 },
	{ "LOLE_OPCODE_SPREAD", LOP_spread, 0 }
};

static const struct {
	std::string name;
	hubfunc_t func;
	uint num_args;
} debug_opcode_funcs[] = {
	{ "LOLE_DEBUG_NOP", LOP_nop, 0 },
	{ "LOLE_DEBUG_DUMP", LOPDBG_dump, 0 },
	{ "LOLE_DEBUG_LOOT_ALL", LOPDBG_loot_all, 0},
	{ "LOLE_DEBUG_QUERY_INJECTED", LOPDBG_query_injected, 0 },
	{ "LOLE_DEBUG_LOOTED_GUID", LOPDBG_looted_GUID, 1},

};

static const size_t num_opcode_funcs = sizeof(opcode_funcs) / sizeof(opcode_funcs[0]);
static const size_t num_debug_opcode_funcs = sizeof(debug_opcode_funcs) / sizeof(debug_opcode_funcs[0]);

// ----------------------
//	  public interface
// -----------------------

int opcode_call(int opcode, const std::string &arg) {
	
	if (opcode & 0x80) {
		int debug_op = opcode & 0x7F;
		if (debug_op > num_debug_opcode_funcs - 1) {
			printf("opcode_get_funcname: error: unknown debug opcode %lu. (valid range: 0 - %lu)\n", debug_op, num_debug_opcode_funcs);
			return 0;
		}
		debug_opcode_funcs[debug_op].func(arg);
		return 1;
	}

	if (opcode > num_opcode_funcs-1) {
		printf("opcode_call: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return 0;
	}

	opcode_funcs[opcode].func(arg);
	return 1;
}

int opcode_get_num_args(int opcode) {
	if (opcode & 0x80) {
		int debug_op = opcode & 0x7F;
		if (debug_op > num_debug_opcode_funcs - 1) {
			printf("opcode_get_funcname: error: unknown debug opcode %lu. (valid range: 0 - %lu)\n", debug_op, num_debug_opcode_funcs);
		}
		return debug_opcode_funcs[debug_op].num_args;
	}
	
	if (opcode > num_opcode_funcs - 1) {
		printf("opcode_get_num_args: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return -1;
	}

	return opcode_funcs[opcode].num_args;
}

const std::string &opcode_get_funcname(int opcode) {
	if (opcode & 0x80) {
		int debug_op = opcode & 0x7F;
		if (debug_op > num_debug_opcode_funcs - 1) {
			printf("opcode_get_funcname: error: unknown debug opcode %lu. (valid range: 0 - %lu)\n", debug_op, num_debug_opcode_funcs);
		}
		return debug_opcode_funcs[debug_op].name;
	}
	
	if (opcode > num_opcode_funcs - 1) {
		printf("opcode_get_funcname: error: unknown opcode %lu. (valid range: 0 - %lu)\n", opcode, num_opcode_funcs);
		return "ERROR";
	}
	return opcode_funcs[opcode].name;

}

// follow stuff

void refollow_if_needed() {
	if (!follow_state.close_enough) LOP_follow_GUID(follow_state.target_GUID);
}