#pragma once

#include "defs.h"

//#define LUA_prot 0x49DBA0

#define TODO 0xFFFFFFFF

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
			UnitReaction = 0x7251C0,
		};

		namespace CTM {
			enum {
				X = 0xCA1264,
				Y = 0xCA1268,
				Z = 0xCA126C,
				ACTION = 0xCA11F4,
				GUID = 0xCA11FC, // this is for interaction

				WALKING_ANGLE = 0xCA11D8,
				GLOBAL_CONST1 = 0xCA11DC,
				CONST2 = 0xCA11E0,
				MIN_DISTANCE = 0xCA11E4,
				FACEANGLE_MAYBE = 0xCA11EC,
			};
		}
	}

	namespace TBC {
		enum Addresses {
			D3D9Device = 0xD2A15C,
			D3D9DeviceOffset = 0x3864,
			ClosePetStables = 0x4FACA0,
			PLAYER_TARGET_ADDR = 0xC6E960,
			PLAYER_FOCUS_ADDR = TODO,
			LUA_DoString_addr = 0x706C80,
			LUA_Prot = 0x49DBA0,
			LUA_Prot_patchaddr = 0x49DBA1,
			SelectUnit_addr = 0x4A6690,
			TicksSinceLastHWEvent = 0xBE10FC,
			GetUnitOrNPCNameAddr = 0x614520,
		};

		namespace ObjectManager {
			enum {
				ClientConnection = 0xD43318,
				CurrentObjectManager = 0x2218,
				LocalGUIDOffset = 0xC0, // offset from the object manager to the local guid
				FirstObjectOffset = 0xAC, // offset from the object manager to the first object
			};
		}

		namespace WowObject {
			enum {
				Type = 0x14,
				GUID = 0x30,
				Next = 0x3C,
				// For UNITs, both the UnitPosX... and these seem to contain these coord values
				X = 0xBF0,
				Y = X + 4,
				Z = X + 8,
				R = X + 12,

				unit_info_field = 0x120,
				UnitHealth = 0x2698,
				UnitMana = UnitHealth + 0x4,
				UnitRage = UnitHealth + 0x8,
				UnitFocus = UnitHealth + 0xC,
				UnitHealthMax = UnitHealth + 0x18,
				UnitManaMax = UnitHealthMax + 0x4,
				UnitRageMax = UnitHealthMax + 0x8,
				UnitFocusMax = UnitHealthMax + 0xC,

				UnitPosX = 0xBE8,
				UnitPosY = UnitPosX + 0x4,
				UnitPosZ = UnitPosX + 0x8,
				UnitRot = UnitPosX + 0xC,

				UnitTargetGUID = 0x2680,

				NPCHealth = 0x11E8,
				NPCMana = NPCHealth + 0x4,
				NPCRage = NPCHealth + 0x8,
				NPCEnergy = NPCHealth + 0xC,
				NPCFocus = NPCHealth + 0x10,

				NPCHealthMax = 0x1200,
				NPCManaMax = NPCHealthMax + 0x4,
				NPCRageMax = NPCHealthMax + 0x8,
				NPCEnergyMax = NPCHealthMax + 0xC,
				NPCFocusMax = NPCHealthMax + 0x10,

				NPCTargetGUID = 0xF08,
			};
		}

		namespace CTM {
			enum {
				X = 0xD68A18,
				Y = 0xD68A1C,
				Z = 0xD68A20,
				ACTION = 0xD689BC,
				GUID = 0xD689C0, // this is for interaction
				MOVE_ATTACK_ZERO = 0xD689CC,

				WALKING_ANGLE = 0xD689A0,
				FL_A4 = 0xD689A4,
				FL_A8 = 0xD689A8,
				MIN_DISTANCE = 0xD689AC,

				INCREMENT = 0xD689B8,

				MYSTERY_C8 = 0xD689C8,
				MYSTERY_90 = 0xD68A90,
				MYSTERY_94 = 0xD68A94
			};
		}
	}
}
