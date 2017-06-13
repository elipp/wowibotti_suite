#pragma once

#include "linalg.h"

typedef struct {
	float s;
	float maxdistance;

	glm::vec4 pos;

	glm::vec4 get_cameraoffset();
	float get_angle();
	void increment_s();
	void decrement_s();

} customcamera_t;


void do_wc3mode_stuff();
extern customcamera_t customcamera;

void wc3mode_mouseup_hook();

int wc3mode_enabled();
void enable_wc3mode(int b);
void reset_camera();

void wc3_start_rect();
void wc3_draw_pylpyrs();
