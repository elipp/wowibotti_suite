#pragma once

#include "defs.h"

//#define LUA_prot 0x49DBA0
#define LUA_prot 0x5191C0
#define DelIgnore 0x5BA4B0
#define DelIgnore_hookaddr (DelIgnore + 0x11)
#define ClosePetStables 0x4FACA0
#define CTM_main 0x612A90
#define CTM_aux 0x7B8940
#define PLAYER_TARGET_ADDR 0xC6E960
#define PLAYER_TARGET_ADDR2 0xC6E968
#define PLAYER_TARGET_ADDR3 0xC6E970
//#define LUA_DoString_addr 0x706C80 // <-- tbc
#define LUA_DoString_addr 0x00819210
#define SelectUnit_addr 0x4A6690
#define GetOSTickCount 0x749850
#define SpellErrMsg 0x6F0B70

// these are for wotlk!
//00B499A4   LastHardwareAction
//00B1D618   TimeStamp


#define CTM_update 0x612990 // this could be used to signal a completed CTM action
#define CTM_update_hookaddr 0x612A71

#define TicksSinceLastHWEvent 0xBE10FC

