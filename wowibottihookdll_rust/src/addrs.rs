use crate::{objectmanager::GUID, Addr, Offset};

pub const D3D9_DEVICE: Addr = 0xD2A15C;
pub const D3D9_DEVICE_OFFSET: Offset = 0x3864;

#[allow(non_upper_case_globals)]
pub mod wow_cfuncs {
    use crate::Addr;
    pub const SpellErrMsg: Addr = 0x4988A0;
    pub const GetUnitOrNPCNameAddr: Addr = 0x614520;
    pub const SelectUnit: Addr = 0x4A6690;
    pub const SetFacing: Addr = 0x7B9DE0;
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

// now that we're using `transmute`, this could be `const` -- edit: can't be const, compiler complains that `it's UB to use this value`
#[macro_export]
macro_rules! define_wow_cfunc {
    ($name:ident, ($($param:ident : $param_ty:ty),*) -> $ret_ty:ty) => {
        pub fn $name ($($param : $param_ty),*) -> $ret_ty {
            let ptr = addrs::$name as *const ();
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
    }
}

define_wow_cfunc! {
    SelectUnit,
    wow_cfuncs::SelectUnit,
    (guid: GUID) -> i32
}
