// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#include <Shlobj.h>


#include <D3D9.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>

#include "addrs.h"
#include "defs.h"
#include "ObjectManager.h"
#include "hooks.h"

#include "timer.h"

#include "window.h"

int const (*LUA_DoString)(const char*, const char*, const char*) = (int const(*)(const char*, const char*, const char*)) LUA_DoString_addr;
int const (*SelectUnit)(GUID_t) = (int const(*)(GUID_t)) SelectUnit_addr;

HINSTANCE  inj_hModule;          // HANDLE for injected module

HANDLE glhProcess;

static point_timestamp pt;

static int face_queued = 0;

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

static BYTE original_opcodes[8];

static int BLAST_ENABLED = 0;
static int CONSTANT_CTM_TARGET = 0;

static void __stdcall walk_to_target();



void __stdcall print_errcode(int code) {
	printf("Spell cast failed, errcode %d\n", code);
}

static int patch_LUA_prot(HANDLE hProcess) {
	printf("Patching LUA protection function...");
	static LPVOID lua_prot_addr = (LPVOID)0x49DBA0;
	static BYTE patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
	WriteProcessMemory(hProcess, lua_prot_addr, patch, sizeof(patch), NULL);
	printf("OK!\n");
	return 1;
}

void DoString(const char* format, ...) {
	char cmd[1024];

	va_list args;
	va_start(args, format);
	vsprintf(cmd, format, args);
	va_end(args);

	LUA_DoString(cmd, cmd, "");
}

static void follow_unit_with_GUID(const std::string& arg);

// this __declspec(noinline) thing has got to do with the msvc optimizer.
// seems like the inline assembly is discarded when this func is inlined, in which case were fucked
__declspec(noinline) static void set_facing(float x) {
	void const (*SetFacing)(float) = (void const (*)(float))0x007B9DE0;

	uint this_ecx = DEREF(0xE29D28) + 0xBE0;

	// printf("set_facing: this_ecx: %X\n", this_ecx);
	// this is due to the fact that SetFacing is uses a __thiscall calling convention, 
	// so the base addr needs to be passed in ECX. no idea which base address this is though,
	// but it seems to be constant enough :D

	__asm push ecx
	__asm mov ecx, this_ecx
	SetFacing(x);
	__asm pop ecx;

}

