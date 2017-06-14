#pragma once

#include "hooks.h"

void add_input_event(inpevent_t *t);
void get_cursor_pos(POINT *p);

int __stdcall AddInputEvent_hook(struct inpevent_t *t);

void hook_input_func();
void unhook_input_func();

extern int need_mouseup;
extern int mouse_pressed;

void add_mouseup();