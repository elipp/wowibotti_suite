use crate::Addr;
const UNKNOWN_ADDRESS: Addr = 0xFFFFFFFF;

// now that we're using `transmute`, this could be `const` -- edit: can't be const, compiler complains that `it's UB to use this value`
#[macro_export]
macro_rules! define_lua_function {
    ($name:ident, ($($param:ident : $param_ty:ty),*) -> $ret_ty:ty) => {
        pub fn $name ($($param : $param_ty),*) -> $ret_ty {
            let ptr = offsets::lua::$name as *const ();
            let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute(ptr) };
            func($($param),*)
        }
    };

    ($name:ident, $addr:expr, ($($param:ident : $param_ty:ty),*) -> $ret_ty:ty) => {
        pub fn $name ($($param : $param_ty),*) -> $ret_ty {
            let ptr = $addr as *const ();
            let func: extern "C" fn($($param_ty),*) -> $ret_ty = unsafe { std::mem::transmute(ptr) };
            func($($param),*)
        }
    };
}
#[cfg(feature = "tbc")]
pub mod offsets {
    use crate::{objectmanager::GUID, Addr, Offset};

    pub const D3D9_DEVICE: Addr = 0xD2A15C;
    pub const D3D9_DEVICE_OFFSET: Offset = 0x3864;

    // 0xD68A00 might contain player GUID

    pub const LAST_HARDWARE_EVENT: Addr = 0xBE10FC;

    #[allow(non_upper_case_globals)]
    pub mod wow_cfuncs {
        use crate::Addr;
        pub const GetUnitOrNPCNameAddr: Addr = 0x614520;
        pub const SelectUnit: Addr = 0x4A6690;
        pub const SetFacing: Addr = 0x7B9DE0;
        pub const SpellErrMsg: Addr = 0x4988A0;
        pub const GetErrorText: Addr = 0x707200;
        pub const GetOsTickCount: Addr = 0x65B3F0; // NOTE: this also writes a new value to the address
    }

    #[allow(non_upper_case_globals)]
    pub mod objectmanager {
        use crate::{Addr, Offset};

        pub const ClientConnection: Addr = 0xD43318;
        pub const CurrentObjectManager: Offset = 0x2218;
        pub const LocalGUIDOffset: Offset = 0xC0; // offset from the object manager to the local guid
        pub const FirstObjectOffset: Offset = 0xAC; // offset from the object manager to the first object
    }

    pub const LUA_PROT: Addr = 0x49DBA0;
    pub const LUA_PROT_PATCHADDR: Addr = 0x49DBA1;
    pub const PLAYER_TARGET_GUID: Addr = 0xC6E960;
    pub const PLAYER_FOCUS_GUID: Addr = 0xC6E980;

    pub mod ctm {
        use crate::Addr;

        pub const PREV_POS_X: Addr = 0xD68A0C;
        pub const PREV_POS_Y: Addr = 0xD68A10;
        pub const PREV_POS_Z: Addr = 0xD68A14;

        pub const TARGET_POS_X: Addr = 0xD68A18;
        pub const TARGET_POS_Y: Addr = 0xD68A1C;
        pub const TARGET_POS_Z: Addr = 0xD68A20;

        pub const CONST1: Addr = 0xD68A24;
        pub const ACTION: Addr = 0xD689BC;
        pub const TIMESTAMP: Addr = 0xD689B8;
        pub const INTERACT_GUID: Addr = 0xD689C0;
        pub const MOVE_ATTACK_ZERO: Addr = 0xD689CC;

        pub const WALKING_ANGLE: Addr = 0xD689A0;
        pub const GLOBAL_CONST1: Addr = 0xD689A4;
        pub const GLOBAL_CONST2: Addr = 0xD689A8;
        pub const MIN_DISTANCE: Addr = 0xD689AC;

        pub const INCREMENT: Addr = 0xD689B8;

        pub const MYSTERY_C8: Addr = 0xD689C8;
        pub const MYSTERY_90: Addr = 0xD68A90;
        pub const MYSTERY_94: Addr = 0xD68A94;
    }

