#pragma once

#include "defs.h"

//#define LUA_prot 0x49DBA0

namespace Wotlk {
	enum Addresses {
		LUA_prot = 0x5191C0,
		DelIgnore = 0x5BA4B0,
		DelIgnore_hookaddr = (Wotlk::DelIgnore + 0x11),
		SendPacket_hookaddr = 0x46776E,
		RecvPacket_hookaddr = 0x467EC7,
		SARC4_encrypt = 0x774EA0,
		mbuttondown_handler = 0x86A7F5,
		mbuttonup_handler = 0x86A85D,
		mbuttonup_hookaddr = 0x86A814,
		mwheel_hookaddr = 0x86A8B3,

		AddInputEvent = 0x868D40,
		AddInputEvent_post = 0x868DA9,

		ClosePetStables = 0x005A1950,
		ClosePetStables_patchaddr = (Wotlk::ClosePetStables + 0x5),
		CTM_main = 0x612A90,
		CTM_aux = 0x7B8940,
		PLAYER_TARGET_ADDR = 0xBD07B0,
		PLAYER_FOCUS_ADDR = 0xBD07D0,
		pylpyr_patchaddr = 0x4F6F90,
		LUA_DoString_addr = 0x00819210,
		SelectUnit_addr = 0x524BF0,
		GetOSTickCount = 0x749850,

		SpellErrMsg = 0x808200,

		LastHardwareAction = 0x00B499A4,
		CurrentTicks = 0x00B1D618,

		//CTM_update = 0x612990 // this could be used to signal a completed CTM action,
		//CTM_update_hookaddr = 0x612A71,

		CTM_finished = 0x7272C0,
		CTM_finished_patchaddr = 0x7273E5,

		CTM_main_hookaddr = 0x727400,

		Camera_static = 0xB7436C,

		SetFacing = 0x00989B70,
	};
}

namespace TBC {
	enum Addresses {
		D3D9Device = 0xD2A15C,
		D3D9DeviceOffset = 0x3864,
		ClosePetStables = 0x4FACA0,
		PLAYER_TARGET_ADDR = 0xC6E960,
		LUA_DoString_addr = 0x706C80,
		LUA_Prot_patchaddr = 0x49DBA0,
		SelectUnit_addr = 0x4A6690,
	};
}



//#define TicksSinceLastHWEvent 0xBE10FC // <-- tbc

