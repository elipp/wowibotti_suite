#pragma once

#include "defs.h"
#include "wowmem.h"

void ctm_next();
void ctm_act();

void ctm_handle_delayed_posthook();

void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance = 0.0);

void ctm_lock();
void ctm_unlock();

int get_wow_CTM_state();

typedef void (*CTM_callback_t)();

struct CTM_posthook_t {
	CTM_callback_t callback;
	int delay_frames;

	int frame_counter, active;

	CTM_posthook_t(CTM_callback_t hookfunc, int frame_delay) : callback(hookfunc), delay_frames(frame_delay), frame_counter(0), active(0) {};
};


struct CTM_t {
	DWORD timestamp;
	LONG ID;
	vec3 destination;
	int action;
	int priority;
	GUID_t interact_GUID;
	float min_distance;

	CTM_posthook_t *posthook;

	CTM_t(const vec3 pos, int act, int prio, GUID_t interact, float min_dist) 
		: destination(pos), action(act), priority(prio), interact_GUID(interact), min_distance(min_dist) {
		
		timestamp = GetTickCount();
		posthook = NULL;

		// ID needs to be figured out :P

	}
	
	CTM_t() {}

	void set_posthook(const CTM_posthook_t &hook) {
		posthook = new CTM_posthook_t(hook);
	}

	int handle_posthook() const {
		if (posthook) {

			if (posthook->delay_frames > 0) {
				posthook->active = 1;
				printf("DEBUG: run_posthook: setting posthook->active = 1; delay expected = %d frames\n", posthook->delay_frames);
				return 1;
			}
			else {
				printf("DEBUG: run_posthook: calling posthook callback (without delay)!\n");
				posthook->callback();
			}
			
			return 1;
		}
		else {
			ctm_next();
			return 0;
		}
	}
};

void ctm_add(const CTM_t&);
CTM_t ctm_pop();
void click_to_move(const CTM_t&);

const CTM_t &ctm_get_current_action();