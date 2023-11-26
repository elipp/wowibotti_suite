use std::ffi::{c_char, c_void, CStr};

use crate::patch::{deref, read_elems_from_addr};
use crate::vec3::Vec3;
use crate::{add_repr_and_tryfrom, Addr, LoleError, LoleResult};

use crate::addrs::offsets;
use crate::addrs::offsets::wowobject;
use crate::cstr_to_str;

use std::arch::asm;

pub type GUID = u64;

pub struct GUIDFmt(pub GUID);

impl std::fmt::Display for GUIDFmt {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "0x{:016X}", self.0)
    }
}

pub fn guid_from_str(s: &str) -> LoleResult<GUID> {
    let res = GUID::from_str_radix(s.trim_start_matches("0x"), 16)?;
    Ok(res)
}

pub const NO_TARGET: GUID = 0x0;

#[derive(Clone, Copy)]
pub struct WowObject {
    base: Addr,
}

add_repr_and_tryfrom! {
    i32,
    #[derive(Debug)]
    pub enum WowObjectType {
        Object = 0,
        Item = 1,
        Container = 2,
        Npc = 3,
        Unit = 4,
        GameObject = 5,
        DynamicObject = 6,
        Corpse = 7,
        AreaTrigger = 8,
        SceneObject = 9,
        Unknown = 10,
    }
}

impl WowObject {
    fn valid(&self) -> bool {
        (self.base != 0) && ((self.base & 0x3) == 0)
    }
    pub fn get_type(&self) -> WowObjectType {
        let t = deref::<i32, 1>(self.base + wowobject::Type);
        t.try_into().unwrap_or(WowObjectType::Unknown)
    }

    #[cfg(feature = "wotlk")]
    pub fn get_name(&self) -> &str {
        match self.get_type() {
            WowObjectType::Npc | WowObjectType::Unit => {
                let mut result: *const c_char; // looks so weird, but the compiler is giving a warning if it's initialized :D
                let mut unknown: usize = 0; // needs some stack address (never modified, apparently)
                let base = self.base;
                let func_addr = offsets::wow_cfuncs::GetUnitOrNPCNameAddr;
                unsafe {
                    asm!(
                        "push 1",
                        "push {unknown}",
                        "call {func_addr:e}",
                        "mov {result}, eax",
                        in("ecx") base,
                        unknown = in(reg) &unknown,
                        func_addr = in(reg) func_addr,
                        result = out(reg) result,
                        out("eax") _,
                        out("edx") _,
                    )
                }
                cstr_to_str!(result).unwrap_or("(null)")

                // can possibly be refactored to:
                // let ptr = offsets::GetUnitOrNPCNameAddr as *const ();
                // let func: extern "thiscall" fn(*mut c_void) -> *const c_char =
                //     unsafe { std::mem::transmute(ptr) };
                // cstr_to_str!(func(self.base)).unwrap_or("!StringError!")
            }

            _ => "Unknown",
        }
    }

    #[cfg(feature = "tbc")]
    pub fn get_name(&self) -> &str {
        match self.get_type() {
            WowObjectType::Npc | WowObjectType::Unit => {
                let mut result: *const c_char; // looks so weird, but the compiler is giving a warning if it's initialized :D
                let base = self.base;
                let func_addr = offsets::wow_cfuncs::GetUnitOrNPCNameAddr;
                unsafe {
                    asm!(
                        "push 0",
                        "call {func_addr:e}",
                        "mov {result}, eax",
                        in("ecx") base,
                        func_addr = in(reg) func_addr,
                        result = out(reg) result,
                        out("eax") _,
                        out("edx") _,
                    )
                }
                cstr_to_str!(result).unwrap_or("(null)")

                // can possibly be refactored to:
                // let ptr = offsets::GetUnitOrNPCNameAddr as *const ();
                // let func: extern "thiscall" fn(*mut c_void) -> *const c_char =
                //     unsafe { std::mem::transmute(ptr) };
                // cstr_to_str!(func(self.base)).unwrap_or("!StringError!")
            }

            _ => "Unknown",
        }
    }
    pub fn get_guid(&self) -> GUID {
        deref::<GUID, 1>(self.base + wowobject::GUID)
    }
    pub fn get_target_guid(&self) -> Option<GUID> {
        let guid = match self.get_type() {
            WowObjectType::Unit => deref::<GUID, 1>(self.base + wowobject::UnitTargetGUID),
            WowObjectType::Npc => deref::<GUID, 1>(self.base + wowobject::NPCTargetGUID),
            _ => 0x0,
        };
        if guid == 0x0 {
            None
        } else {
            Some(guid)
        }
    }

