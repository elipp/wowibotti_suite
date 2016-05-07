#include "ctm.h"

#include <queue>

static std::queue<CTM_t> ctm_queue;
static int ctm_locked = 0;

void ctm_add(const CTM_t &ctm) {
	if (ctm_queue.size() > 5) return;

	ctm_queue.push(ctm);
}

void ctm_act() {

	if (ctm_locked) return;

	auto &CTM = ctm_queue.front();
	click_to_move(CTM);

	ctm_lock();
}

void ctm_commit() {

	if (ctm_queue.empty()) return;
	
	ctm_unlock();

	ctm_act();
	ctm_queue.pop();
	
	ctm_lock();

}


static const uint
	CTM_X = 0xD68A18,
	CTM_Y = 0xD68A1C,
	CTM_Z = 0xD68A20,
	CTM_ACTION = 0xD689BC,
	CTM_GUID = 0xD689C0, // this is for interaction
	CTM_MOVE_ATTACK_ZERO = 0xD689CC,

	CTM_walking_angle = 0xD689A0,
	CTM_FL_A4 = 0xD689A4,
	CTM_FL_A8 = 0xD689A8,
	CTM_min_distance = 0xD689AC,

	CTM_increment = 0xD689B8,

	CTM_mystery_C8 = 0xD689C8,
	CTM_mystery_A90 = 0xD68A90,
	CTM_mystery_A94 = 0xD68A94;

int get_wow_CTM_state() {
	int state;
	readAddr(CTM_ACTION, &state, sizeof(state));
	return state;
}

void ctm_lock() {
	ctm_locked = 1;
}

void ctm_unlock() {
	ctm_locked = 0;
}

void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance) {

	if (ctm_locked) {
		return;
	}

	ObjectManager OM;

	auto p = OM.get_local_object();

	vec3 diff = p.get_pos() - point; // this is kinda weird, since usually one would take dest - current_loc, but np
	float directed_angle = atan2(diff.y, diff.x);

	writeAddr(CTM_walking_angle, &directed_angle, sizeof(directed_angle));


	static const uint
		ALL_9A4 = 0x415F66F3;

	static const float
		MOVE_9A8 = 0.25, // 0.25, don't really know what this is
		MOVE_MINDISTANCE = 0.5; // this is 0.5, minimum distance from exact point

	static const float
		FOLLOW_9A8 = 9.0,
		FOLLOW_MINDISTANCE = 3.0;

	static const float
		MOVE_AND_ATTACK_9A8 = 13.444444, // for M&A its something like 13.444
										 //	MOVE_AND_ATTACK_9AC = 0x406AAAAA; // 3.6666 (melee range?)
		MOVE_AND_ATTACK_MINDISTANCE = 1.5; // use 1.5 for this (test!) seems to work:P http://www.h-schmidt.net/FloatConverter/IEEE754.html

	static const float
		LOOT_9A8 = 13.4444444,
		LOOT_MINDISTANCE = 3.6666666;

	float float_9A8, min_dist;
	GUID_t interact;

	switch (action) {
	case CTM_MOVE:
		float_9A8 = MOVE_9A8;
		min_dist = min_distance == 0 ? MOVE_MINDISTANCE : min_distance;
		interact = 0;
		break;

	case CTM_FOLLOW:
		float_9A8 = FOLLOW_9A8;
		min_dist = min_distance == 0 ? FOLLOW_MINDISTANCE : min_distance;
		interact = 0;
		break;

	case CTM_MOVE_AND_ATTACK:
		float_9A8 = MOVE_AND_ATTACK_9A8;
		min_dist = min_distance == 0 ? MOVE_AND_ATTACK_MINDISTANCE : min_distance;
		interact = interact_GUID;
		break;


	case CTM_LOOT:
		float_9A8 = LOOT_9A8;
		min_dist = min_distance == 0 ? LOOT_MINDISTANCE : min_distance;
		interact = interact_GUID;
		break;

	default:
		float_9A8 = MOVE_9A8;
		min_dist = 0.5;
		interact = 0;
		break;
	}

	writeAddr(CTM_FL_A4, &ALL_9A4, sizeof(ALL_9A4));
	writeAddr(CTM_FL_A8, &float_9A8, sizeof(float));
	writeAddr(CTM_min_distance, &min_dist, sizeof(float));
	writeAddr(CTM_GUID, &interact, sizeof(GUID_t));

	// the value of 0xD689B8 seems to be incremented with every step CTM'd, but it seems to be ok like this (modify once, after that keeps track of itself)

	float increment = 0;
	readAddr(CTM_increment, &increment, sizeof(increment));

	if (increment == 0) {
		static const float B8 = 0.01;
		writeAddr(CTM_increment, &B8, sizeof(B8));
	}

	// 0xD689C8 usually gets the value 2.
	static const uint C8 = 0x2;
	writeAddr(CTM_mystery_C8, &C8, sizeof(C8));

	// 0xD68A0C:14 contain the player's current position, probably updated even when the CTM is not legit??

	// 0xD68A90 is a constant 0x3F7FF605 (float ~0.9998, or perhaps not a float at all. who knows?)
	static const uint A90 = 0x3F7FF605;
	static const uint A94 = 0x1;

	writeAddr(CTM_mystery_A90, &A90, sizeof(A90));
	writeAddr(CTM_mystery_A94, &A94, sizeof(A94));

	writeAddr(CTM_X, &point.x, sizeof(point.x));
	writeAddr(CTM_Y, &point.y, sizeof(point.y));
	writeAddr(CTM_Z, &point.z, sizeof(point.z));

	writeAddr(CTM_ACTION, &action, sizeof(action));

}


void click_to_move(const CTM_t& c) {
	click_to_move(c.destination, c.action, c.interact_GUID, c.min_distance);
}


// CTM notes:


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