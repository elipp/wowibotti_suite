#pragma once

#include "defs.h"

#define LUA_prot 0x49DBA0
#define DelIgnore 0x5BA4B0
#define DelIgnore_hookaddr (DelIgnore + 0x11)
#define ClosePetStables 0x4FACA0
#define CTM_main 0x612A90
#define CTM_aux 0x7B8940
#define PLAYER_TARGET_ADDR 0xC6E960
#define PLAYER_TARGET_ADDR2 0xC6E968
#define PLAYER_TARGET_ADDR3 0xC6E970
#define LUA_DoString_addr 0x706C80
#define SelectUnit_addr 0x4A6690
#define GetOSTickCount 0x749850

#define CTM_update 0x612990 // this could be used to signal a completed CTM action
#define CTM_update_hookaddr 0x612A71

#define TicksSinceLastHWEvent 0xBE10FC

// just as an intrigue, seems like the player's spellbook spells are at C6FB00 + 4*n :)

// function at Wow.exe:7D8560 is of interest to clickable AOE spells. the aoe coordinates are at 7D85F3 (ESI+3C), where ESI seems to be mostly E1D828
// E1D734 contains a mask. for aoe its 40.

// 4D8450 seems to be a kind of input handler function :P

// 6FAD80 is where the AOE coordinates are inserted into those addresses (the float array is passed as argument to this func, ARG.1)