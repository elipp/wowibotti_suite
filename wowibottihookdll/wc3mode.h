#pragma once

#include "linalg.h"

typedef struct customcamera_t {
	float s;
	float maxdistance;

	glm::vec4 pos;

	glm::vec4 get_cameraoffset();
	float get_angle();
	void increment_s();
	void decrement_s();
	float get_s();

} customcamera_t;

typedef struct marrowgar_status {
	vec3 best_world_pos;
	BYTE best_hotness;
	
	vec3 current_world_pos;
	BYTE current_hotness;
} marrowgar_status;

void do_wc3mode_stuff();
extern customcamera_t customcamera;

void wc3mode_mouseup_hook();
void wc3mode_setselection(const std::string &names_commaseparated);

int wc3mode_enabled();
void enable_wc3mode(int b);
void reset_camera();

void wc3_start_rect();
bool wc3_rect_active();
void wc3_draw_pylpyrs();

int init_custom_d3d();
void draw_custom_d3d();

void cleanup_custom_d3d();

void draw_marrowgar_stuff();

int get_window_width();
int get_window_height();

void wc3mode_assign_cgroup(int index);
void wc3mode_restore_cgroup(int index);

extern int MARROWGAR_ENABLED;

marrowgar_status get_current_marrowgar_status();
