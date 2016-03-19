#pragma once

#include "defs.h"
#include "wowmem.h"

void click_to_move(vec3 point, uint action, GUID_t interact_GUID, float min_distance = 0.0);

void ctm_lock();
void ctm_unlock();

int get_wow_CTM_state();
