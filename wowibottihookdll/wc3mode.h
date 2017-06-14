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


void do_wc3mode_stuff();
extern customcamera_t customcamera;

void wc3mode_mouseup_hook();
void wc3mode_checkselection(const RECT &r);

int wc3mode_enabled();
void enable_wc3mode(int b);
void reset_camera();

void wc3_start_rect();
void wc3_draw_pylpyrs();

int init_custom_d3d();
void draw_custom_d3d();
