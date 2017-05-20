#pragma once

#include "defs.h"

//#define LUA_prot 0x49DBA0
#define LUA_prot 0x5191C0
#define DelIgnore 0x5BA4B0
#define DelIgnore_hookaddr (DelIgnore + 0x11)
#define SendPacket_hookaddr 0x46776E
#define RecvPacket_hookaddr 0x467EC7


//#define ClosePetStables 0x4FACA0 // TBC
#define ClosePetStables 0x005A1950 
#define CTM_main 0x612A90
#define CTM_aux 0x7B8940

//#define PLAYER_TARGET_ADDR 0xC6E960 // <-- tbc

#define PLAYER_TARGET_ADDR 0xBD07B0

//#define LUA_DoString_addr 0x706C80 // <-- tbc
#define LUA_DoString_addr 0x00819210
//#define SelectUnit_addr 0x4A6690 // <-- tbc
#define SelectUnit_addr 0x524BF0
#define GetOSTickCount 0x749850
#define SpellErrMsg 0x6F0B70

// these are for wotlk!
#define LastHardwareAction 0x00B499A4   
#define CurrentTicks 0x00B1D618   


//#define CTM_update 0x612990 // this could be used to signal a completed CTM action
//#define CTM_update_hookaddr 0x612A71

#define CTM_update 0x7272C0
#define CTM_update_hookaddr 0x7273E5


//#define TicksSinceLastHWEvent 0xBE10FC // <-- tbc