    #[allow(non_upper_case_globals)]
    pub mod lua {
        use crate::Addr;
        pub const lua_gettop: Addr = 0x0072DAE0;
        pub const lua_settop: Addr = 0x0072DB00;
        pub const lua_tonumber: Addr = 0x0072DF40;
        pub const lua_tointeger: Addr = 0x0072DF80;
        pub const lua_touserdata: Addr = 0x0072E120;
        pub const lua_toboolean: Addr = 0x0072DFC0;
        pub const lua_pushnumber: Addr = 0x0072E1A0;
        pub const lua_pushinteger: Addr = 0x0072E1D0;
        pub const lua_isstring: Addr = 0x0072DE70;
        pub const lua_tolstring: Addr = 0x0072DFF0;
        pub const lua_pushlstring: Addr = 0x0072E200;
        pub const lua_pushstring: Addr = 0x0072E250;
        pub const lua_pushboolean: Addr = 0x0072E3B0;
        pub const lua_pushcclosure: Addr = 0x0072E2F0;
        pub const lua_pushnil: Addr = 0x0072E180;
        pub const lua_gettable: Addr = 0x0072E440;
        pub const lua_setfield: Addr = 0x0072E7E0;
        pub const lua_getfield: Addr = 0x0072E470;
        pub const lua_getfield_wow_weird: Addr = 0x0072F710;
        pub const lua_replace: Addr = 0x0072DC80;
        pub const lua_next: Addr = 0x0072EE30;
        pub const lua_gettype: Addr = 0x0072DDC0;
        pub const lua_gettypestring: Addr = 0x0072DDE0;
        pub const lua_dostring: Addr = 0x00706C80;

        pub const wow_lua_State: Addr = 0xE1DB84;

        // the Lua stack in wow prevents all lua_pushcclosure calls
        // where the closure address isn't within Wow.exe's .code section
        // this is the upper limit

        pub const vfp_max: Addr = 0xE1F834;
        pub const vfp_original: u32 = 0xDEADBEEF;
        // pub const FrameScript_Register: Addr = 0x007059B0;
    }

    pub mod socket {
        use crate::{Addr, Offset};
        pub const CONNECTION: Addr = 0xD4332C;
        pub const SOCKOBJ: Offset = 0x2198;
    }

    // NOTE: /script DEFAULT_CHAT_FRAME:AddMessage( GetMouseFocus():GetName() ); is a great way to find out stuff

    #[allow(non_upper_case_globals)]
    pub mod wowobject {
        use crate::Offset;
        pub const Type: Offset = 0x14;
        pub const GUID: Offset = 0x30;
        pub const Next: Offset = 0x3C;
        // For UNITs, both the UnitPosX... and these seem to contain these coord values
        pub const PosX: Offset = 0xBF0;
        pub const PosY: Offset = PosX + 0x4;
        pub const PosZ: Offset = PosX + 0x8;
        pub const Rot: Offset = PosX + 0xC;

        pub const MovementInfo: Offset = 0x128; // todo: deref this to get to the movementinfo, for posx etc

        pub const UnitHealth: Offset = 0x2698;
        pub const UnitMana: Offset = UnitHealth + 0x4;
        pub const UnitRage: Offset = UnitHealth + 0x8;
        pub const UnitFocus: Offset = UnitHealth + 0xC;
        pub const UnitHealthMax: Offset = UnitHealth + 0x18;
        pub const UnitManaMax: Offset = UnitHealthMax + 0x4;
        pub const UnitRageMax: Offset = UnitHealthMax + 0x8;
        pub const UnitFocusMax: Offset = UnitHealthMax + 0xC;

        pub const UnitPosX: Offset = 0xBE8;
        pub const UnitPosY: Offset = UnitPosX + 0x4;
        pub const UnitPosZ: Offset = UnitPosX + 0x8;
        pub const UnitRot: Offset = UnitPosX + 0xC;

        pub const NpcPosX: Offset = 0xC28;
        pub const NpcPosY: Offset = UnitPosX + 0x4;
        pub const NpcPosZ: Offset = UnitPosX + 0x8;
        pub const NpcRot: Offset = UnitPosX + 0xC;

        pub const UnkState: Offset = 0x120;

        pub const UnitTargetGUID: Offset = 0x2680;

        pub const NPCHealth: Offset = 0x11E8;
        pub const NPCMana: Offset = NPCHealth + 0x4;
        pub const NPCRage: Offset = NPCHealth + 0x8;
        pub const NPCEnergy: Offset = NPCHealth + 0xC;
        pub const NPCFocus: Offset = NPCHealth + 0x10;

        pub const NPCHealthMax: Offset = 0x1200;
        pub const NPCManaMax: Offset = NPCHealthMax + 0x4;
        pub const NPCRageMax: Offset = NPCHealthMax + 0x8;
        pub const NPCEnergyMax: Offset = NPCHealthMax + 0xC;
        pub const NPCFocusMax: Offset = NPCHealthMax + 0x10;