    pub fn get_target(&self) -> Option<WowObject> {
        if let Some(guid) = self.get_target_guid() {
            let om = ObjectManager::new().ok()?;
            om.get_object_by_guid(guid)
        } else {
            None
        }
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
        match self.get_type() {
            WowObjectType::Npc | WowObjectType::Unit => {
                let movement_info = self.get_movement_info();
                if movement_info.is_null() {
                    return [0., 0., 0., 0.];
                } else {
                    read_elems_from_addr::<4, f32>(movement_info as Addr + 0x10)
                }
            }
            _ => [0., 0., 0., 0.],
        }
    }
    pub fn get_pos(&self) -> Vec3 {
        let [x, y, z, _] = self.get_xyzr();
        Vec3 { x, y, z }
    }
    pub fn get_pos_and_rotvec(&self) -> (Vec3, Vec3) {
        let [x, y, z, r] = self.get_xyzr();
        (Vec3 { x, y, z }, Vec3::from_rot_value(r))
    }
    pub fn get_rotvec(&self) -> Vec3 {
        let rot = deref::<f32, 1>(self.base + wowobject::Rot);
        Vec3::from_rot_value(rot)
    }
    pub fn get_movement_info(&self) -> *const c_void {
        deref::<_, 1>(self.base + wowobject::MovementInfo)
    }

    pub fn yards_in_front_of(&self, yards: f32) -> Vec3 {
        let (pos, rotvec) = self.get_pos_and_rotvec();
        pos + (yards * rotvec)
    }

    pub fn in_combat(&self) -> LoleResult<bool> {
        let unk_state = deref::<Addr, 1>(self.base + wowobject::UnkState);
        if unk_state == 0 {
            return Err(LoleError::NullPtrError);
        }
        let combat_flags = deref::<u32, 1>(unk_state + 0xA0);
        Ok((combat_flags >> 0x13) & 0x1 != 0)
    }
}

impl std::fmt::Display for WowObject {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "[0x{:X}] WowObjectType::[{:?}] - GUID: {} - Name: {}",
            self.base,
            self.get_type(),
            GUIDFmt(self.get_guid()),
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
        let client_connection = deref::<Addr, 1>(offsets::objectmanager::ClientConnection);
        if client_connection == 0 {
            return Err(LoleError::ClientConnectionIsNull);
        }
        let current_object_manager_base =
            deref::<Addr, 1>(client_connection + offsets::objectmanager::CurrentObjectManager);

        if current_object_manager_base == 0 {
            Err(LoleError::ObjectManagerIsNull)
        } else {
            Ok(Self {
                base: current_object_manager_base,
            })
        }
    }
    pub fn iter(&self) -> WowObjectIterator {
        self.into_iter()
    }
    pub fn get_local_guid(&self) -> GUID {
        deref::<GUID, 1>(self.base + offsets::objectmanager::LocalGUIDOffset)
    }
    pub fn get_player_target_guid(&self) -> GUID {
        deref::<GUID, 1>(offsets::PLAYER_TARGET_GUID)
    }
    pub fn get_focus_guid(&self) -> GUID {
        deref::<GUID, 1>(offsets::PLAYER_FOCUS_GUID)
    }
    pub fn get_player(&self) -> LoleResult<WowObject> {
        let local_guid = self.get_local_guid();
        self.iter()
            .find(|w| w.get_guid() == local_guid)
            .ok_or(LoleError::PlayerNotFound)
    }
    pub fn get_player_and_target(&self) -> LoleResult<(WowObject, Option<WowObject>)> {
        let player = self.get_player()?;
        let target_guid = self.get_player_target_guid();
        Ok((player, self.get_object_by_guid(target_guid)))
    }
    pub fn get_object_by_guid(&self, guid: GUID) -> Option<WowObject> {
        if guid != 0 {
            self.iter().find(|w| w.get_guid() == guid)
        } else {
            None
        }
    }
    pub fn get_units_and_npcs(&self) -> impl Iterator<Item = WowObject> {
        self.iter()
            .filter(|w| matches!(w.get_type(), WowObjectType::Npc | WowObjectType::Unit))
    }
    pub fn get_unit_by_name(&self, name: &str) -> Option<WowObject> {
        self.get_units_and_npcs().find(|w| w.get_name() == name)
    }
    fn get_first_object(&self) -> Option<WowObject> {
        // no need to check for base != 0, because ::new is the only way to construct ObjectManager
        WowObject::try_new(deref::<Addr, 1>(
            self.base + offsets::objectmanager::FirstObjectOffset,
        ))
    }
}
