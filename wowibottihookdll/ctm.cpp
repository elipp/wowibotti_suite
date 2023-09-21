#include "ctm.h"
#include "timer.h"
#include "opcodes.h"

#include <queue>

static std::queue<CTM_t> ctm_queue;
static int ctm_locked = 0;

CTM_posthook_t::CTM_posthook_t(CTM_callback_t hookfunc, void* hookfunc_arg, size_t arg_size, float delay_milliseconds) :
	callback(hookfunc), delay_ms(delay_milliseconds), argument{ 0 }, argument_size(0), active(0) {
	timestamp.start();
	delay_ms = delay_milliseconds;

	if (hookfunc_arg) {
		memcpy(argument, hookfunc_arg, arg_size);
		argument_size = arg_size;
	}
	else {
		hookfunc_arg = NULL;
		argument_size = 0;
	}
};

CTM_posthook_t::CTM_posthook_t() : callback(NULL), delay_ms(0), active(0) {}


void ctm_queue_reset() {
	ctm_queue = std::queue<CTM_t>();
	ctm_unlock();
}

static vec3 CTM_direction;

static struct {
	vec3 startpos;
	Timer timer;
	vec3 get_estimate() {
		// assume a minimum of 3 units / sec. normal walking speed would be roughly 6.5-7.5 units/s
		return 4.0 * timer.get_s() * CTM_direction + startpos;
	}
	bool is_moving(const vec3 &curpos) {
		vec3 diff = get_estimate() - curpos;
		float d = dot(diff, CTM_direction);
	
		if (d < 0) {
			// then we're basically ahead of the estimate so all is good, the startpos can be reset
			timer.start();
			startpos = curpos;
			return true;
		}
		else if (diff.length() > 3) {
			return false;
		}

		return true;
	}

} CTM_START;


static int ctm_posthook_delay_active() {
	if (ctm_queue.empty()) { return 0; }

	CTM_t *c = ctm_get_current_action();
	if (!c) return 0;
	if (!c->get_current_posthook()) return 0;

	//PRINT("posthook->active: %d\n", c->get_current_posthook()->active);

	if (c->get_current_posthook()->active) return 1;

	return 0;
}

static void ctm_queue_reinit_with(const CTM_t &new_ctm) {
	ctm_queue_reset();
	ctm_queue.push(new_ctm);
}

int ctm_queue_get_top_prio() {
	if (!ctm_queue.empty()) {
		const CTM_t &top = ctm_queue.front();
		return top.priority;
	}
	else {
		return CTM_PRIO_NONE;
	}
}

int ctm_job_in_progress() {
	return ctm_queue_get_top_prio() != CTM_PRIO_NONE && get_wow_CTM_state() != CTM_DONE;
}


void ctm_add(const CTM_t &new_ctm) {


	int topprio = ctm_queue_get_top_prio();

	if (topprio == CTM_PRIO_NOOVERRIDE) {
		PRINT("CTM_PRIO_NOOVERRIDE set, not overriding\n");
		return;
	}

	PRINT("called ctm_ADD with %.1f, %.1f, %.1f, ID=%ld, prio %d, action 0x%X\n",
		new_ctm.destination.x, new_ctm.destination.y, new_ctm.destination.z, new_ctm.ID, new_ctm.priority, new_ctm.action);

	if (new_ctm.priority == CTM_PRIO_LOW && topprio == CTM_PRIO_LOW) {
		ctm_queue.push(new_ctm);
	}
	
	else if (topprio <= new_ctm.priority) {
		ctm_queue_reinit_with(new_ctm);
		ctm_act();
	}

}

void ctm_act() {

	if (ctm_locked) return;
	if (ctm_queue.empty()) return;
	if (ctm_posthook_delay_active()) return;

	ObjectManager OM;
	auto p = OM.get_local_object();
	if (!p) return;

	CTM_t &ctm = ctm_queue.front();
	PRINT("called ctm_act with %.3f, %.3f, %.3f, ID=%ld prio %d, action 0x%X\n", ctm.destination.x, ctm.destination.y, ctm.destination.z, ctm.ID, ctm.priority, ctm.action);

	CTM_direction = ctm.destination - p->get_pos();

	if (CTM_direction.length() > 0.01) {
		CTM_direction = CTM_direction.unit();
	}
	CTM_START.startpos = p->get_pos();
	CTM_START.timer.start();

	PRINT("CTM_direction: (%.3f, %.3f, %.3f)\n", CTM_direction.x, CTM_direction.y, CTM_direction.z);

	click_to_move(ctm);

	ctm_lock();
}