        pub const NPCTargetGUID: Offset = 0xF08;
    }

    define_lua_function! {
        SelectUnit,
        wow_cfuncs::SelectUnit,
        (guid: GUID) -> i32
    }

    define_lua_function! {
        GetOsTickCount,
        wow_cfuncs::GetOsTickCount,
        () -> u32
    }
}

#[cfg(feature = "wotlk")]
pub mod offsets {
    use crate::{objectmanager::GUID, Addr, Offset};

    use super::UNKNOWN_ADDRESS;

    pub const D3D9_DEVICE: Addr = 0xC5DF88;
    pub const D3D9_DEVICE_OFFSET: Offset = 0x397C;

    pub const LAST_HARDWARE_EVENT: Addr = 0xB499A4;

    #[allow(non_upper_case_globals)]
    pub mod wow_cfuncs {
        use crate::{addrs::UNKNOWN_ADDRESS, Addr};
        // pub const GetUnitOrNPCNameAddr: Addr = 0x614520;
        pub const GetUnitOrNPCNameAddr: Addr = 0x72A000;
        pub const SelectUnit: Addr = 0x524BF0;
        pub const SetFacing: Addr = 0x989B70;
        pub const SpellErrMsg: Addr = 0x808200;
        pub const GetErrorText: Addr = UNKNOWN_ADDRESS;
        pub const GetOsTickCount: Addr = 0x749850; // NOTE: this also writes a new value to the address
    }

    #[allow(non_upper_case_globals)]
    pub mod objectmanager {
        use crate::{Addr, Offset};

        pub const ClientConnection: Addr = 0xC79CE0;
        pub const CurrentObjectManager: Offset = 0x2ED0;
        pub const LocalGUIDOffset: Offset = 0xC0; // offset from the object manager to the local guid
        pub const FirstObjectOffset: Offset = 0xAC; // offset from the object manager to the first object
    }

    pub const LUA_PROT: Addr = UNKNOWN_ADDRESS;
    pub const LUA_PROT_PATCHADDR: Addr = UNKNOWN_ADDRESS;
    pub const PLAYER_TARGET_GUID: Addr = 0xBD07B0;
    pub const PLAYER_FOCUS_GUID: Addr = 0xBD07D0;

    #[allow(non_upper_case_globals)]
    pub mod lua {
        use crate::{addrs::UNKNOWN_ADDRESS, Addr};
        pub const lua_gettop: Addr = 0x84DBD0;
        pub const lua_settop: Addr = 0x84DBF0;
        pub const lua_tonumber: Addr = 0x84E030;
        pub const lua_tointeger: Addr = 0x84E070;
        pub const lua_touserdata: Addr = 0x84E210;
        pub const lua_toboolean: Addr = 0x84E0B0;
        pub const lua_pushnumber: Addr = 0x84E2A0;
        pub const lua_pushinteger: Addr = 0x84E2D0;
        pub const lua_isstring: Addr = 0x84DF60;
        pub const lua_tolstring: Addr = 0x84E0E0;
        pub const lua_pushlstring: Addr = 0x84E300;
        pub const lua_pushstring: Addr = 0x84E350;
        pub const lua_pushboolean: Addr = 0x84E4D0;
        pub const lua_pushcclosure: Addr = 0x84E400;
        pub const lua_pushnil: Addr = 0x84E280;
        pub const lua_gettable: Addr = 0x84E560;
        pub const lua_setfield: Addr = 0x84E900;
        pub const lua_getfield: Addr = 0x84E590;
        pub const lua_getfield_wow_weird: Addr = 0x84F380;
        pub const lua_replace: Addr = UNKNOWN_ADDRESS;
        pub const lua_next: Addr = 0x854690;
        pub const lua_gettype: Addr = 0x84DEB0;
        pub const lua_gettypestring: Addr = 0x84DED0;
        pub const lua_dostring: Addr = 0x819210;

        pub const wow_lua_State: Addr = 0xD3F78C;

        // the Lua stack in wow prevents all lua_pushcclosure calls
        // where the closure address isn't within Wow.exe's .code section
        // this is the upper limit

        pub const vfp_max: Addr = 0xD415BC;
        pub const vfp_original: u32 = 0x9DE3B3;
        // pub const FrameScript_Register: Addr = 0x007059B0;
    }

    pub mod ctm {
        use crate::{addrs::UNKNOWN_ADDRESS, Addr};

