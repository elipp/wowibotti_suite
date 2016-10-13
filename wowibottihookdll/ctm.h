#pragma once

#include "defs.h"
#include "wowmem.h"

void ctm_next();
void ctm_act();

int ctm_handle_delayed_posthook();

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
	CTM_posthook_t() {
		callback = NULL;
		delay_frames = 0;
		frame_counter = 0;
		active = 0;
	}
};

enum {
	CTM_PRIO_LOW,
	CTM_PRIO_EXCLUSIVE
};

static LONG CTMID = 0;

struct CTM_t {
	DWORD timestamp;
	LONG ID;
	vec3 destination;
	int action;
	int priority;
	GUID_t interact_GUID;
	float min_distance;

	std::vector<CTM_posthook_t> posthook_chain;
	int hookchain_index;


	CTM_t(const vec3 pos, int act, int prio, GUID_t interact, float min_dist) 
		: destination(pos), action(act), priority(prio), interact_GUID(interact), min_distance(min_dist) {
		
		timestamp = GetTickCount();
		ID = CTMID;

		posthook_chain = std::vector<CTM_posthook_t>();
		hookchain_index = 0;

		++CTMID;
	}
	
	void add_posthook(const CTM_posthook_t &hook) {
		posthook_chain.push_back(hook);
		PRINT("@add_posthook, base addr = %p, ID=%ld posthook_chain.size() = %lp\n", this, this->ID, posthook_chain.size());
	}

	CTM_posthook_t *get_current_posthook() {
		if (!posthook_chain.empty() && hookchain_index < posthook_chain.size()) {
			return &posthook_chain[hookchain_index];
		}
		else return NULL;
	}

	int hook_next() {
		++hookchain_index;
		if (hookchain_index >= posthook_chain.size()) return 0;

		posthook_chain[hookchain_index].active = 1;

		return 1;
	}

	int handle_posthook() {
	
		PRINT("at CTM_t::handle_posthook(), base = %p, ID=%ld\n", this, this->ID);
		if (posthook_chain.empty()) {
			PRINT("posthook_chain.size() == %lu\n", posthook_chain.size());
			ctm_next();
			return 0;
		}

		if (hookchain_index < posthook_chain.size()) {
			CTM_posthook_t &h = posthook_chain[hookchain_index];
			h.active = 1;
			PRINT("DEBUG: run_posthook: setting posthook->active = 1; delay expected = %d frames\n", h.delay_frames);
			return 1;
		}
		else {
			PRINT("@handle_posthook: no more posthooks, calling ctm_next\n");
			ctm_next();
			return 0;
		}
	}
};

void ctm_add(const CTM_t&);
CTM_t ctm_pop();
void click_to_move(const CTM_t&);

CTM_t *ctm_get_current_action();