static void click_to_move(vec3 point, uint action, GUID_t interact_GUID) {
	static const uint
		CTM_X = 0xD68A18,
		CTM_Y = 0xD68A1C,
		CTM_Z = 0xD68A20,
		CTM_push = 0xD689BC,
		CTM_GUID = 0xD689C0, // this is for interaction
		CTM_MOVE_ATTACK_ZERO = 0xD689CC,

		CTM_walking_angle = 0xD689A0,
		CTM_const_float_9A4 = 0xD689A4, 
		CTM_const_float_9A8 = 0xD689A8,
		CTM_const_float_9AC = 0xD689AC; 

	// at least for CTM_MOVE and CTM_MOVE_AND_ATTACK, D689AC contains the minimum distance you need to be 
	// until you're considered done with the CTM. 9A8 is something i've not yet figured out, but it doesn't seem to really affect anything

	// CTM_MOVE_ATTACK_ZERO must be 0 for at least CTM_MOVE_AND_ATTACK, otherwise segfault
	// explanation: the function 7BCB00 segfaults at 7BCB14, if it's not
	// can't remember which function calls 7BCB00, but the branch
	// isn't taken when there's a 0 at D689CC. :D

	// at Wow.exe:612A53 we can see that when the player is done CTMing,
	// addresses D689C0-D689C8, D68998, D6899C are set to 0.0, and CTM_action to 0D


	// seems like addresses D689A0 D689A4 D689A8 are floats, maybe some angles?
	// A0 = the angle where the character should be walking?
	// D689A8 and D689AC are weird..
	// for walking, "mystery" is a constant float 0.5

	// based on my testing, the addresses that change upon a legit CTM are:
	// D6896C, D689A0:AC, D689B8:BC, D689C8, D68A0C:20, D68A90:94

	// here's a memdump diff for evidence (range was D68800 - D68B00 i think):

	/* $ diff pre-ctm.txt mid-ctm.txt
		94c94
		< 00D6896C   00000000
		-- -
		> 00D6896C   40E00000
		107, 110c107, 110
		< 00D689A0   00000000
		< 00D689A4   00000000
		< 00D689A8   00000000
		< 00D689AC   00000000
		-- -
		> 00D689A0   3F588DE2
		> 00D689A4   415F66F3
		> 00D689A8   3E800000
		> 00D689AC   3F000000
		113, 114c113, 114
		< 00D689B8   00000000
		< 00D689BC   0000000D
		-- -
		> 00D689B8   3D3E07D9
		> 00D689BC   00000004
		117c117
		< 00D689C8   00000000
		-- -
		> 00D689C8   00000002
		134, 139c134, 139
		< 00D68A0C   00000000
		< 00D68A10   00000000
		< 00D68A14   00000000
		< 00D68A18   00000000
		< 00D68A1C   00000000
		< 00D68A20   00000000
		-- -
		> 00D68A0C   C3F6F64F
		> 00D68A10   C584BEC0
		> 00D68A14   423572C4
		> 00D68A18   C3F5789C
		> 00D68A1C   C584A3D1
		> 00D68A20   4239D718
		167, 168c167, 168
		< 00D68A90   00000000
		< 00D68A94   00000000
		-- -
		> 00D68A90   3F7FF605
		> 00D68A94   00000001

	*/

	// D689A0 is the walking direction.

	ObjectManager OM;

	auto p = OM.get_object_by_GUID(OM.get_localGUID());

	vec3 diff = p.get_pos() - point; // this is kinda weird, since usually one would take dest - current_loc, but np
	float directed_angle = atan2(diff.y, diff.x);

	writeAddr(CTM_walking_angle, &directed_angle, sizeof(directed_angle));


	static const uint
		ALL_9A4 = 0x415F66F3;

	static const uint
		MOVE_9A8 = 0x3E800000, // 0.25, don't really know what this is
		MOVE_9AC = 0x3F000000; // this is 0.5, minimum distance from exact point

	static const uint
		FOLLOW_9A8 = 0x41100000,
		FOLLOW_9AC = 0x40400000;

	static const uint
		MOVE_AND_ATTACK_9A8 = 0x41571C71, // for M&A its something like 13.444
	//	MOVE_AND_ATTACK_9AC = 0x406AAAAA; // 3.6666 (melee range?)
		MOVE_AND_ATTACK_9AC = 0x3FC00000; // use 1.5 for this (test!) seems to work:P http://www.h-schmidt.net/FloatConverter/IEEE754.html

	writeAddr(CTM_const_float_9A4, &ALL_9A4, sizeof(ALL_9A4));

	if (action == CTM_MOVE) {
		writeAddr(CTM_const_float_9A8, &MOVE_9A8, sizeof(MOVE_9A8));
		writeAddr(CTM_const_float_9AC, &MOVE_9AC, sizeof(MOVE_9AC));
	}

	else if (action == CTM_FOLLOW) {
		writeAddr(CTM_const_float_9A8, &FOLLOW_9A8, sizeof(FOLLOW_9A8));
		writeAddr(CTM_const_float_9AC, &FOLLOW_9AC, sizeof(FOLLOW_9AC));
	}
	else if (action == CTM_MOVE_AND_ATTACK) {
		if (interact_GUID != 0) {
			const uint zero = 0;
			writeAddr(CTM_GUID, &interact_GUID, sizeof(GUID));
			writeAddr(CTM_MOVE_ATTACK_ZERO, &zero, sizeof(zero));
		}
		writeAddr(CTM_const_float_9A8, &MOVE_AND_ATTACK_9A8, sizeof(MOVE_AND_ATTACK_9A8));
		writeAddr(CTM_const_float_9AC, &MOVE_AND_ATTACK_9AC, sizeof(MOVE_AND_ATTACK_9AC));
	}


	// the value of 0xD689B8 seems to be incremented with every step CTM'd, but it seems to be working after the first 

	float b8val = 0;
	readAddr(0xD689B8, &b8val, sizeof(b8val));

	if (b8val == 0) {
		static const float B8 = 0.01;
		writeAddr(0xD689B8, &B8, sizeof(B8));
	}

	// 0xD689C8 usually gets the value 2.
	static const uint C8 = 0x2;
	writeAddr(0xD689C8, &C8, sizeof(C8));

	// 0xD68A0C:14 contain the player's current position, probably updated even when the CTM is not legit??

	// 0xD68A90 is a constant 0x3F7FF605 (float ~0.9998, or perhaps not a float at all. who knows?)
	static const uint A90 = 0x3F7FF605;
	static const uint A94 = 0x1;

	writeAddr(0xD68A90, &A90, sizeof(A90));
	writeAddr(0xD68A94, &A94, sizeof(A94));

	writeAddr(CTM_X, &point.x, sizeof(point.x));
	writeAddr(CTM_Y, &point.y, sizeof(point.y));
	writeAddr(CTM_Z, &point.z, sizeof(point.z));
	
	writeAddr(CTM_push, &action, sizeof(action));

}

