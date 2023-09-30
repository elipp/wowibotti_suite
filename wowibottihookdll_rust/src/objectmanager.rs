use std::ffi::{c_char, CStr};

use crate::patch::deref;
use crate::vec3::Vec3;
use crate::{Addr, LoleError, LoleResult};

use crate::addresses::{self as addrs, PLAYER_TARGET_GUID};
use crate::cstr_to_str;

use std::arch::asm;

pub type GUID = u64;

pub const NO_TARGET: GUID = 0x0;

#[derive(Clone, Copy)]
pub struct WowObject {
    base: Addr,
}

mod wowobject {
    use crate::Offset;
    pub const Type: Offset = 0x14;
    pub const GUID: Offset = 0x30;
    pub const Next: Offset = 0x3C;
    // For UNITs, both the UnitPosX... and these seem to contain these coord values
    pub const PosX: Offset = 0xBF0;
    pub const PosY: Offset = PosX + 0x4;
    pub const PosZ: Offset = PosX + 0x8;
    pub const Rot: Offset = PosX + 0xC;

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

#[repr(u8)]
#[derive(Debug)]
pub enum WowObjectType {
    Object,
    Item,
    Container,
    Npc,
    Unit,
    GameObject,
    DynamicObject,
    Corpse,
    AreaTrigger,
    SceneObject,
    Unknown(i32),
}

impl From<i32> for WowObjectType {
    fn from(value: i32) -> Self {
        match value {
            0 => WowObjectType::Object,
            1 => WowObjectType::Item,
            2 => WowObjectType::Container,
            3 => WowObjectType::Npc,
            4 => WowObjectType::Unit,
            5 => WowObjectType::GameObject,
            6 => WowObjectType::DynamicObject,
            7 => WowObjectType::Corpse,
            8 => WowObjectType::AreaTrigger,
            9 => WowObjectType::SceneObject,
            v => WowObjectType::Unknown(v),
        }
    }
}

impl WowObject {
    fn valid(&self) -> bool {
        (self.base != 0) && ((self.base & 0x3) == 0)
    }
    pub fn get_type(&self) -> WowObjectType {
        let t = deref::<i32, 1>(self.base + wowobject::Type);
        t.into()
    }
    pub fn get_name(&self) -> &str {
        match self.get_type() {
            WowObjectType::Npc | WowObjectType::Unit => {
                let mut result: *const c_char = std::ptr::null();
                let base = self.base;
                let func_addr = addrs::GetUnitOrNPCNameAddr;
                unsafe {
                    asm!(
                        "push 0",
                        "call {func_addr:e}",
                        "mov {result}, eax",
                        in("ecx") base,
                        func_addr = in(reg) func_addr,
                        result = out(reg) result,
                    )
                }
                cstr_to_str!(result).unwrap_or("!StringError!")
            }
            _ => "Unknown",
        }
    }
    fn get_GUID(&self) -> GUID {
        deref::<GUID, 1>(self.base + wowobject::GUID)
    }
    fn get_next(&self) -> Option<WowObject> {
        let next_base_addr = deref::<Addr, 1>(self.base + wowobject::Next);
        WowObject::try_new(next_base_addr)
    }
    pub fn try_new(base: Addr) -> Option<Self> {
        let res = WowObject { base };
        if res.valid() {
            Some(res)
        } else {
            None
        }
    }
    pub fn get_xyzr(&self) -> [f32; 4] {
        let m = unsafe { std::slice::from_raw_parts((self.base + wowobject::PosX) as *const _, 4) };
        [m[0], m[1], m[2], m[3]]
    }

    pub fn get_pos(&self) -> Vec3 {
        let m = unsafe { std::slice::from_raw_parts((self.base + wowobject::PosX) as *const _, 3) };
        Vec3 {
            x: m[0],
            y: m[1],
            z: m[2],
        }
    }
}

impl std::fmt::Display for WowObject {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "[0x{:X}] WowObjectType::[{:?}] - GUID: {:016X} - Name: {}",
            self.base,
            self.get_type(),
            self.get_GUID(),
            self.get_name(),
        )
    }
}

pub struct WowObjectIterator(Option<WowObject>);

impl Iterator for WowObjectIterator {
    type Item = WowObject;

    fn next(&mut self) -> Option<Self::Item> {
        if let Some(w) = self.0 {
            self.0 = w.get_next();
            self.0
        } else {
            None
        }
    }
}

mod objectmanager {
    use crate::{Addr, Offset};

    pub const ClientConnection: Addr = 0xD43318;
    pub const CurrentObjectManager: Offset = 0x2218;
    pub const LocalGUIDOffset: Offset = 0xC0; // offset from the object manager to the local guid
    pub const FirstObjectOffset: Offset = 0xAC; // offset from the object manager to the first object
}

impl IntoIterator for ObjectManager {
    type Item = WowObject;

    type IntoIter = WowObjectIterator;

    fn into_iter(self) -> Self::IntoIter {
        WowObjectIterator(self.get_first_object())
    }
}

#[derive(Copy, Clone)]
pub struct ObjectManager {
    base: Addr,
}

impl ObjectManager {
    pub fn new() -> LoleResult<Self> {
        let client_connection = deref::<Addr, 1>(objectmanager::ClientConnection);
        if client_connection == 0 {
            return Err(LoleError::ClientConnectionIsNull);
        }
        let current_object_manager_base =
            deref::<Addr, 1>(client_connection + objectmanager::CurrentObjectManager);

        if current_object_manager_base == 0 {
            return Err(LoleError::ObjectManagerIsNull);
        } else {
            Ok(Self {
                base: current_object_manager_base,
            })
        }
    }
    pub fn iter(&self) -> WowObjectIterator {
        self.into_iter()
    }
    pub fn get_local_GUID(&self) -> GUID {
        deref::<GUID, 1>(self.base + objectmanager::LocalGUIDOffset)
    }
    pub fn get_player_target_GUID(&self) -> GUID {
        deref::<GUID, 1>(PLAYER_TARGET_GUID)
    }
    pub fn get_player(&self) -> LoleResult<WowObject> {
        let local_GUID = self.get_local_GUID();
        self.iter()
            .find(|w| w.get_GUID() == local_GUID)
            .ok_or_else(|| LoleError::PlayerNotFound)
    }
    pub fn get_player_and_target(&self) -> LoleResult<(WowObject, Option<WowObject>)> {
        let player = self.get_player()?;
        let target_guid = self.get_player_target_GUID();
        Ok((player, self.get_object_by_GUID(target_guid)))
    }
    pub fn get_object_by_GUID(&self, guid: GUID) -> Option<WowObject> {
        self.iter().find(|w| w.get_GUID() == guid)
    }
    pub fn get_unit_by_name(&self, name: &str) -> Option<WowObject> {
        self.iter()
            .filter(|w| matches!(w.get_type(), WowObjectType::Npc | WowObjectType::Unit))
            .find(|w| w.get_name() == name)
    }
    fn get_first_object(&self) -> Option<WowObject> {
        // no need to check for base != 0, because ::new is the only way to construct this
        WowObject::try_new(deref::<Addr, 1>(
            self.base + objectmanager::FirstObjectOffset,
        ))
    }
}
