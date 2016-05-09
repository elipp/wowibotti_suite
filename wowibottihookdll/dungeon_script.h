#pragma once

#include <queue>

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

struct dscript_objective_t {
	int type;
	vec3 wp_pos;
	
	vec3 pack_pos;
	int num_mobs;
	int pack_type;
};

struct dscript_t {
	std::string script_name;

	std::queue<dscript_objective_t> tasks;
	int read_from_file(const std::string &filepath);
};