        pub const PREV_POS_X: Addr = UNKNOWN_ADDRESS;
        pub const PREV_POS_Y: Addr = UNKNOWN_ADDRESS;
        pub const PREV_POS_Z: Addr = UNKNOWN_ADDRESS;

        pub const TARGET_POS_X: Addr = 0xCA1264;
        pub const TARGET_POS_Y: Addr = 0xCA1268;
        pub const TARGET_POS_Z: Addr = 0xCA126C;

        pub const CONST1: Addr = UNKNOWN_ADDRESS;
        pub const ACTION: Addr = 0xCA11F4;
        pub const TIMESTAMP: Addr = UNKNOWN_ADDRESS;
        pub const INTERACT_GUID: Addr = 0xCA11FC;
        pub const MOVE_ATTACK_ZERO: Addr = UNKNOWN_ADDRESS;

        pub const WALKING_ANGLE: Addr = 0xCA11D8;
        pub const GLOBAL_CONST1: Addr = 0xCA11DC;
        pub const GLOBAL_CONST2: Addr = 0xCA11E0;
        pub const MIN_DISTANCE: Addr = 0xCA11E4;

        pub const INCREMENT: Addr = UNKNOWN_ADDRESS;

        pub const MYSTERY_C8: Addr = UNKNOWN_ADDRESS;
        pub const MYSTERY_90: Addr = UNKNOWN_ADDRESS;
        pub const MYSTERY_94: Addr = UNKNOWN_ADDRESS;
    }

    pub mod socket {
        use crate::{Addr, Offset};
        pub const CONNECTION: Addr = 0xC79CF4;
        pub const SOCKOBJ: Offset = 0x2E38;
    }

    #[allow(non_upper_case_globals)]
    pub mod wowobject {
        use crate::Offset;
        pub const Type: Offset = 0x14;
        pub const GUID: Offset = 0x30;
        pub const Next: Offset = 0x3C;
        // For UNITs, both the UnitPosX... and these seem to contain these coord values
        pub const PosX: Offset = 0xBF0;
        pub const PosY: Offset = PosX + 0x4;
        pub const PosZ: Offset = PosX + 0x8;
        pub const Rot: Offset = PosX + 0xC;

        pub const MovementInfo: Offset = 0x128; // todo: deref this to get to the movementinfo, for posx etc

        pub const UnitHealth: Offset = 0x2698;
        pub const UnitMana: Offset = UnitHealth + 0x4;
        pub const UnitRage: Offset = UnitHealth + 0x8;
        pub const UnitFocus: Offset = UnitHealth + 0xC;
        pub const UnitHealthMax: Offset = UnitHealth + 0x18;
        pub const UnitManaMax: Offset = UnitHealthMax + 0x4;
        pub const UnitRageMax: Offset = UnitHealthMax + 0x8;
        pub const UnitFocusMax: Offset = UnitHealthMax + 0xC;

        pub const UnitPosX: Offset = 0xBE8;
        pub const UnitPosY: Offset = UnitPosX + 0x4;
        pub const UnitPosZ: Offset = UnitPosX + 0x8;
        pub const UnitRot: Offset = UnitPosX + 0xC;

        pub const NpcPosX: Offset = 0xC28;
        pub const NpcPosY: Offset = UnitPosX + 0x4;
        pub const NpcPosZ: Offset = UnitPosX + 0x8;
        pub const NpcRot: Offset = UnitPosX + 0xC;

        pub const UnkState: Offset = 0x120;

        pub const UnitTargetGUID: Offset = 0x2680;

        pub const NPCHealth: Offset = 0x11E8;
        pub const NPCMana: Offset = NPCHealth + 0x4;
        pub const NPCRage: Offset = NPCHealth + 0x8;
        pub const NPCEnergy: Offset = NPCHealth + 0xC;
        pub const NPCFocus: Offset = NPCHealth + 0x10;

        pub const NPCHealthMax: Offset = 0x1200;
        pub const NPCManaMax: Offset = NPCHealthMax + 0x4;
        pub const NPCRageMax: Offset = NPCHealthMax + 0x8;
        pub const NPCEnergyMax: Offset = NPCHealthMax + 0xC;
        pub const NPCFocusMax: Offset = NPCHealthMax + 0x10;

        pub const NPCTargetGUID: Offset = 0xF08;
    }

    define_lua_function! {
        SelectUnit,
        wow_cfuncs::SelectUnit,
        (guid: GUID) -> i32
    }

    define_lua_function! {
        GetOsTickCount,
        wow_cfuncs::GetOsTickCount,
        () -> u32
    }
}