void ctm_abort_if_not_moving() {

	if (!ctm_job_in_progress()) { return; }

	ObjectManager OM;
	auto p = OM.get_local_object();
	if (!p) return;

	if (!CTM_START.is_moving(p->get_pos())) {
		PRINT("ctm_next(): determined that we have not been moving, aborting CTM task!\n");
	//	DoString("SendChatMessage(\"I'm stuck, halp plx!\", \"GUILD\")");
		ctm_cancel();
	}
	
}


void ctm_next() {

	if (ctm_queue.empty()) return;
	
	CTM_t &ctm = ctm_queue.front();

	ObjectManager OM;
	auto p = OM.get_local_object();
	if (!p) { return; }

	vec3 ppos = p->get_pos();
	float dist = (ppos - ctm.destination).length();
	if (dist > ctm.min_distance + 1) {
		PRINT("ctm_next() called, but we're not actually within 1 yd of the destination point, retrying...\n");
		ctm_unlock();
		ctm_act(); // refresh
	}
	else {
		PRINT("called ctm_next(), ctm_queue.size() = %u\n", ctm_queue.size());
		ctm_unlock();
		ctm_pop();
	}


}

CTM_t ctm_pop() {
	CTM_t c = ctm_queue.front();
	ctm_queue.pop();
	return c;
}


CTM_t *ctm_get_current_action() {
	if (ctm_queue.empty()) { return NULL;  }
	return &ctm_queue.front();
}


int ctm_handle_delayed_posthook() {
	if (ctm_posthook_delay_active()) {
		PRINT("at ctm_handle_delayed_posthook()\n");

		CTM_t &c = ctm_queue.front();
		CTM_posthook_t *h = c.get_current_posthook();

		PRINT("CTM id: %ld\n", c.ID);

		if (h->timestamp.get_ms() < h->delay_ms) {
			return 1;
		}
		else {
			h->active = 0;
			h->callback(h->argument);
			PRINT("called posthook callback after %f ms!\n", h->delay_ms);
			
			if (!c.hook_next()) { // this also increments the iterator (crap?)
				ctm_next();
			}
			
			return 1;
		}
	}

	return 0;
}

void ctm_purge_old() {
	while (!ctm_queue.empty()) {
		const CTM_t &c = ctm_queue.front();
		if (c.timestamp.get_s() > 15) {
			ctm_queue.pop();
		}
		else {
			break;
		}
	}
}



// TBC:
//static const uint
//	CTM_X = 0xD68A18,
//	CTM_Y = 0xD68A1C,
//	CTM_Z = 0xD68A20,
//	CTM_ACTION = 0xD689BC,
//	CTM_GUID = 0xD689C0, // this is for interaction
//	CTM_MOVE_ATTACK_ZERO = 0xD689CC,
//
//	CTM_walking_angle = 0xD689A0,
//	CTM_FL_A4 = 0xD689A4,
//	CTM_FL_A8 = 0xD689A8,
//	CTM_min_distance = 0xD689AC,
//
//	CTM_increment = 0xD689B8,
//
//	CTM_mystery_C8 = 0xD689C8,
//	CTM_mystery_A90 = 0xD68A90,
//	CTM_mystery_A94 = 0xD68A94;

// the lowest-level unique function to setting CTM is 0x5FC680
// maybe some interesting static at 0xD3F78C


static const uint
CTM_X = 0xCA1264,
CTM_Y = 0xCA1268,
CTM_Z = 0xCA126C,
CTM_ACTION = 0xCA11F4,
CTM_GUID = 0xCA11FC, // this is for interaction

CTM_walking_angle = 0xCA11D8,
CTM_GLOBAL_CONST1 = 0xCA11DC,
CTM_CONST2 = 0xCA11E0,
CTM_min_distance = 0xCA11E4,
CTM_faceangle_maybe = 0xCA11EC;

int get_wow_CTM_state() {
	int state;
	readAddr(CTM_ACTION, &state, sizeof(state));
	return state;
}

SIZE_T set_wow_CTM_state(int state) {
	return writeAddr(CTM_ACTION, &state, sizeof(state));
}

void ctm_lock() {
	ctm_locked = 1;
}

void ctm_unlock() {
	ctm_locked = 0;
}

static int prevpos_index = 0;
static vec3 prevpos;

void ctm_cancel() {
	ctm_queue_reset();
	stopfollow();
}

int ctm_check_direction() {
	auto c = ctm_get_current_action();
	if (!c) return 1;

	if (c->action != CTM_FACE) {
		ObjectManager OM;
		auto p = OM.get_local_object();
		if (!p) { return 0; }
		vec3 ppos = p->get_pos();
		if (prevpos_index == 0) {
			prevpos = ppos;
			prevpos_index = 1;
		}
		else {
			prevpos_index = 0;
			if ((c->destination - ppos).length() > (c->destination - prevpos).length()) {
				PRINT("Warning: we seem to be moving away from CTM destination!\n");
				return 0;
			}

		}
	}

	return 1;
}


