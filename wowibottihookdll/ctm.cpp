#include "ctm.h"

#include <queue>

static std::queue<CTM_t> ctm_queue;
static int ctm_locked = 0;

static int ctm_posthook_delay_active() {
	if (ctm_queue.size() < 1) { return 0; }

	CTM_t *c = ctm_get_current_action();
	if (!c) return 0;
	if (!c->get_current_posthook()) return 0;

	PRINT("posthook->active: %d\n", c->get_current_posthook()->active);

	if (c->get_current_posthook()->active) return 1;

	return 0;
}


void ctm_add(const CTM_t &ctm) {
	if (ctm_queue.size() > 5) return;

	if (ctm.priority == CTM_PRIO_EXCLUSIVE) {
		ctm_queue = std::queue<CTM_t>();
		ctm_queue.push(ctm);
		ctm_unlock();
		ctm_act();
	}
	else {
		if (ctm_queue.size() > 0) {
			const CTM_t &c = ctm_queue.front();
			if (c.priority == CTM_PRIO_EXCLUSIVE) return;
		}
		
		PRINT("called ctm_ADD with %.3f, %.3f, %.3f, ID=%ld, prio %d, action 0x%X\n", ctm.destination.x, ctm.destination.y, ctm.destination.z, ctm.ID, ctm.priority, ctm.action);
		ctm_queue.push(ctm);
	}
}

void ctm_act() {

	if (ctm_locked) return;
	if (ctm_queue.empty()) return;
	if (ctm_posthook_delay_active()) return;

	auto &ctm = ctm_queue.front();

	PRINT("called ctm_act with %.3f, %.3f, %.3f, ID=%ld prio %d, action 0x%X\n", ctm.destination.x, ctm.destination.y, ctm.destination.z, ctm.ID, ctm.priority, ctm.action);

	click_to_move(ctm);

	ctm_lock();
}

void ctm_next() {

	if (ctm_queue.empty()) return;
	
	ctm_unlock();
	ctm_pop();

	PRINT("called ctm_next(), ctm_queue.size() = %u\n", ctm_queue.size());

}

CTM_t ctm_pop() {
	CTM_t c = ctm_queue.front();
	ctm_queue.pop();
	return c;
}


CTM_t *ctm_get_current_action() {
	if (ctm_queue.size() < 1) { return NULL;  }
	return &ctm_queue.front();
}


int ctm_handle_delayed_posthook() {
	if (ctm_posthook_delay_active()) {
		PRINT("at ctm_handle_delayed_posthook()\n");

		CTM_t &c = ctm_queue.front();
		CTM_posthook_t *h = c.get_current_posthook();

		PRINT("CTM id: %ld\n", c.ID);

		if (h->frame_counter < h->delay_frames) {
			++h->frame_counter;
			return 1;
		}
		else {
			h->active = 0;
			h->callback();
			PRINT("called posthook callback after %d frames!\n", h->delay_frames);
			
			if (!c.hook_next()) { // this also increments the iterator (crap?)
				ctm_next();
			}
			
			return 1;
		}
	}

	return 0;
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

	WowObject p;
	if (!OM.get_local_object(&p)) return;

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
