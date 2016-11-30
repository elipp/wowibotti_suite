#pragma once

#include "defs.h"
#include "wowmem.h"
#include "timer.h"

void ctm_next();
void ctm_act();

int ctm_handle_delayed_posthook();

void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance = 0.0);

void ctm_lock();
void ctm_unlock();

void ctm_purge_old();

int ctm_job_in_progress();
void ctm_abort_if_not_moving();

int get_wow_CTM_state();
SIZE_T set_wow_CTM_state(int state);

typedef void (*CTM_callback_t)(void*);

void ctm_update_prevpos();
int char_is_moving();

struct CTM_posthook_t {
	CTM_callback_t callback;
	float delay_ms;

	void *argument;
	size_t argument_size;

	Timer timestamp;
	int active;

	CTM_posthook_t(CTM_callback_t hookfunc, void *hookfunc_arg, size_t arg_size, float delay_milliseconds) : callback(hookfunc), delay_ms(delay_milliseconds), active(0) {
		timestamp.start();
		
		if (hookfunc_arg) {
			argument = malloc(arg_size);
			memcpy(hookfunc_arg, argument, arg_size);
			argument_size = arg_size;
		}
		else {
			hookfunc_arg = NULL;
			argument_size = 0;
		}
	};

	CTM_posthook_t() : callback(NULL), delay_ms(0), active(0) {}

	~CTM_posthook_t() {
		if (argument) free(argument);
		argument = NULL;
	}
};

enum {
	CTM_PRIO_NONE,
	CTM_PRIO_LOW,
	CTM_PRIO_REPLACE,
	CTM_PRIO_EXCLUSIVE,
	CTM_PRIO_FOLLOW,
	CTM_PRIO_HOLD_POSITION,
	CTM_PRIO_CLEAR_HOLD
};

static LONG CTMID = 0;

struct CTM_t {
	Timer timestamp;
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
		
		timestamp.start();
		ID = CTMID;

		posthook_chain = std::vector<CTM_posthook_t>();
		hookchain_index = 0;

		++CTMID;
	}
	
	void add_posthook(const CTM_posthook_t &hook) {
		posthook_chain.push_back(hook);
		PRINT("@add_posthook, base addr = %p, ID=%ld posthook_chain.size() = %lu\n", this, this->ID, posthook_chain.size());
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
			PRINT("DEBUG: run_posthook: setting posthook->active = 1; delay expected = %f ms\n", h.delay_ms);
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
void ctm_queue_reset();
CTM_t *ctm_get_current_action();
