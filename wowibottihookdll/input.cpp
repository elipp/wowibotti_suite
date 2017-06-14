#include "input.h"
#include "defs.h"
#include "wc3mode.h"
#include "hooks.h"

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

void get_cursor_pos(POINT *p) {
	GetCursorPos(p);
	ScreenToClient(wow_hWnd, p);
}

int need_mouseup = 0;
int mouse_pressed = 0;

void add_input_event(inpevent_t *t) {
	static const auto ADD_INPUT_EVENT = (void(*)(DWORD, DWORD, DWORD, DWORD, DWORD))(AddInputEvent);
	ADD_INPUT_EVENT(t->event, t->param, t->x, t->y, t->unk1);
}



static int handle_inputmousedown(struct inpevent_t *t) {

	if (t->param == MOUSELEFT) {
		wc3_start_rect(); // with this return 0
		return 0; //return 1;
	}
	else if (t->param == MOUSERIGHT) {
		//t->action = 0xD; // HEHE clever try, but apparently 0xD requires a valid 0x9 to work.
		return 1;
	}
	return 0;

}


static int handle_inputmouseup(struct inpevent_t *t) {

	if (t->param == MOUSELEFT) {
		if (need_mouseup) {
			need_mouseup = 0;
			mouse_pressed = 0;
			return 1;
		}
		else {
			wc3mode_mouseup_hook(); // with this return 0
			return 0;
		}
	}

	if (t->param == MOUSERIGHT) {
		return 1;
	}

	return 1;
}

static void handle_mwheel(int delta) {
	if (delta < 0) customcamera.increment_s();
	if (delta > 0) customcamera.decrement_s();
}

int __stdcall AddInputEvent_hook(struct inpevent_t *t) {

	DWORD ai = DEREF(0xD41400);
	DWORD ai2 = DEREF(0xD41404);

	static int ALT_pressed = 0;


	if (t->event == INPUT_KEYDOWN && t->param == 0x4 && ALT_pressed == 0) { // ALT
		enable_wc3mode(1);
		ALT_pressed = 1;
		return 0;
	}

	if (t->event == INPUT_KEYUP && t->param == 0x4) { // ALT
		enable_wc3mode(0);
		ALT_pressed = 0;
		return 0;
	}

	if (ALT_pressed && t->event == INPUT_KEYDOWN && t->param == 0x52) {
		// ALT+R reset camera
		reset_camera();
		return 0;
	}

	if (!wc3mode_enabled()) {
		if (t->event == INPUT_MOUSEDOWN && t->param == MOUSELEFT) {
			mouse_pressed = 1;
		}

		return 1;
	}

	int r = 1;

	switch (t->event) {
	case INPUT_MOUSEDOWN:
		r = handle_inputmousedown(t);
		break;

	case INPUT_MOUSEUP:
		r = handle_inputmouseup(t);
		break;

	case INPUT_MOUSEWHEEL:
		handle_mwheel(t->param);
		r = 0;
		break;

	case INPUT_MOUSEDRAG:
		r = 0;
		break;

	default:
		r = 1;
	}

	//if (t->event != INPUT_MOUSEMOVE) {
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