static GUID_t get_raid_target_GUID(int index) {
	GUID_t GUID;
	readAddr(0xC6F700 + index * 8, &GUID, sizeof(GUID)); // these are stored at the static address C6F700 until C6F764
	return GUID;
}

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
		WowObject next = OM.getFirstObject();
		GUID_t target_GUID;
		readAddr(PLAYER_TARGET_ADDR, &target_GUID, sizeof(target_GUID));
		fprintf(fp, "local GUID = 0x%016llX, player target: %016llX\n", OM.get_localGUID(), target_GUID);

		while (next.valid()) {

			if (!(next.get_type() == OBJECT_TYPE_ITEM || next.get_type() == OBJECT_TYPE_CONTAINER)) {  // filter out items and containers :P
				fprintf(fp, "object GUID: 0x%016llX, base addr = %X, type: %s\n", next.get_GUID(), next.get_base(), next.get_type_name().c_str());
				
				if (next.get_type() == OBJECT_TYPE_NPC || next.get_type() == OBJECT_TYPE_UNIT) { 
					vec3 pos = next.get_pos();
					 fprintf(fp, "coords = (%f, %f, %f), rot: %f\n", pos.x, pos.y, pos.z, next.get_rot());

					if (next.get_type() == OBJECT_TYPE_NPC) {
						fprintf(fp, "name: %s, health: %d/%d, target GUID: 0x%016llX, combat = %d\n\n", next.NPC_get_name().c_str(), next.NPC_getCurHealth(), next.NPC_getMaxHealth(), next.NPC_get_target_GUID(), next.in_combat());
					}
					else if (next.get_type() == OBJECT_TYPE_UNIT) {
						fprintf(fp, "name: %s, target GUID: 0x%016llX, combat = %d\n", next.unit_get_name().c_str(), next.unit_get_target_GUID(), next.in_combat());
						fprintf(fp, "buffs:\n");
						for (int n = 1; n <= 16; ++n) {
							int spellID = next.unit_get_buff(n);
							if (spellID) fprintf(fp, "%d: spellID = %u\n", n, spellID);
						}
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

static void dump_wrap_hubfunc(const std::string &arg) {
	dump_wowobjects_to_log();
}

static void __stdcall walk_to_target() {

	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	click_to_move(t.get_pos(), CTM_MOVE, 0);

}

static void __stdcall face_target() {

	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return;

	vec3 diff = t.get_pos() - p.get_pos();
	click_to_move(p.get_pos() + 0.3*diff.unit(), CTM_MOVE, 0); 
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

static void __stdcall melee_behind_target() {
	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return;

	float target_rot = t.get_rot();

	// basically rotating the vector (1, 0, 0) target_rot radians anti-clockwise
	//vec3 rot_unit(1.0 * std::cos(target_rot) - 0.0 * std::sin(target_rot), 1.0 * std::sin(target_rot) + 0.0 * std::cos(target_rot), 0.0);
	
	vec3 rot_unit = vec3(std::cos(target_rot), std::sin(target_rot), 0.0);
	vec3 pc = p.get_pos();
	vec3 tc = t.get_pos();

	//vec3 point_behind(tc.x - std::cos(target_rot), tc.y - std::sin(target_rot), tc.z);
	vec3 point_behind = tc - rot_unit;
	vec3 diff = pc - point_behind;

	//printf("melee_behind.. diff.length: %f\n", diff.length());

	if (diff.length() < 0.3) { // 1.0 sometimes results in some pretty weird shit.. :D
		face_queued = 1; 
		return;
	}
	else {
		click_to_move(point_behind, CTM_MOVE_AND_ATTACK, t.get_GUID());
	}

}


static void __stdcall broadcast_CTM(float *coords, int action) {

	float x, y, z;

	x = coords[0];
	y = coords[1]; 
	z = coords[2]; 

	printf("broadcast_CTM: got CTM coords: (%f, %f %f), action = %d\n", x, y, z, action);

	char sprintf_buf[128];

	sprintf(sprintf_buf, "%.1f,%.1f,%.1f", x, y, z);

	// the CTM mode is determined in the LUA logic, in the subcommand handler.

	DoString("RunMacroText(\"/lole ctm %s\")", sprintf_buf); 
}

static int player_is_moving() {


	ObjectManager OM;
	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return -1;

	static const float normal_speed_per_tick = 2.18e-6;

	point_timestamp new_pt = p.get_pos();

	LONGLONG tick_diff = new_pt.ticks.QuadPart - pt.ticks.QuadPart;

	//if (tick_diff > 500000) { // on my machine, 200000 ticks is (roughly) every third frame, so about 46ms. 
		// so basically, if the player hasn't moved enough in about 100ms, it can be considered stationary
		// 100% run speed is about 2.18e-6 units ("yards") per tick

	vec3 diff = new_pt.p - pt.p;

	// 0.47 / 216000 is roughly walking speed

	if (diff.length() > 0.15) { return 1; }

	pt = new_pt;

	return 1;
}


static void __stdcall every_frame_hook_func() {

	static int every_third_frame = 0;

	if (every_third_frame == 0) {
		//if (BLAST_ENABLED) {
			//DoString("RunMacroText(\"/lole\")");
		//}
		if (CONSTANT_CTM_TARGET) {
			walk_to_target();
		}

		if (face_queued) { // this is a trick to not mess up the main thread
			// seems to me that the setfacing function is only local, doesn't inform the server of the new orientation
			face_target();
			face_queued = 0;
		}
	}

	if (!follow_state.close_enough) {
		follow_unit_with_GUID(follow_state.target_GUID); // this has a timeout of 10s.
	}

	every_third_frame = every_third_frame > 2 ? 0 : every_third_frame + 1;


}


void change_target(const std::string &arg) {

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

static void blast(const std::string &arg) {
	if (arg == "1") {
		printf("got BLAST_ON AddonMessage, enabling blast.\n");
		//BLAST_ENABLED = 1;
	}
	else if (arg == "0") {
		printf("got BLAST_OFF AddonMessage, disabling blast.\n");
		//BLAST_ENABLED = 0;
	}
	else {
		printf("blast (from DelIgnore_hub): warning: unknown argument \"%s\"\n", arg.c_str());
		//BLAST_ENABLED = 0;
	}
}

static void move_into_casting_range(const std::string& arg) {
	GUID_t target_GUID = *(GUID_t*)PLAYER_TARGET_ADDR;
	if (!target_GUID) return;

	ObjectManager OM;
	WowObject t = OM.get_object_by_GUID(target_GUID);

	if (!t.valid()) return;

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) return;

	vec3 ppos = p.get_pos();
	vec3 tpos = t.get_pos();

	vec3 diff = tpos - ppos;
	
	char *endptr;
	float minrange = strtof(arg.c_str(), &endptr);

	if (diff.length() > minrange-1) {
		// move in a straight line to a distance of 29 yd from the target. Kinda bug-prone though..
		vec3 new_point = tpos - (minrange - 0.5) * diff.unit(); 
		click_to_move(new_point, CTM_MOVE, 0x0);
		return;
	}
}

static void follow_unit_with_GUID(const std::string& arg) {
	// the arg should contain host character GUID
	// CONSIDER: change this to straight up player name and implement a get WowObject by name func?
	ObjectManager OM;

	char *endptr;
	GUID_t GUID = strtoull(arg.c_str(), &endptr, 16);

	WowObject p = OM.get_object_by_GUID(OM.get_localGUID());

	if (!p.valid()) {
		printf("follow_unit_with_GUID: LOLE_OPCODE_FOLLOW: getting local object failed? WTF? XD\n");
		return;
	}

	if (GUID == 0) {
		float prot = p.get_rot();
		vec3 rot_unit = vec3(std::cos(prot), std::sin(prot), 0.0);
		click_to_move(p.get_pos() + 0.51*rot_unit, CTM_MOVE, 0);
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
		//DoString("FollowUnit(\"" + o.unit_get_name() + "\")");
		printf("follow difference < 10! calling WOWAPI FollowUnit()\n");
		DoString("FollowUnit(\"%s\")", o.unit_get_name().c_str());
		follow_state.clear();
	}
	else {
		follow_state.start(arg);
	}


	if (follow_state.timer.get_s() > 10) {
		follow_state.clear();
	}

	return;

}

static void caster_face_target(const std::string &arg) {
	face_queued = 1;
}

static void act_on_CTM_broadcast(const std::string &arg) {
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

static void lole_nop(const std::string& arg) {
	return;
}

static void lole_dungeon_script(const std::string &arg) {

}

typedef void(*hubfunc_t)(const std::string &);

// lole opcodes. also defined in lole.lua (let's just hope they match XD)
// turns this shit into a nice jump table

#define LOLE_OPCODE_NOP 0x0 
#define LOLE_OPCODE_TARGET_GUID 0x1
#define LOLE_OPCODE_BLAST 0x2
#define LOLE_OPCODE_CASTER_RANGE_CHECK 0x3
#define LOLE_OPCODE_GATHER_FOLLOW 0x4
#define LOLE_OPCODE_CASTER_FACE 0x5
#define LOLE_OPCODE_CTM_BROADCAST 0x6
#define LOLE_OPCODE_COOLDOWNS 0x7
#define LOLE_OPCODE_CC 0x8
#define LOLE_OPCODE_DUNGEON_SCRIPT 0x9

#define LOLE_DEBUG_OPCODE_DUMP 0xF1

static const struct {
	std::string name;
	hubfunc_t func;
	uint num_args;
} hubfuncs[] = {
	{"LOLE_NOP", lole_nop, 0},
	{"LOLE_TARGET_GUID", change_target, 1},
	{"LOLE_BLAST", blast, 1},
	{"LOLE_CASTER_RANGE_CHECK", move_into_casting_range, 1},
	{"LOLE_FOLLOW", follow_unit_with_GUID, 1},
	{"LOLE_CASTER_FACE", caster_face_target, 0},
	{"LOLE_CTM_BROADCAST", act_on_CTM_broadcast, 3},
	{"LOLE_COOLDOWNS", lole_nop, 3}, // nyi
	{"LOLE_CC", lole_nop, 3},
	{"LOLE_DUNGEON_SCRIPT", lole_dungeon_script, 1}
};

static const struct {
	std::string name;
	hubfunc_t func;
	uint num_args;
} debug_hubfuncs[] = {
	{ "LOLE_NOP", lole_nop, 0 },
	{ "LOLE_DEBUG_DUMP", dump_wrap_hubfunc, 0 }
};

static const size_t num_hubfuncs = sizeof(hubfuncs) / sizeof(hubfuncs[0]);

static void __stdcall DelIgnore_hub(const char* arg_) {
	// the opcodes sent by the addon all begin with the sequence "LOP_"
	static const std::string opcode_prefix("LOP_");

	const std::string arg(arg_);

	if (strncmp(arg.c_str(), opcode_prefix.c_str(), opcode_prefix.length()) != 0) {
		printf("DelIgnore_hub: invalid opcode \"%s\": DelIgnore_hub opcodes must begin with the sequence \"%s\"\n", arg_, opcode_prefix.c_str());
		return;
	}

	if (arg.length() < 6) {
		printf("DelIgnore_hub: invalid opcode \"%s\"\n", arg.c_str());
		return;
	}

	std::string opstr = arg.substr(4, 2);
	char *endptr;
	unsigned long op = strtoul(opstr.c_str(), &endptr, 16);

	if (op & 0x80) {
		int debug_op = op & 0x7F;
		debug_hubfuncs[debug_op].func(arg);
		printf("DelIgnore_hub: got DEBUG opcode %lu -> %s\n", op, debug_hubfuncs[debug_op].name.c_str());
		return;
	}

	if (op > num_hubfuncs - 1) {
		printf("DelIgnore_hub: error: unknown opcode %lu. (valid range: 0 - %lu)\n", op, num_hubfuncs);
		return;
	}

	auto func = hubfuncs[op];

	printf("DelIgnore_hub: got opcode %lu -> %s\n", op, func.name.c_str());

	if (func.num_args > 0) {
		// then we expect to find a ':' and arguments separated by ',':s
		size_t pos;
		if ((pos = arg.find(':', 5)) == std::string::npos) {
			printf("DelIgnore_hub: error: opcode %lu (%s) expects %d arguments, none found! (did you forget ':'?)\n",
				op, func.name.c_str(), func.num_args);
			return;
		}
		std::string args = arg.substr(pos + 1);
		printf("DelIgnore_hub: calling func %s with args \"%s\"\n", func.name.c_str(), args.c_str());

		func.func(args); // call the func with args
	}
	else {
		func.func("");
	}


}


static int hook_all() {
	
	install_hook("EndScene", every_frame_hook_func);
	install_hook("DelIgnore", DelIgnore_hub);
	install_hook("ClosePetStables", melee_behind_target);
	//install_hook("CTM_aux", broadcast_CTM);
	install_hook("CTM_main", broadcast_CTM);
	
	return 1;
}

static int unhook_all() {
	
	uninstall_hook("EndScene");
	uninstall_hook("DelIgnore");
	uninstall_hook("ClosePetStables");
	
	return 1;
}


DWORD WINAPI ThreadProc(LPVOID lpParam) {
	MSG messages;
	char *pString = reinterpret_cast<char *> (lpParam);
	HMENU hMenu = CreateDLLWindowMenu();
	RegisterDLLWindowClass("DLLWindowClass", inj_hModule);

	//HWND hwnd = CreateWindowEx(0, "DLLWindowClass", pString, WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, hMenu, inj_hModule, NULL);
	//ShowWindow(hwnd, SW_SHOWNORMAL);

	hook_all();

	//if (RegisterHotKey(hwnd, 100, MOD_ALT, 'G')) {
		//printf("Registered window %X as blast client!\n", (DWORD)hwnd);
	//}

	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 1;
}


LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	int lo, hi;
	switch (message) {

	case WM_COMMAND:
		switch (wParam)
		{
		case MYMENU_EXIT:
			unhook_all();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case MYMENU_DUMP:
			dump_wowobjects_to_log();
			break;
		case MYMENU_HOOK:
			hook_all();
			break;

		case MYMENU_UNHOOK:
			unhook_all();
			break;
		case MYMENU_CTM: {

			ObjectManager OM;
			WowObject o = OM.get_object_by_GUID(OM.get_localGUID());
			if (!o.valid()) {
				break;
			}

			WowObject t = OM.get_object_by_GUID(o.unit_get_target_GUID());
			if (!t.valid()) {
				break;
			}
			click_to_move(t.get_pos(), CTM_MOVE_AND_ATTACK, t.get_GUID());
			break;
		}
		case MYMENU_CTM_CONSTANT: {
			CONSTANT_CTM_TARGET = !CONSTANT_CTM_TARGET;
			printf("CONSTANT_CTM: %d\n", CONSTANT_CTM_TARGET);
			break;
		}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	HANDLE hProcess = GetCurrentProcess();
	HANDLE windowThread = INVALID_HANDLE_VALUE;

	glhProcess = hProcess;

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		
		patch_LUA_prot(hProcess);
		//hook_all();
		windowThread = CreateThread(0, NULL, ThreadProc, (LPVOID)"Dump", NULL, NULL);
		inj_hModule = hModule;

		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		printf("DLL DETACHED! Unhooking all functions.\n");

		PostThreadMessage(GetThreadId(windowThread), WM_DESTROY, 0, 0);
		//WaitForSingleObject(windowThread, INFINITE);
		unhook_all();

		break;
	}
	//logfile.close();
	return TRUE;
}

