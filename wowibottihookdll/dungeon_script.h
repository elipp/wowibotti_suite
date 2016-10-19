#pragma once

#include <queue>
#include <unordered_map>

#include "defs.h"


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

struct dscript_objective_t {
	int type;
	vec3 wp_pos;
	
	vec3 pack_pos;
	int num_mobs;
	int pack_type;

	std::vector<GUID> mob_GUIDS;
};

struct dscript_t {
	std::string script_name;
	int script_state;

	std::queue<dscript_objective_t> tasks;
	int read_from_file(const std::string &filepath);

	dscript_t() {
		script_state = 0;
	}
};

int dscript_read_all(); // read all from the dscript/ directory
int dscript_run(); // run currently active script
int dscript_load(const std::string &scriptname);
int dscript_unload();