// this seems to work OK?

void ctm_face_angle(float angle) {
	//ctm_add(CTM_t(CTM_FACE, CTM_PRIO_REPLACE));
	// apparently, action 2 stores zeroes into CA11F8, -1FC, -200, -1D4 and -1F4
	// CA11F0 has this weird value assigned to it
	// both set of coordinates (CA1258-60 and CA1264-6C) are set to zero
	
	static const float TWO_PI = M_PI * 2.0;

	float angle_normalized = angle - TWO_PI * floor(angle / TWO_PI);

	//PRINT("angle_normalized: %f\n", angle_normalized);


	// these don't really seem to matter
	float zero = 0.0;

	writeAddr(0xCA11F8, &zero, 4);
	writeAddr(0xCA11FC, &zero, 4);
	writeAddr(0xCA1200, &zero, 4);

	writeAddr(0xCA11D4, &zero, 4);
	writeAddr(0xCA11F4, &zero, 4);

	writeAddr(0xCA1258, &zero, 4);
	writeAddr(0xCA125C, &zero, 4);
	writeAddr(0xCA1260, &zero, 4);

	writeAddr(0xCA1264, &zero, 4);
	writeAddr(0xCA1268, &zero, 4);
	writeAddr(0xCA126C, &zero, 4);

	// this is, for whatever reason, crucial!!
	static const uint
		GLOBAL_CONST1 = 0x415F66F3;

	writeAddr(CTM_GLOBAL_CONST1, &GLOBAL_CONST1, sizeof(float));

	writeAddr(CTM_faceangle_maybe, &angle_normalized, 4);
	unsigned int two = 2;
	writeAddr(CTM_ACTION, &two, 4);
}


void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance, float angle) {

	if (ctm_locked) {
		return;
	}

	if (action == CTM_FACE) {
		ctm_face_angle(angle);
		return;
	}

	ObjectManager OM;
	auto p = OM.get_local_object();
	if (!p) return;
	vec3 pp = p->get_pos();
	vec3 diff = pp - point; // this is kinda weird, since usually one would take dest - current_loc, but np
	//float directed_angle = atan2(diff.y, diff.x);
	float directed_angle = atan2(point.y, point.x) - atan2(pp.y, pp.x) - 0.5*M_PI;
	if (directed_angle < 0) { directed_angle += 2 * M_PI; }

	//PRINT("directed angle: %f, diff: %.3f, %.3f, %.3f\n", directed_angle, diff.x, diff.y, diff.z);

	writeAddr(CTM_walking_angle, &directed_angle, sizeof(directed_angle));

	static const uint
		GLOBAL_CONST1 = 0x415F66F3;

	static const float
		MOVE_CONST2 = 0.25, // 0.25, don't really know what this is
		MOVE_MINDISTANCE = 0.5; // this is 0.5, minimum distance from exact point

	static const float
		LOOT_CONST2 = 13.4444444,
		LOOT_MINDISTANCE = 3.6666666;

	float float_CONST2, min_dist;
	GUID_t interact;

	switch (action) {
	case CTM_MOVE:
		float_CONST2 = MOVE_CONST2;
		min_dist = min_distance == 0 ? MOVE_MINDISTANCE : min_distance;
		interact = 0;
		break;

	case CTM_LOOT:
		float_CONST2 = LOOT_CONST2;
		min_dist = min_distance == 0 ? LOOT_MINDISTANCE : min_distance;
		interact = interact_GUID;
		break;

	default:
		float_CONST2 = MOVE_CONST2;
		min_dist = 0.5;
		interact = 0;
		break;
	}

	writeAddr(CTM_GLOBAL_CONST1, &GLOBAL_CONST1, sizeof(float));
	writeAddr(CTM_CONST2, &float_CONST2, sizeof(float));
	writeAddr(CTM_min_distance, &min_dist, sizeof(float));
	writeAddr(CTM_GUID, &interact, sizeof(GUID_t));

	writeAddr(CTM_X, &point.x, sizeof(point.x));
	writeAddr(CTM_Y, &point.y, sizeof(point.y));
	writeAddr(CTM_Z, &point.z, sizeof(point.z));

	writeAddr(CTM_ACTION, &action, sizeof(action));

}


void click_to_move(const CTM_t& c) {
	click_to_move(c.destination, c.action, c.interact_GUID, c.min_distance, c.angle);
}

void broadcast_hold() {
	DoString("RunMacroText(\"/lole broadcast hold\")");
}
