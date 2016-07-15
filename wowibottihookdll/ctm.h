#pragma once

#include "defs.h"
#include "wowmem.h"

typedef void (*CTM_posthook_t)();

struct CTM_t {
	DWORD timestamp;
	LONG ID;
	vec3 destination;
	int action;
	int priority;
	GUID_t interact_GUID;
	float min_distance;
	CTM_posthook_t posthook;

	CTM_t(const vec3 pos, int act, int prio, GUID_t interact, float min_dist) 
		: destination(pos), action(act), priority(prio), interact_GUID(interact), min_distance(min_dist) {
		
		timestamp = GetTickCount();
		posthook = NULL;

		// ID needs to be figured out :P

	}
	CTM_t() {}

	void set_posthook(CTM_posthook_t hook) {
		posthook = hook;
	}

	void run_posthook() {
		if (posthook != NULL) posthook();
	}
};

void ctm_add(const CTM_t&);
void ctm_commit();
void ctm_act();
CTM_t ctm_pop();

void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance = 0.0);
void click_to_move(const CTM_t&);

void ctm_lock();
void ctm_unlock();

int get_wow_CTM_state();
