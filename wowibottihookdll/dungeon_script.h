#pragma once

#include <queue>
#include <unordered_map>

#include "defs.h"
#include "wowmem.h"


enum {
	SCRIPT_CTM_NONE,
	SCRIPT_CTM_WP,
	SCRIPT_CTM_PULL
};

enum {
	SCRIPT_PACKTYPE_NONE,
	SCRIPT_PACKTYPE_STATIC,
	SCRIPT_PACKTYPE_PATROL
};

enum {
	SCRIPT_STATE_IDLE,
	SCRIPT_STATE_DRINKING,
	SCRIPT_STATE_MOVING,
	SCRIPT_STATE_FIGHTING
};

struct mob_t {
	int current_health;
	int max_health;
	int dead;
	int in_combat;
	std::string name;
	GUID_t guid;
	mob_t(const WowObject &o) {
		current_health = o.get_health();
		max_health = o.get_health_max();
		dead = o.NPC_unit_is_dead();
		in_combat = o.in_combat();
		name = o.get_name();
		guid = o.get_GUID();
	}
};

struct dscript_objective_t {
	int type;
	vec3 wp_pos;
	
	vec3 pack_pos;
	int num_mobs;
	int pack_type;
	float radius;
	int in_progress;

	const mob_t *mob_to_kill;
	std::vector<mob_t> mobs;
	int get_mob_info();

	dscript_objective_t() : in_progress(0) {}; // the other stuff comes from the read_from_file func
};

struct dscript_t {
	std::string script_name;
	int script_state;

	std::vector<dscript_objective_t> tasks;
	int read_from_file(const std::string &filepath);

	dscript_t() {
		script_state = 0;
	}
};

int dscript_read_all(); // read all from the dscript/ directory

int dscript_load(const std::string &scriptname); // load script
int dscript_run(); // run currently active script
int dscript_next(); // advance to the next objective
int dscript_unload();

int dscript_state(const std::string &arg);

