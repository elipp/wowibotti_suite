#pragma once

#include "defs.h"

//#define LUA_prot 0x49DBA0

namespace Addresses {
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
			PLAYER_FOCUS_ADDR = 0x0,
			LUA_DoString_addr = 0x706C80,
			LUA_Prot = 0x49DBA0,
			LUA_Prot_patchaddr = 0x49DBA1,
			SelectUnit_addr = 0x4A6690,
			TicksSinceLastHWEvent = 0xBE10FC,
		};

		namespace ObjectManager {
			enum {
				ClientConnection = 0xD43318,
				CurrentObjectManager = 0x2218,
				LocalGUIDOffset = 0xC0, // offset from the object manager to the local guid
				FirstObjectOffset = 0xAC, // offset from the object manager to the first object
				NextObjectOffset = 0x3C, // offset from one object to the next
			};
		}

		namespace WowObject {
			enum {
				GUID = 0x30,
				Next = 0x3C,
				Type = 0x14,
				X = 0xBF0,
				Y = X + 4,
				Z = X + 8,
				R = X + 12,
				unit_info_field = 0x120,
				Name = 0xDB8,

				UnitHealth = 0x40,
				UnitHealthMax = 0x58,
				UnitPosX = 0x798,
				UnitPosY = UnitPosX + 0x4,
				UnitPosZ = UnitPosX + 0x8,
				UnitRot = 0x7A8,
				UnitTargetGUID = 0x2680,

				NPCHealth = 0x11E8,
				NPCMana = 0x11EC,
				NPCRage = 0x11F0,
				NPCEnergy = 0x11F4,
				NPCFocus = 0x11F8,

				NPCHealthMax = 0x1200,
				NPCManaMax = 0x1204,
				NPCRageMax = 0x1208,
				NPCEnergyMax = 0x120C,
				NPCFocusMax = 0x1210,

			};
		}

		namespace CTM {
			enum {
				X = 0xD68A18,
				Y = 0xD68A1C,
				Z = 0xD68A20,
			};
		}
	}
}



