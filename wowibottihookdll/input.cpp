#include "input.h"
#include "defs.h"
#include "wc3mode.h"
#include "hooks.h"
#include "ctm.h"

// 480130 handler for all these opcodes B)
// B41834 contains a mask of which mouse buttons are being held down
#define MOUSELEFT 0x1
#define MOUSERIGHT 0x4

#define INPUT_KEYDOWN 0x7
#define INPUT_KEYUP 0x8
#define INPUT_MOUSEDOWN 0x9
#define INPUT_MOUSEMOVE 0xA
#define INPUT_MOUSEWHEEL 0xB
#define INPUT_MOUSEDRAG 0xC
#define INPUT_MOUSEUP 0xD

#define WOWINPUT_KEY_CTRL 0x2
#define WOWINPUT_KEY_ALT 0x4
#define WOWINPUT_KEY_R 0x52
#define WOWINPUT_KEY_H 0x48

void get_cursor_pos(POINT *p) {
	GetCursorPos(p);
	ScreenToClient(wow_hWnd, p);
}

int need_mouseup = 0;
int mouse_pressed = 0;

static int control_key_down = 0;

void add_input_event(inpevent_t *t) {
	static const auto ADD_INPUT_EVENT = (void(*)(DWORD, DWORD, DWORD, DWORD, DWORD))(AddInputEvent);
	ADD_INPUT_EVENT(t->event, t->param, t->x, t->y, t->unk1);
}


enum { INPUTEVENT_FILTER = 0, INPUTEVENT_LET_THROUGH };


static int handle_inputmousedown(struct inpevent_t *t) {

	if (!wc3mode_enabled()) {
		if (t->event == INPUT_MOUSEDOWN && t->param == MOUSELEFT) {
			mouse_pressed = 1;
		}

		return INPUTEVENT_LET_THROUGH;
	}

	if (t->param == MOUSELEFT) {
		if (t->y > (float)get_window_height() * 0.78) {
			PRINT("mouse left down within area!\n");
			return INPUTEVENT_LET_THROUGH;
		}
		wc3_start_rect(); // with this return 0
		return INPUTEVENT_FILTER; //return 1;
	}
	else if (t->param == MOUSERIGHT) {
		//t->action = 0xD; // HEHE clever try, but apparently 0xD requires a valid 0x9 to work.
		return INPUTEVENT_LET_THROUGH;
	}
	return INPUTEVENT_FILTER;

}


static int handle_inputmouseup(struct inpevent_t *t) {

	if (!wc3mode_enabled()) return INPUTEVENT_LET_THROUGH;

	if (t->param == MOUSELEFT) {
		if (need_mouseup) {
			need_mouseup = 0;
			mouse_pressed = 0;
			return INPUTEVENT_LET_THROUGH;
		}
		else if (t->y > (float)get_window_height() * 0.78) {
			PRINT("mouse left up within area!\n");
			return INPUTEVENT_LET_THROUGH;
		}
		else {
			wc3mode_mouseup_hook(); // with this return 0
			return INPUTEVENT_FILTER;
		}
	}

	if (t->param == MOUSERIGHT) {
		return INPUTEVENT_LET_THROUGH;
	}

	return INPUTEVENT_LET_THROUGH;
}

static int handle_keydown(struct inpevent_t *t) {
	switch (t->param) {

	case WOWINPUT_KEY_CTRL:
		control_key_down = 1;
		return INPUTEVENT_LET_THROUGH;

	case WOWINPUT_KEY_R:
		if (wc3mode_enabled()) {
			reset_camera();
			return INPUTEVENT_FILTER;
		}
		break;

	case WOWINPUT_KEY_H:
		if (wc3mode_enabled()) {
			broadcast_hold();
			return INPUTEVENT_FILTER;
		}
		break;

	}

	return INPUTEVENT_LET_THROUGH;
}

static int handle_keyup(struct inpevent_t *t) {
	if (t->param == WOWINPUT_KEY_CTRL) control_key_down = 0;
	return INPUTEVENT_LET_THROUGH;
}

static int handle_mousedrag(struct inpevent_t *t) {

	if (!wc3mode_enabled()) return INPUTEVENT_LET_THROUGH;
	else return INPUTEVENT_FILTER;

}


static int handle_mwheel(struct inpevent_t *t) {
	
	if (!wc3mode_enabled()) return INPUTEVENT_LET_THROUGH;

	int delta = t->param;

	if (delta < 0) customcamera.increment_s();
	if (delta > 0) customcamera.decrement_s();

	return INPUTEVENT_FILTER;
}

int __stdcall AddInputEvent_hook(struct inpevent_t *t) {

	DWORD ai = DEREF(0xD41400);
	DWORD ai2 = DEREF(0xD41404);

	static int ALT_pressed = 0;

	int r = INPUTEVENT_LET_THROUGH;

	switch (t->event) {
	case INPUT_KEYDOWN:
		r = handle_keydown(t);
		break;

	case INPUT_KEYUP:
		r = handle_keyup(t);
		break;

	case INPUT_MOUSEDOWN:
		r = handle_inputmousedown(t);
		break;

	case INPUT_MOUSEUP:
		r = handle_inputmouseup(t);
		break;

	case INPUT_MOUSEWHEEL:
		r = handle_mwheel(t);
		break;

	case INPUT_MOUSEDRAG:
		r = handle_mousedrag(t);
		break;

	default:
		r = INPUTEVENT_LET_THROUGH;
		break;
	}

	//if (t->event != INPUT_MOUSEMOVE) { // because spam
	//	PRINT("[%d] [%s] (%d/%d) vars: %X, %X, %d, %d, %X\n",
	//		GetTickCount(), r == 1 ? "VALID" : "FILTERED", ai, ai2, t->event, t->param, t->x, t->y, t->unk1);
	//}

	return r;
}


void hook_input_func() {
	hookable_t *h = find_hookable("AddInputEvent");
	h->patch.enable();
}

void unhook_input_func() {
	hookable_t *h = find_hookable("AddInputEvent");
	h->patch.disable();
}

void add_mouseup() {
	PRINT("add_mouseup called!\n");
	POINT p;
	get_cursor_pos(&p);

	inpevent_t mup;

	mup.event = 0xD;
	mup.param = MOUSELEFT;
	mup.x = p.x;
	mup.y = p.y;
	mup.unk1 = 0;

	add_input_event(&mup);
}

bool iscontrolkeydown() {
	return control_key_down;
}